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

#ifndef SCYLLA_QUERY_H
#define SCYLLA_QUERY_H

#include <my_global.h>
#include <table.h>
#include <string>
#include <vector>

/**
 * ScyllaQueryBuilder - Builds CQL queries from MariaDB operations
 */
class ScyllaQueryBuilder
{
private:
  std::string build_column_list(TABLE *table);
  std::string build_values_list(TABLE *table, const uchar *buf);
  std::string build_primary_key_where(TABLE *table, const uchar *buf);
  std::string build_set_clause(TABLE *table, const uchar *old_data, const uchar *new_data);
  bool has_where_clause(const std::string &where_clause);
  
public:
  /**
   * Build CREATE TABLE CQL statement
   * @param table MariaDB table structure
   * @param keyspace ScyllaDB keyspace name
   * @param table_name ScyllaDB table name
   * @return CQL CREATE TABLE statement
   */
  std::string build_create_table_cql(TABLE *table, const std::string &keyspace,
                                      const std::string &table_name);
  
  /**
   * Build INSERT CQL statement
   * @param table MariaDB table structure
   * @param buf Row buffer
   * @param keyspace ScyllaDB keyspace name
   * @param table_name ScyllaDB table name
   * @return CQL INSERT statement
   */
  std::string build_insert_cql(TABLE *table, const uchar *buf,
                                const std::string &keyspace,
                                const std::string &table_name);
  
  /**
   * Build UPDATE CQL statement
   * @param table MariaDB table structure
   * @param old_data Old row buffer
   * @param new_data New row buffer
   * @param keyspace ScyllaDB keyspace name
   * @param table_name ScyllaDB table name
   * @return CQL UPDATE statement
   */
  std::string build_update_cql(TABLE *table, const uchar *old_data,
                                const uchar *new_data,
                                const std::string &keyspace,
                                const std::string &table_name);
  
  /**
   * Build DELETE CQL statement
   * @param table MariaDB table structure
   * @param buf Row buffer
   * @param keyspace ScyllaDB keyspace name
   * @param table_name ScyllaDB table name
   * @return CQL DELETE statement
   */
  std::string build_delete_cql(TABLE *table, const uchar *buf,
                                const std::string &keyspace,
                                const std::string &table_name);
  
  /**
   * Build SELECT CQL statement
   * @param table MariaDB table structure
   * @param keyspace ScyllaDB keyspace name
   * @param table_name ScyllaDB table name
   * @param allow_filtering Whether to add ALLOW FILTERING
   * @param where_clause Optional WHERE clause
   * @return CQL SELECT statement
   */
  std::string build_select_cql(TABLE *table, const std::string &keyspace,
                                const std::string &table_name,
                                bool allow_filtering = false,
                                const std::string &where_clause = "");
  
  /**
   * Build WHERE clause from index key
   * @param table MariaDB table structure
   * @param key Key buffer
   * @param keypart_map Key part map
   * @return WHERE clause string
   */
  std::string build_where_from_key(TABLE *table, const uchar *key,
                                    key_part_map keypart_map);
};

#endif // SCYLLA_QUERY_H
