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

#include "scylla_query.h"
#include "scylla_types.h"
#include <sstream>
#include <my_bitmap.h>

/**
 * Build column list for SELECT
 */
std::string ScyllaQueryBuilder::build_column_list(TABLE *table)
{
  std::ostringstream oss;
  bool first = true;
  
  for (uint i = 0; i < table->s->fields; i++) {
    if (!first) {
      oss << ", ";
    }
    oss << table->field[i]->field_name.str;
    first = false;
  }
  
  return oss.str();
}

/**
 * Build values list for INSERT
 */
std::string ScyllaQueryBuilder::build_values_list(TABLE *table, const uchar *buf)
{
  std::ostringstream oss;
  bool first = true;
  
  MY_BITMAP *org_bitmap = dbug_tmp_use_all_columns(table, &table->read_set);
  
  for (uint i = 0; i < table->s->fields; i++) {
    Field *field = table->field[i];
    
    if (!first) {
      oss << ", ";
    }
    
    // Move to the field's position in the buffer
    field->move_field((uchar*)buf + (field->ptr - table->record[0]));
    
    oss << ScyllaTypes::get_cql_value(field);
    
    // Restore field position
    field->move_field(table->record[0] + (field->ptr - (uchar*)buf));
    
    first = false;
  }
  
  dbug_tmp_restore_column_map(&table->read_set, org_bitmap);
  
  return oss.str();
}

/**
 * Build WHERE clause for primary key lookup
 */
std::string ScyllaQueryBuilder::build_primary_key_where(TABLE *table, const uchar *buf)
{
  std::ostringstream oss;
  
  MY_BITMAP *org_bitmap = dbug_tmp_use_all_columns(table, &table->read_set);
  
  // Find primary key fields
  if (table->s->primary_key != MAX_KEY) {
    KEY *key_info = &table->key_info[table->s->primary_key];
    
    for (uint i = 0; i < key_info->user_defined_key_parts; i++) {
      KEY_PART_INFO *key_part = &key_info->key_part[i];
      Field *field = key_part->field;
      
      if (i > 0) {
        oss << " AND ";
      }
      
      // Move to the field's position in the buffer
      field->move_field((uchar*)buf + (field->ptr - table->record[0]));
      
      oss << field->field_name.str << " = " << ScyllaTypes::get_cql_value(field);
      
      // Restore field position
      field->move_field(table->record[0] + (field->ptr - (uchar*)buf));
    }
  } else {
    // No primary key defined - use first field as fallback
    if (table->s->fields > 0) {
      Field *field = table->field[0];
      field->move_field((uchar*)buf + (field->ptr - table->record[0]));
      oss << field->field_name.str << " = " << ScyllaTypes::get_cql_value(field);
      field->move_field(table->record[0] + (field->ptr - (uchar*)buf));
    }
  }
  
  dbug_tmp_restore_column_map(&table->read_set, org_bitmap);
  
  return oss.str();
}

/**
 * Build SET clause for UPDATE
 */
std::string ScyllaQueryBuilder::build_set_clause(TABLE *table, 
                                                  const uchar *old_data,
                                                  const uchar *new_data)
{
  std::ostringstream oss;
  bool first = true;
  
  MY_BITMAP *org_bitmap = dbug_tmp_use_all_columns(table, &table->read_set);
  
  // Get primary key fields to skip them in SET clause
  std::vector<uint> pk_fields;
  if (table->s->primary_key != MAX_KEY) {
    KEY *key_info = &table->key_info[table->s->primary_key];
    for (uint i = 0; i < key_info->user_defined_key_parts; i++) {
      pk_fields.push_back(key_info->key_part[i].fieldnr - 1);
    }
  }
  
  for (uint i = 0; i < table->s->fields; i++) {
    // Skip primary key fields in SET clause
    bool is_pk = false;
    for (uint pk_field : pk_fields) {
      if (pk_field == i) {
        is_pk = true;
        break;
      }
    }
    if (is_pk) continue;
    
    Field *field = table->field[i];
    
    if (!first) {
      oss << ", ";
    }
    
    // Move to the field's position in new data buffer
    field->move_field((uchar*)new_data + (field->ptr - table->record[0]));
    
    oss << field->field_name.str << " = " << ScyllaTypes::get_cql_value(field);
    
    // Restore field position
    field->move_field(table->record[0] + (field->ptr - (uchar*)new_data));
    
    first = false;
  }
  
  dbug_tmp_restore_column_map(&table->read_set, org_bitmap);
  
  return oss.str();
}

/**
 * Check if WHERE clause is empty
 */
bool ScyllaQueryBuilder::has_where_clause(const std::string &where_clause)
{
  return !where_clause.empty() && 
         where_clause.find_first_not_of(" \t\r\n") != std::string::npos;
}

/**
 * Build CREATE TABLE CQL statement
 */
std::string ScyllaQueryBuilder::build_create_table_cql(TABLE *table,
                                                        const std::string &keyspace,
                                                        const std::string &table_name)
{
  std::ostringstream oss;
  
  oss << "CREATE TABLE IF NOT EXISTS " << keyspace << "." << table_name << " (";
  
  // Add columns
  for (uint i = 0; i < table->s->fields; i++) {
    Field *field = table->field[i];
    
    if (i > 0) {
      oss << ", ";
    }
    
    oss << field->field_name.str << " " << ScyllaTypes::mariadb_to_cql_type(field);
  }
  
  // Add primary key
  if (table->s->primary_key != MAX_KEY) {
    KEY *key_info = &table->key_info[table->s->primary_key];
    
    oss << ", PRIMARY KEY (";
    
    for (uint i = 0; i < key_info->user_defined_key_parts; i++) {
      if (i > 0) {
        oss << ", ";
      }
      oss << key_info->key_part[i].field->field_name.str;
    }
    
    oss << ")";
  } else {
    // If no primary key is defined, use the first field
    if (table->s->fields > 0) {
      oss << ", PRIMARY KEY (" << table->field[0]->field_name.str << ")";
    }
  }
  
  oss << ")";
  
  return oss.str();
}

/**
 * Build INSERT CQL statement
 */
std::string ScyllaQueryBuilder::build_insert_cql(TABLE *table, const uchar *buf,
                                                  const std::string &keyspace,
                                                  const std::string &table_name)
{
  std::ostringstream oss;
  
  oss << "INSERT INTO " << keyspace << "." << table_name << " (";
  oss << build_column_list(table);
  oss << ") VALUES (";
  oss << build_values_list(table, buf);
  oss << ")";
  
  return oss.str();
}

/**
 * Build UPDATE CQL statement
 */
std::string ScyllaQueryBuilder::build_update_cql(TABLE *table,
                                                  const uchar *old_data,
                                                  const uchar *new_data,
                                                  const std::string &keyspace,
                                                  const std::string &table_name)
{
  std::ostringstream oss;
  
  oss << "UPDATE " << keyspace << "." << table_name << " SET ";
  oss << build_set_clause(table, old_data, new_data);
  oss << " WHERE ";
  oss << build_primary_key_where(table, old_data);
  
  return oss.str();
}

/**
 * Build DELETE CQL statement
 */
std::string ScyllaQueryBuilder::build_delete_cql(TABLE *table, const uchar *buf,
                                                  const std::string &keyspace,
                                                  const std::string &table_name)
{
  std::ostringstream oss;
  
  oss << "DELETE FROM " << keyspace << "." << table_name;
  oss << " WHERE ";
  oss << build_primary_key_where(table, buf);
  
  return oss.str();
}

/**
 * Build SELECT CQL statement
 */
std::string ScyllaQueryBuilder::build_select_cql(TABLE *table,
                                                  const std::string &keyspace,
                                                  const std::string &table_name,
                                                  bool allow_filtering,
                                                  const std::string &where_clause)
{
  std::ostringstream oss;
  
  oss << "SELECT ";
  oss << build_column_list(table);
  oss << " FROM " << keyspace << "." << table_name;
  
  if (has_where_clause(where_clause)) {
    oss << " WHERE " << where_clause;
  }
  
  if (allow_filtering) {
    oss << " ALLOW FILTERING";
  }
  
  return oss.str();
}

/**
 * Build WHERE clause from index key
 */
std::string ScyllaQueryBuilder::build_where_from_key(TABLE *table,
                                                      const uchar *key,
                                                      key_part_map keypart_map)
{
  std::ostringstream oss;
  
  if (table->s->primary_key != MAX_KEY) {
    KEY *key_info = &table->key_info[table->s->primary_key];
    const uchar *key_ptr = key;
    
    for (uint i = 0; i < key_info->user_defined_key_parts; i++) {
      if (!(keypart_map & (1 << i))) {
        break;
      }
      
      KEY_PART_INFO *key_part = &key_info->key_part[i];
      Field *field = key_part->field;
      
      if (i > 0) {
        oss << " AND ";
      }
      
      // Read the key value
      field->move_field((uchar*)key_ptr);
      oss << field->field_name.str << " = " << ScyllaTypes::get_cql_value(field);
      field->move_field(table->record[0] + (field->ptr - (uchar*)key_ptr));
      
      // Move to next key part
      key_ptr += key_part->store_length;
    }
  }
  
  return oss.str();
}
