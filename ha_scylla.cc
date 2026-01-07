/*
   Copyright (c) 2025, MariaDB ScyllaDB Storage Engine

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA
*/

#include "ha_scylla.h"
#include "scylla_types.h"
#include <my_global.h>
#include <sql_class.h>
#include <sql_plugin.h>
#include <mysqld_error.h>
#include <sstream>

// Plugin variables
static char *scylla_default_hosts = NULL;
static unsigned int scylla_default_port = 9042;
static char *scylla_default_keyspace = NULL;

static MYSQL_SYSVAR_STR(hosts, scylla_default_hosts,
  PLUGIN_VAR_RQCMDARG | PLUGIN_VAR_MEMALLOC,
  "Default ScyllaDB contact points (comma-separated)",
  NULL, NULL, "127.0.0.1");

static MYSQL_SYSVAR_UINT(port, scylla_default_port,
  PLUGIN_VAR_RQCMDARG,
  "Default ScyllaDB native transport port",
  NULL, NULL, 9042, 1, 65535, 0);

static MYSQL_SYSVAR_STR(keyspace, scylla_default_keyspace,
  PLUGIN_VAR_RQCMDARG | PLUGIN_VAR_MEMALLOC,
  "Default ScyllaDB keyspace name",
  NULL, NULL, "mariadb");

static struct st_mysql_sys_var* scylla_system_variables[] = {
  MYSQL_SYSVAR(hosts),
  MYSQL_SYSVAR(port),
  MYSQL_SYSVAR(keyspace),
  NULL
};

// Storage engine handlerton
static handler* scylla_create_handler(handlerton *hton, TABLE_SHARE *table,
                                       MEM_ROOT *mem_root);

static int scylla_init_func(void *p);
static int scylla_done_func(void *p);

handlerton *scylla_hton;

/**
 * Create handler instance
 */
static handler* scylla_create_handler(handlerton *hton, TABLE_SHARE *table,
                                       MEM_ROOT *mem_root)
{
  return new (mem_root) ha_scylla(hton, table);
}

/**
 * Initialize storage engine
 */
static int scylla_init_func(void *p)
{
  DBUG_ENTER("scylla_init_func");
  
  scylla_hton = (handlerton *)p;
  scylla_hton->state = SHOW_OPTION_YES;
  scylla_hton->create = scylla_create_handler;
  scylla_hton->flags = HTON_NO_FLAGS;
  scylla_hton->db_type = DB_TYPE_UNKNOWN;
  
  DBUG_RETURN(0);
}

/**
 * Deinitialize storage engine
 */
static int scylla_done_func(void *p)
{
  DBUG_ENTER("scylla_done_func");
  DBUG_RETURN(0);
}

// Plugin declaration
struct st_mysql_storage_engine scylla_storage_engine = {
  MYSQL_HANDLERTON_INTERFACE_VERSION
};

mysql_declare_plugin(scylla)
{
  MYSQL_STORAGE_ENGINE_PLUGIN,
  &scylla_storage_engine,
  "SCYLLA",
  "MariaDB Corporation",
  "ScyllaDB storage engine for MariaDB",
  PLUGIN_LICENSE_GPL,
  scylla_init_func,
  scylla_done_func,
  0x0100, /* version 1.0 */
  NULL,
  scylla_system_variables,
  "1.0",
  MariaDB_PLUGIN_MATURITY_EXPERIMENTAL
}
mysql_declare_plugin_end;

/**
 * Constructor
 */
ha_scylla::ha_scylla(handlerton *hton, TABLE_SHARE *table_arg)
  : handler(hton, table_arg),
    current_position(0),
    scan_active(false),
    scylla_port(scylla_default_port)
{
  if (scylla_default_hosts) {
    scylla_hosts = scylla_default_hosts;
  }
  if (scylla_default_keyspace) {
    keyspace_name = scylla_default_keyspace;
  }
}

/**
 * Destructor
 */
ha_scylla::~ha_scylla()
{
}

/**
 * Parse table comment for ScyllaDB connection parameters
 * Expected format: COMMENT='scylla_hosts=host1,host2;scylla_keyspace=ks;scylla_table=tbl'
 */
int ha_scylla::parse_table_comment(const char *comment)
{
  DBUG_ENTER("ha_scylla::parse_table_comment");
  
  if (!comment || !*comment) {
    DBUG_RETURN(0);
  }
  
  std::string comment_str(comment);
  std::istringstream iss(comment_str);
  std::string token;
  
  while (std::getline(iss, token, ';')) {
    size_t eq_pos = token.find('=');
    if (eq_pos == std::string::npos) continue;
    
    std::string key = token.substr(0, eq_pos);
    std::string value = token.substr(eq_pos + 1);
    
    // Trim whitespace
    key.erase(0, key.find_first_not_of(" \t\r\n"));
    key.erase(key.find_last_not_of(" \t\r\n") + 1);
    value.erase(0, value.find_first_not_of(" \t\r\n"));
    value.erase(value.find_last_not_of(" \t\r\n") + 1);
    
    if (key == "scylla_hosts") {
      scylla_hosts = value;
    } else if (key == "scylla_keyspace") {
      keyspace_name = value;
    } else if (key == "scylla_table") {
      table_name = value;
    } else if (key == "scylla_port") {
      scylla_port = std::stoi(value);
    }
  }
  
  DBUG_RETURN(0);
}

/**
 * Connect to ScyllaDB cluster
 */
int ha_scylla::connect_to_scylla()
{
  DBUG_ENTER("ha_scylla::connect_to_scylla");
  
  if (conn && conn->is_connected()) {
    DBUG_RETURN(0);
  }
  
  try {
    conn = std::make_shared<ScyllaConnection>();
    
    if (scylla_hosts.empty()) {
      scylla_hosts = "127.0.0.1";
    }
    
    if (!conn->connect(scylla_hosts, scylla_port)) {
      my_printf_error(ER_CONNECT_TO_FOREIGN_DATA_SOURCE,
                      "Cannot connect to ScyllaDB cluster at %s:%d",
                      MYF(0), scylla_hosts.c_str(), scylla_port);
      DBUG_RETURN(HA_ERR_NO_CONNECTION);
    }
    
    if (!keyspace_name.empty()) {
      conn->use_keyspace(keyspace_name);
    }
  }
  catch (const std::exception &e) {
    my_printf_error(ER_CONNECT_TO_FOREIGN_DATA_SOURCE,
                    "ScyllaDB connection error: %s", MYF(0), e.what());
    DBUG_RETURN(HA_ERR_NO_CONNECTION);
  }
  
  DBUG_RETURN(0);
}

/**
 * Execute CQL query
 */
int ha_scylla::execute_cql(const std::string &cql)
{
  DBUG_ENTER("ha_scylla::execute_cql");
  
  int rc = connect_to_scylla();
  if (rc) {
    DBUG_RETURN(rc);
  }
  
  try {
    if (!conn->execute(cql, result_set)) {
      my_printf_error(ER_GET_ERRNO, "CQL execution failed: %s",
                      MYF(0), cql.c_str());
      DBUG_RETURN(HA_ERR_GENERIC);
    }
  }
  catch (const std::exception &e) {
    my_printf_error(ER_GET_ERRNO, "CQL execution error: %s", MYF(0), e.what());
    DBUG_RETURN(HA_ERR_GENERIC);
  }
  
  DBUG_RETURN(0);
}

/**
 * Return table capabilities
 */
ulonglong ha_scylla::table_flags() const
{
  return (HA_BINLOG_ROW_CAPABLE | HA_BINLOG_STMT_CAPABLE |
          HA_NO_TRANSACTIONS | HA_REC_NOT_IN_SEQ |
          HA_NULL_IN_KEY | HA_CAN_GEOMETRY | HA_CAN_INDEX_BLOBS |
          HA_AUTO_PART_KEY | HA_CAN_RTREEKEYS);
}

/**
 * Return index capabilities
 */
ulong ha_scylla::index_flags(uint idx, uint part, bool all_parts) const
{
  return (HA_READ_NEXT | HA_READ_PREV | HA_READ_ORDER | HA_READ_RANGE |
          HA_KEYREAD_ONLY);
}

/**
 * Create ScyllaDB table
 */
int ha_scylla::create_scylla_table(const char *name, TABLE *form)
{
  DBUG_ENTER("ha_scylla::create_scylla_table");
  
  if (table_name.empty()) {
    // Extract table name from full name
    const char *table_ptr = strrchr(name, '/');
    if (table_ptr) {
      table_name = table_ptr + 1;
    } else {
      table_name = name;
    }
  }
  
  ScyllaQueryBuilder builder;
  std::string cql = builder.build_create_table_cql(form, keyspace_name, table_name);
  
  int rc = execute_cql(cql);
  if (rc) {
    DBUG_RETURN(rc);
  }
  
  DBUG_RETURN(0);
}

/**
 * Create table
 */
int ha_scylla::create(const char *name, TABLE *form, HA_CREATE_INFO *create_info)
{
  DBUG_ENTER("ha_scylla::create");
  
  // Parse table comment for connection parameters
  if (create_info->comment.str) {
    parse_table_comment(create_info->comment.str);
  }
  
  // Use defaults if not specified
  if (keyspace_name.empty()) {
    keyspace_name = scylla_default_keyspace ? scylla_default_keyspace : "mariadb";
  }
  
  int rc = connect_to_scylla();
  if (rc) {
    DBUG_RETURN(rc);
  }
  
  // Create keyspace if it doesn't exist
  std::string create_ks_cql = "CREATE KEYSPACE IF NOT EXISTS " + keyspace_name +
                              " WITH replication = {'class': 'SimpleStrategy', 'replication_factor': 1}";
  execute_cql(create_ks_cql);
  
  conn->use_keyspace(keyspace_name);
  
  // Create table in ScyllaDB
  rc = create_scylla_table(name, form);
  
  DBUG_RETURN(rc);
}

/**
 * Open table
 */
int ha_scylla::open(const char *name, int mode, uint test_if_locked)
{
  DBUG_ENTER("ha_scylla::open");
  
  // Parse table share comment
  if (table->s->comment.str) {
    parse_table_comment(table->s->comment.str);
  }
  
  // Extract table name if not specified
  if (table_name.empty()) {
    const char *table_ptr = strrchr(name, '/');
    if (table_ptr) {
      table_name = table_ptr + 1;
    } else {
      table_name = name;
    }
  }
  
  // Use defaults if not specified
  if (keyspace_name.empty()) {
    keyspace_name = scylla_default_keyspace ? scylla_default_keyspace : "mariadb";
  }
  
  int rc = connect_to_scylla();
  if (rc) {
    DBUG_RETURN(rc);
  }
  
  thr_lock_data_init(&scylla_hton->lock, &lock, NULL);
  
  DBUG_RETURN(0);
}

/**
 * Close table
 */
int ha_scylla::close(void)
{
  DBUG_ENTER("ha_scylla::close");
  DBUG_RETURN(0);
}

/**
 * Delete table
 */
int ha_scylla::delete_table(const char *name)
{
  DBUG_ENTER("ha_scylla::delete_table");
  
  if (table_name.empty()) {
    const char *table_ptr = strrchr(name, '/');
    if (table_ptr) {
      table_name = table_ptr + 1;
    } else {
      table_name = name;
    }
  }
  
  int rc = connect_to_scylla();
  if (rc) {
    DBUG_RETURN(rc);
  }
  
  std::string cql = "DROP TABLE IF EXISTS " + keyspace_name + "." + table_name;
  rc = execute_cql(cql);
  
  DBUG_RETURN(rc);
}

/**
 * Truncate table
 */
int ha_scylla::truncate()
{
  DBUG_ENTER("ha_scylla::truncate");
  
  std::string cql = "TRUNCATE " + keyspace_name + "." + table_name;
  int rc = execute_cql(cql);
  
  DBUG_RETURN(rc);
}

/**
 * Rename table
 */
int ha_scylla::rename_table(const char *from, const char *to)
{
  DBUG_ENTER("ha_scylla::rename_table");
  
  // ScyllaDB doesn't support table rename directly
  // Would need to create new table and migrate data
  my_printf_error(ER_ILLEGAL_HA, "ScyllaDB storage engine does not support table rename", MYF(0));
  
  DBUG_RETURN(HA_ERR_WRONG_COMMAND);
}

/**
 * Check if query needs ALLOW FILTERING
 */
bool ha_scylla::needs_allow_filtering(TABLE *table_arg)
{
  // For now, always add ALLOW FILTERING to SELECT queries
  // In production, this should be more intelligent based on the query structure
  return true;
}

/**
 * Store result row to MariaDB record buffer
 */
int ha_scylla::store_result_to_record(uchar *buf, size_t row_index)
{
  DBUG_ENTER("ha_scylla::store_result_to_record");
  
  if (row_index >= result_set.size()) {
    DBUG_RETURN(HA_ERR_END_OF_FILE);
  }
  
  my_bitmap_map *org_bitmap = dbug_tmp_use_all_columns(table, table->write_set);
  
  const std::vector<std::string> &row = result_set[row_index];
  
  // Clear the record
  memset(buf, 0, table->s->null_bytes);
  
  for (uint i = 0; i < table->s->fields && i < row.size(); i++) {
    Field *field = table->field[i];
    
    if (row[i].empty() || row[i] == "NULL") {
      field->set_null();
    } else {
      field->set_notnull();
      ScyllaTypes::store_field_value(field, row[i]);
    }
  }
  
  dbug_tmp_restore_column_map(table->write_set, org_bitmap);
  
  DBUG_RETURN(0);
}

/**
 * Write row
 */
int ha_scylla::write_row(uchar *buf)
{
  DBUG_ENTER("ha_scylla::write_row");
  
  ScyllaQueryBuilder builder;
  std::string cql = builder.build_insert_cql(table, buf, keyspace_name, table_name);
  
  int rc = execute_cql(cql);
  
  DBUG_RETURN(rc);
}

/**
 * Update row
 */
int ha_scylla::update_row(const uchar *old_data, uchar *new_data)
{
  DBUG_ENTER("ha_scylla::update_row");
  
  ScyllaQueryBuilder builder;
  std::string cql = builder.build_update_cql(table, old_data, new_data, 
                                             keyspace_name, table_name);
  
  int rc = execute_cql(cql);
  
  DBUG_RETURN(rc);
}

/**
 * Delete row
 */
int ha_scylla::delete_row(const uchar *buf)
{
  DBUG_ENTER("ha_scylla::delete_row");
  
  ScyllaQueryBuilder builder;
  std::string cql = builder.build_delete_cql(table, buf, keyspace_name, table_name);
  
  int rc = execute_cql(cql);
  
  DBUG_RETURN(rc);
}

/**
 * Initialize table scan
 */
int ha_scylla::rnd_init(bool scan)
{
  DBUG_ENTER("ha_scylla::rnd_init");
  
  scan_active = scan;
  current_position = 0;
  result_set.clear();
  
  if (scan) {
    ScyllaQueryBuilder builder;
    std::string cql = builder.build_select_cql(table, keyspace_name, table_name, true);
    
    int rc = execute_cql(cql);
    if (rc) {
      DBUG_RETURN(rc);
    }
  }
  
  DBUG_RETURN(0);
}

/**
 * Get next row in table scan
 */
int ha_scylla::rnd_next(uchar *buf)
{
  DBUG_ENTER("ha_scylla::rnd_next");
  
  if (current_position >= result_set.size()) {
    DBUG_RETURN(HA_ERR_END_OF_FILE);
  }
  
  int rc = store_result_to_record(buf, current_position);
  current_position++;
  
  DBUG_RETURN(rc);
}

/**
 * Position to a specific row
 */
int ha_scylla::rnd_pos(uchar *buf, uchar *pos)
{
  DBUG_ENTER("ha_scylla::rnd_pos");
  
  size_t row_index;
  memcpy(&row_index, pos, sizeof(size_t));
  
  int rc = store_result_to_record(buf, row_index);
  
  DBUG_RETURN(rc);
}

/**
 * Store current position
 */
void ha_scylla::position(const uchar *record)
{
  DBUG_ENTER("ha_scylla::position");
  
  size_t pos = current_position - 1;
  memcpy(ref, &pos, sizeof(size_t));
  
  DBUG_VOID_RETURN;
}

/**
 * End table scan
 */
int ha_scylla::rnd_end()
{
  DBUG_ENTER("ha_scylla::rnd_end");
  
  scan_active = false;
  current_position = 0;
  
  DBUG_RETURN(0);
}

/**
 * Initialize index scan
 */
int ha_scylla::index_init(uint idx, bool sorted)
{
  DBUG_ENTER("ha_scylla::index_init");
  
  active_index = idx;
  DBUG_RETURN(0);
}

/**
 * End index scan
 */
int ha_scylla::index_end()
{
  DBUG_ENTER("ha_scylla::index_end");
  
  active_index = MAX_KEY;
  DBUG_RETURN(0);
}

/**
 * Read row by index
 */
int ha_scylla::index_read_map(uchar *buf, const uchar *key,
                               key_part_map keypart_map,
                               enum ha_rkey_function find_flag)
{
  DBUG_ENTER("ha_scylla::index_read_map");
  
  // Build WHERE clause from key
  ScyllaQueryBuilder builder;
  std::string where_clause = builder.build_where_from_key(table, key, keypart_map);
  std::string cql = builder.build_select_cql(table, keyspace_name, table_name, 
                                             true, where_clause);
  
  result_set.clear();
  current_position = 0;
  
  int rc = execute_cql(cql);
  if (rc) {
    DBUG_RETURN(rc);
  }
  
  if (result_set.empty()) {
    DBUG_RETURN(HA_ERR_KEY_NOT_FOUND);
  }
  
  rc = store_result_to_record(buf, 0);
  current_position = 1;
  
  DBUG_RETURN(rc);
}

/**
 * Read next row in index scan
 */
int ha_scylla::index_next(uchar *buf)
{
  DBUG_ENTER("ha_scylla::index_next");
  DBUG_RETURN(rnd_next(buf));
}

/**
 * Read previous row in index scan
 */
int ha_scylla::index_prev(uchar *buf)
{
  DBUG_ENTER("ha_scylla::index_prev");
  DBUG_RETURN(HA_ERR_WRONG_COMMAND);
}

/**
 * Read first row in index
 */
int ha_scylla::index_first(uchar *buf)
{
  DBUG_ENTER("ha_scylla::index_first");
  
  current_position = 0;
  DBUG_RETURN(rnd_next(buf));
}

/**
 * Read last row in index
 */
int ha_scylla::index_last(uchar *buf)
{
  DBUG_ENTER("ha_scylla::index_last");
  DBUG_RETURN(HA_ERR_WRONG_COMMAND);
}

/**
 * Get table info
 */
int ha_scylla::info(uint flag)
{
  DBUG_ENTER("ha_scylla::info");
  
  if (flag & HA_STATUS_AUTO) {
    stats.auto_increment_value = 1;
  }
  
  if (flag & HA_STATUS_VARIABLE) {
    stats.records = 10000; // Estimate
    stats.deleted = 0;
    stats.data_file_length = 0;
    stats.index_file_length = 0;
    stats.mean_rec_length = 0;
  }
  
  if (flag & HA_STATUS_CONST) {
    stats.create_time = 0;
  }
  
  DBUG_RETURN(0);
}

/**
 * External lock
 */
int ha_scylla::external_lock(THD *thd, int lock_type)
{
  DBUG_ENTER("ha_scylla::external_lock");
  DBUG_RETURN(0);
}

/**
 * Start statement
 */
int ha_scylla::start_stmt(THD *thd, thr_lock_type lock_type)
{
  DBUG_ENTER("ha_scylla::start_stmt");
  DBUG_RETURN(0);
}

/**
 * Store lock
 */
THR_LOCK_DATA **ha_scylla::store_lock(THD *thd, THR_LOCK_DATA **to,
                                       enum thr_lock_type lock_type)
{
  if (lock_type != TL_IGNORE && lock.type == TL_UNLOCK) {
    lock.type = lock_type;
  }
  
  *to++ = &lock;
  
  return to;
}

/**
 * Estimate records in range
 */
ha_rows ha_scylla::records_in_range(uint inx, key_range *min_key,
                                     key_range *max_key)
{
  DBUG_ENTER("ha_scylla::records_in_range");
  DBUG_RETURN(10); // Rough estimate
}
