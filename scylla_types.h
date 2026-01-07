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

#ifndef SCYLLA_TYPES_H
#define SCYLLA_TYPES_H

#include <my_global.h>
#include <field.h>
#include <string>

/**
 * ScyllaTypes - Utilities for mapping between MariaDB and ScyllaDB data types
 */
class ScyllaTypes
{
public:
  /**
   * Convert MariaDB field type to ScyllaDB CQL type
   * @param field MariaDB field
   * @return CQL type string
   */
  static std::string mariadb_to_cql_type(Field *field);
  
  /**
   * Get the CQL value representation of a MariaDB field
   * @param field MariaDB field
   * @return CQL value string (properly quoted/escaped)
   */
  static std::string get_cql_value(Field *field);
  
  /**
   * Store a CQL value into a MariaDB field
   * @param field MariaDB field
   * @param value String value from ScyllaDB
   */
  static void store_field_value(Field *field, const std::string &value);
  
  /**
   * Check if a field type is supported
   * @param field MariaDB field
   * @return true if supported
   */
  static bool is_supported_type(Field *field);
  
  /**
   * Escape a string for CQL
   * @param str String to escape
   * @return Escaped string
   */
  static std::string escape_string(const std::string &str);
  
  /**
   * Check if field can be a primary key
   * @param field MariaDB field
   * @return true if can be primary key
   */
  static bool can_be_primary_key(Field *field);
};

#endif // SCYLLA_TYPES_H
