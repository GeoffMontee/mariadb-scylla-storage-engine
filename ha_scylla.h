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
  const char *table_type() const override { return "SCYLLA"; }
  const char *index_type(uint inx) override { return "NONE"; }
  
  // Capabilities and requirements
  ulonglong table_flags() const override;
  ulong index_flags(uint idx, uint part, bool all_parts) const override;
  uint max_supported_keys() const override { return 1; }
  uint max_supported_key_parts() const override { return 64; }
  uint max_supported_key_length() const override { return 3500; }
  uint max_supported_key_part_length() const override { return 3500; }
  
  // Table operations
  int create(const char *name, TABLE *form, HA_CREATE_INFO *create_info) override;
  int open(const char *name, int mode, uint test_if_locked) override;
  int close(void) override;
  int delete_table(const char *name) override;
  int truncate() override;
  int rename_table(const char *from, const char *to) override;
  
  // Row operations
  int write_row(const uchar *buf) override;
  int update_row(const uchar *old_data, const uchar *new_data) override;
  int delete_row(const uchar *buf) override;
  
  // Scanning operations
  int index_init(uint idx, bool sorted) override;
  int index_end() override;
  int index_read_map(uchar *buf, const uchar *key, key_part_map keypart_map,
                     enum ha_rkey_function find_flag) override;
  int index_next(uchar *buf) override;
  int index_prev(uchar *buf) override;
  int index_first(uchar *buf) override;
  int index_last(uchar *buf) override;
  
  int rnd_init(bool scan) override;
  int rnd_next(uchar *buf) override;
  int rnd_pos(uchar *buf, uchar *pos) override;
  void position(const uchar *record) override;
  int rnd_end() override;
  
  // Table info
  int info(uint flag) override;
  int external_lock(THD *thd, int lock_type) override;
  
  // Transaction support (basic - ScyllaDB is eventually consistent)
  int start_stmt(THD *thd, thr_lock_type lock_type) override;
  
  // THR_LOCK support
  THR_LOCK_DATA **store_lock(THD *thd, THR_LOCK_DATA **to,
                              enum thr_lock_type lock_type) override;
  
  // Number of rows estimate
  ha_rows records_in_range(uint inx, const key_range *min_key,
                           const key_range *max_key, page_range *pages) override;
};

#endif // HA_SCYLLA_H
