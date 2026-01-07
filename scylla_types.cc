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

#include "scylla_types.h"
#include <sstream>
#include <iomanip>
#include <cstring>

/**
 * Convert MariaDB field type to ScyllaDB CQL type
 */
std::string ScyllaTypes::mariadb_to_cql_type(Field *field)
{
  switch (field->type()) {
    // Integer types
    case MYSQL_TYPE_TINY:
      return "tinyint";
    case MYSQL_TYPE_SHORT:
      return "smallint";
    case MYSQL_TYPE_LONG:
    case MYSQL_TYPE_INT24:
      return "int";
    case MYSQL_TYPE_LONGLONG:
      return "bigint";
    
    // Floating point types
    case MYSQL_TYPE_FLOAT:
      return "float";
    case MYSQL_TYPE_DOUBLE:
      return "double";
    case MYSQL_TYPE_DECIMAL:
    case MYSQL_TYPE_NEWDECIMAL:
      return "decimal";
    
    // String types
    case MYSQL_TYPE_VARCHAR:
    case MYSQL_TYPE_VAR_STRING:
    case MYSQL_TYPE_STRING:
      return "text";
    case MYSQL_TYPE_TINY_BLOB:
    case MYSQL_TYPE_MEDIUM_BLOB:
    case MYSQL_TYPE_LONG_BLOB:
    case MYSQL_TYPE_BLOB:
      if (field->charset() == &my_charset_bin) {
        return "blob";
      } else {
        return "text";
      }
    
    // Date/Time types
    case MYSQL_TYPE_DATE:
      return "date";
    case MYSQL_TYPE_TIME:
      return "time";
    case MYSQL_TYPE_DATETIME:
    case MYSQL_TYPE_TIMESTAMP:
    case MYSQL_TYPE_TIMESTAMP2:
      return "timestamp";
    
    // Other types
    case MYSQL_TYPE_ENUM:
    case MYSQL_TYPE_SET:
      return "text";
    
    case MYSQL_TYPE_JSON:
      return "text";
    
    // UUID (stored as BINARY(16) or CHAR(36) in MariaDB)
    case MYSQL_TYPE_BIT:
      return "boolean";
    
    default:
      return "text"; // Default fallback
  }
}

/**
 * Get the CQL value representation of a MariaDB field
 */
std::string ScyllaTypes::get_cql_value(Field *field)
{
  if (field->is_null()) {
    return "NULL";
  }
  
  std::ostringstream oss;
  
  switch (field->type()) {
    // Integer types
    case MYSQL_TYPE_TINY:
    case MYSQL_TYPE_SHORT:
    case MYSQL_TYPE_LONG:
    case MYSQL_TYPE_INT24:
    case MYSQL_TYPE_LONGLONG: {
      longlong value = field->val_int();
      oss << value;
      break;
    }
    
    // Floating point types
    case MYSQL_TYPE_FLOAT:
    case MYSQL_TYPE_DOUBLE: {
      double value = field->val_real();
      oss << std::setprecision(15) << value;
      break;
    }
    
    case MYSQL_TYPE_DECIMAL:
    case MYSQL_TYPE_NEWDECIMAL: {
      String str;
      field->val_str(&str);
      oss << str.c_ptr_safe();
      break;
    }
    
    // String types
    case MYSQL_TYPE_VARCHAR:
    case MYSQL_TYPE_VAR_STRING:
    case MYSQL_TYPE_STRING:
    case MYSQL_TYPE_ENUM:
    case MYSQL_TYPE_SET:
    case MYSQL_TYPE_JSON: {
      String str;
      field->val_str(&str);
      oss << "'" << escape_string(std::string(str.c_ptr_safe(), str.length())) << "'";
      break;
    }
    
    // Blob types
    case MYSQL_TYPE_TINY_BLOB:
    case MYSQL_TYPE_MEDIUM_BLOB:
    case MYSQL_TYPE_LONG_BLOB:
    case MYSQL_TYPE_BLOB: {
      String str;
      field->val_str(&str);
      
      if (field->charset() == &my_charset_bin) {
        // Binary data - convert to hex
        oss << "0x";
        const uchar *data = reinterpret_cast<const uchar*>(str.ptr());
        for (size_t i = 0; i < str.length(); i++) {
          oss << std::hex << std::setw(2) << std::setfill('0') << (int)data[i];
        }
      } else {
        // Text data
        oss << "'" << escape_string(std::string(str.c_ptr_safe(), str.length())) << "'";
      }
      break;
    }
    
    // Date/Time types
    case MYSQL_TYPE_DATE: {
      MYSQL_TIME ltime;
      field->get_date(&ltime, 0);
      oss << "'" << std::setfill('0')
          << std::setw(4) << ltime.year << "-"
          << std::setw(2) << ltime.month << "-"
          << std::setw(2) << ltime.day << "'";
      break;
    }
    
    case MYSQL_TYPE_TIME: {
      MYSQL_TIME ltime;
      field->get_time(&ltime);
      oss << "'" << std::setfill('0')
          << std::setw(2) << ltime.hour << ":"
          << std::setw(2) << ltime.minute << ":"
          << std::setw(2) << ltime.second << "'";
      break;
    }
    
    case MYSQL_TYPE_DATETIME:
    case MYSQL_TYPE_TIMESTAMP:
    case MYSQL_TYPE_TIMESTAMP2: {
      MYSQL_TIME ltime;
      field->get_date(&ltime, 0);
      // Convert to Unix timestamp in milliseconds for ScyllaDB
      struct tm tm_struct;
      memset(&tm_struct, 0, sizeof(tm_struct));
      tm_struct.tm_year = ltime.year - 1900;
      tm_struct.tm_mon = ltime.month - 1;
      tm_struct.tm_mday = ltime.day;
      tm_struct.tm_hour = ltime.hour;
      tm_struct.tm_min = ltime.minute;
      tm_struct.tm_sec = ltime.second;
      
      time_t timestamp = mktime(&tm_struct);
      oss << (timestamp * 1000LL + ltime.second_part / 1000);
      break;
    }
    
    case MYSQL_TYPE_BIT: {
      longlong value = field->val_int();
      oss << (value ? "true" : "false");
      break;
    }
    
    default: {
      String str;
      field->val_str(&str);
      oss << "'" << escape_string(std::string(str.c_ptr_safe(), str.length())) << "'";
      break;
    }
  }
  
  return oss.str();
}

/**
 * Store a CQL value into a MariaDB field
 */
void ScyllaTypes::store_field_value(Field *field, const std::string &value)
{
  if (value == "NULL" || value.empty()) {
    field->set_null();
    return;
  }
  
  field->set_notnull();
  
  switch (field->type()) {
    // Integer types
    case MYSQL_TYPE_TINY:
    case MYSQL_TYPE_SHORT:
    case MYSQL_TYPE_LONG:
    case MYSQL_TYPE_INT24:
    case MYSQL_TYPE_LONGLONG: {
      longlong int_val = std::stoll(value);
      field->store(int_val, false);
      break;
    }
    
    // Floating point types
    case MYSQL_TYPE_FLOAT:
    case MYSQL_TYPE_DOUBLE: {
      double double_val = std::stod(value);
      field->store(double_val);
      break;
    }
    
    case MYSQL_TYPE_DECIMAL:
    case MYSQL_TYPE_NEWDECIMAL: {
      field->store(value.c_str(), value.length(), &my_charset_latin1);
      break;
    }
    
    // String types
    case MYSQL_TYPE_VARCHAR:
    case MYSQL_TYPE_VAR_STRING:
    case MYSQL_TYPE_STRING:
    case MYSQL_TYPE_ENUM:
    case MYSQL_TYPE_SET:
    case MYSQL_TYPE_JSON:
    case MYSQL_TYPE_TINY_BLOB:
    case MYSQL_TYPE_MEDIUM_BLOB:
    case MYSQL_TYPE_LONG_BLOB:
    case MYSQL_TYPE_BLOB: {
      field->store(value.c_str(), value.length(), field->charset());
      break;
    }
    
    // Date/Time types
    case MYSQL_TYPE_DATE:
    case MYSQL_TYPE_TIME:
    case MYSQL_TYPE_DATETIME:
    case MYSQL_TYPE_TIMESTAMP:
    case MYSQL_TYPE_TIMESTAMP2: {
      MYSQL_TIME ltime;
      memset(&ltime, 0, sizeof(ltime));
      
      if (field->type() == MYSQL_TYPE_DATE) {
        // Parse date: YYYY-MM-DD
        sscanf(value.c_str(), "%d-%d-%d", 
               &ltime.year, &ltime.month, &ltime.day);
        ltime.time_type = MYSQL_TIMESTAMP_DATE;
      } else if (field->type() == MYSQL_TYPE_TIME) {
        // Parse time: HH:MM:SS
        sscanf(value.c_str(), "%d:%d:%d",
               &ltime.hour, &ltime.minute, &ltime.second);
        ltime.time_type = MYSQL_TIMESTAMP_TIME;
      } else {
        // Parse timestamp (Unix timestamp in milliseconds)
        try {
          long long timestamp_ms = std::stoll(value);
          time_t timestamp_sec = timestamp_ms / 1000;
          struct tm *tm_struct = gmtime(&timestamp_sec);
          
          ltime.year = tm_struct->tm_year + 1900;
          ltime.month = tm_struct->tm_mon + 1;
          ltime.day = tm_struct->tm_mday;
          ltime.hour = tm_struct->tm_hour;
          ltime.minute = tm_struct->tm_min;
          ltime.second = tm_struct->tm_sec;
          ltime.second_part = (timestamp_ms % 1000) * 1000;
          ltime.time_type = MYSQL_TIMESTAMP_DATETIME;
        } catch (...) {
          // If parsing as timestamp fails, try as datetime string
          field->store(value.c_str(), value.length(), field->charset());
          break;
        }
      }
      
      field->store_time(&ltime);
      break;
    }
    
    case MYSQL_TYPE_BIT: {
      longlong bit_val = (value == "true" || value == "1") ? 1 : 0;
      field->store(bit_val, false);
      break;
    }
    
    default: {
      field->store(value.c_str(), value.length(), field->charset());
      break;
    }
  }
}

/**
 * Check if a field type is supported
 */
bool ScyllaTypes::is_supported_type(Field *field)
{
  switch (field->type()) {
    case MYSQL_TYPE_TINY:
    case MYSQL_TYPE_SHORT:
    case MYSQL_TYPE_LONG:
    case MYSQL_TYPE_INT24:
    case MYSQL_TYPE_LONGLONG:
    case MYSQL_TYPE_FLOAT:
    case MYSQL_TYPE_DOUBLE:
    case MYSQL_TYPE_DECIMAL:
    case MYSQL_TYPE_NEWDECIMAL:
    case MYSQL_TYPE_VARCHAR:
    case MYSQL_TYPE_VAR_STRING:
    case MYSQL_TYPE_STRING:
    case MYSQL_TYPE_TINY_BLOB:
    case MYSQL_TYPE_MEDIUM_BLOB:
    case MYSQL_TYPE_LONG_BLOB:
    case MYSQL_TYPE_BLOB:
    case MYSQL_TYPE_DATE:
    case MYSQL_TYPE_TIME:
    case MYSQL_TYPE_DATETIME:
    case MYSQL_TYPE_TIMESTAMP:
    case MYSQL_TYPE_TIMESTAMP2:
    case MYSQL_TYPE_ENUM:
    case MYSQL_TYPE_SET:
    case MYSQL_TYPE_JSON:
    case MYSQL_TYPE_BIT:
      return true;
    
    default:
      return false;
  }
}

/**
 * Escape a string for CQL
 */
std::string ScyllaTypes::escape_string(const std::string &str)
{
  std::string result;
  result.reserve(str.length() * 2);
  
  for (char c : str) {
    if (c == '\'') {
      result += "''"; // Escape single quote by doubling it
    } else {
      result += c;
    }
  }
  
  return result;
}

/**
 * Check if field can be a primary key
 */
bool ScyllaTypes::can_be_primary_key(Field *field)
{
  // Most types can be primary keys except blobs and text
  switch (field->type()) {
    case MYSQL_TYPE_TINY_BLOB:
    case MYSQL_TYPE_MEDIUM_BLOB:
    case MYSQL_TYPE_LONG_BLOB:
    case MYSQL_TYPE_BLOB:
      return false;
    
    default:
      return true;
  }
}
