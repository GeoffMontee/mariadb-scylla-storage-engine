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

#ifndef HA_SCYLLA_H
#define HA_SCYLLA_H

#include <my_global.h>
#include <handler.h>
#include <table.h>
#include <memory>
#include <string>
#include <vector>

#include "scylla_connection.h"
#include "scylla_query.h"

// Forward declarations
class ScyllaConnection;
class ScyllaQueryBuilder;

/**
 * ha_scylla - MariaDB storage engine handler for ScyllaDB
 * 
 * This handler allows MariaDB tables to be backed by ScyllaDB tables,
 * translating SQL operations to CQL queries.
 */
class ha_scylla: public handler
{
private:
  THR_LOCK_DATA lock;                    // MariaDB lock structure
  std::shared_ptr<ScyllaConnection> conn; // Connection to ScyllaDB cluster
  std::string keyspace_name;              // ScyllaDB keyspace name
  std::string table_name;                 // ScyllaDB table name
  
  // Query results
  std::vector<std::vector<std::string>> result_set;
  size_t current_position;
  bool scan_active;
  
  // Table metadata
  std::string primary_key_column;
  std::vector<std::string> clustering_columns;
  
  // Connection parameters stored during table creation
  std::string scylla_hosts;
  int scylla_port;
  
  // Helper methods
  int connect_to_scylla();
  int parse_table_comment(const char *comment);
  int create_scylla_table(const char *name, TABLE *form);
  int execute_cql(const std::string &cql);
  int store_result_to_record(uchar *buf, size_t row_index);
  bool needs_allow_filtering(TABLE *table_arg);
  
public:
  ha_scylla(handlerton *hton, TABLE_SHARE *table_arg);
  ~ha_scylla();
  
  // Storage engine identification
  const char *table_type() const { return "SCYLLA"; }
  const char *index_type(uint inx) { return "NONE"; }
  
  // Capabilities and requirements
  ulonglong table_flags() const;
  ulong index_flags(uint idx, uint part, bool all_parts) const;
  
  // Table operations
  int create(const char *name, TABLE *form, HA_CREATE_INFO *create_info);
  int open(const char *name, int mode, uint test_if_locked);
  int close(void);
  int delete_table(const char *name);
  int truncate();
  int rename_table(const char *from, const char *to);
  
  // Row operations
  int write_row(uchar *buf);
  int update_row(const uchar *old_data, uchar *new_data);
  int delete_row(const uchar *buf);
  
  // Scanning operations
  int index_init(uint idx, bool sorted);
  int index_end();
  int index_read_map(uchar *buf, const uchar *key, key_part_map keypart_map,
                     enum ha_rkey_function find_flag);
  int index_next(uchar *buf);
  int index_prev(uchar *buf);
  int index_first(uchar *buf);
  int index_last(uchar *buf);
  
  int rnd_init(bool scan);
  int rnd_next(uchar *buf);
  int rnd_pos(uchar *buf, uchar *pos);
  void position(const uchar *record);
  int rnd_end();
  
  // Table info
  int info(uint flag);
  int external_lock(THD *thd, int lock_type);
  
  // Transaction support (basic - ScyllaDB is eventually consistent)
  int start_stmt(THD *thd, thr_lock_type lock_type);
  
  // THR_LOCK support
  THR_LOCK_DATA **store_lock(THD *thd, THR_LOCK_DATA **to,
                              enum thr_lock_type lock_type);
  
  // Number of rows estimate
  ha_rows records_in_range(uint inx, key_range *min_key, key_range *max_key);
};

#endif // HA_SCYLLA_H
