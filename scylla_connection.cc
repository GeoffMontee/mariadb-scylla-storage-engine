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

#include "scylla_connection.h"
#include <sstream>
#include <iomanip>
#include <stdexcept>
#include <cstring>

/*
 * ScyllaConnection implementation
 * 
 * This class wraps the ScyllaDB cpp-rs-driver to provide connection
 * management and query execution. The cpp-rs-driver is a Rust-based
 * driver that provides a C API compatible with the Cassandra driver interface.
 */

/**
 * Constructor
 */
ScyllaConnection::ScyllaConnection()
  : cluster(nullptr),
    session(nullptr),
    connected(false)
{
}

/**
 * Destructor
 */
ScyllaConnection::~ScyllaConnection()
{
  cleanup();
}

/**
 * Cleanup resources
 */
void ScyllaConnection::cleanup()
{
  std::lock_guard<std::mutex> lock(mtx);
  
  if (session) {
    CassFuture* close_future = cass_session_close(session);
    cass_future_wait(close_future);
    cass_future_free(close_future);
    cass_session_free(session);
    session = nullptr;
  }
  
  if (cluster) {
    cass_cluster_free(cluster);
    cluster = nullptr;
  }
  
  connected = false;
}

/**
 * Get error message from future
 */
std::string ScyllaConnection::get_error_message(CassFuture* future)
{
  const char* message;
  size_t message_length;
  cass_future_error_message(future, &message, &message_length);
  return std::string(message, message_length);
}

/**
 * Connect to ScyllaDB cluster
 */
bool ScyllaConnection::connect(const std::string &hosts, int port)
{
  std::lock_guard<std::mutex> lock(mtx);
  
  if (connected) {
    return true;
  }
  
  // Create cluster configuration
  cluster = cass_cluster_new();
  if (!cluster) {
    return false;
  }
  
  // Set contact points
  cass_cluster_set_contact_points(cluster, hosts.c_str());
  cass_cluster_set_port(cluster, port);
  
  // Set protocol version to v4 (compatible with ScyllaDB)
  cass_cluster_set_protocol_version(cluster, CASS_PROTOCOL_VERSION_V4);
  
  // Set connection timeout
  cass_cluster_set_connect_timeout(cluster, 10000); // 10 seconds
  cass_cluster_set_request_timeout(cluster, 10000); // 10 seconds
  
  // Create session
  session = cass_session_new();
  if (!session) {
    cass_cluster_free(cluster);
    cluster = nullptr;
    return false;
  }
  
  // Connect session
  CassFuture* connect_future = cass_session_connect(session, cluster);
  cass_future_wait(connect_future);
  
  CassError rc = cass_future_error_code(connect_future);
  if (rc != CASS_OK) {
    std::string error = get_error_message(connect_future);
    cass_future_free(connect_future);
    cass_session_free(session);
    cass_cluster_free(cluster);
    session = nullptr;
    cluster = nullptr;
    return false;
  }
  
  cass_future_free(connect_future);
  connected = true;
  
  return true;
}

/**
 * Disconnect from ScyllaDB cluster
 */
void ScyllaConnection::disconnect()
{
  cleanup();
}

/**
 * Check if connected
 */
bool ScyllaConnection::is_connected() const
{
  std::lock_guard<std::mutex> lock(mtx);
  return connected;
}

/**
 * Use a specific keyspace
 */
bool ScyllaConnection::use_keyspace(const std::string &keyspace)
{
  std::lock_guard<std::mutex> lock(mtx);
  
  if (!connected || !session) {
    return false;
  }
  
  std::string cql = "USE " + keyspace;
  CassStatement* statement = cass_statement_new(cql.c_str(), 0);
  CassFuture* query_future = cass_session_execute(session, statement);
  
  cass_future_wait(query_future);
  
  CassError rc = cass_future_error_code(query_future);
  bool success = (rc == CASS_OK);
  
  if (success) {
    current_keyspace = keyspace;
  }
  
  cass_future_free(query_future);
  cass_statement_free(statement);
  
  return success;
}

/**
 * Execute a CQL query with results
 */
bool ScyllaConnection::execute(const std::string &cql, 
                                std::vector<std::vector<std::string>> &result)
{
  std::lock_guard<std::mutex> lock(mtx);
  
  if (!connected || !session) {
    return false;
  }
  
  result.clear();
  
  CassStatement* statement = cass_statement_new(cql.c_str(), 0);
  CassFuture* query_future = cass_session_execute(session, statement);
  
  cass_future_wait(query_future);
  
  CassError rc = cass_future_error_code(query_future);
  if (rc != CASS_OK) {
    cass_future_free(query_future);
    cass_statement_free(statement);
    return false;
  }
  
  const CassResult* cass_result = cass_future_get_result(query_future);
  if (cass_result) {
    CassIterator* row_iterator = cass_iterator_from_result(cass_result);
    
    while (cass_iterator_next(row_iterator)) {
      const CassRow* row = cass_iterator_get_row(row_iterator);
      std::vector<std::string> row_data;
      
      size_t column_count = cass_result_column_count(cass_result);
      for (size_t i = 0; i < column_count; i++) {
        const CassValue* value = cass_row_get_column(row, i);
        
        if (cass_value_is_null(value)) {
          row_data.push_back("NULL");
        } else {
          CassValueType type = cass_value_type(value);
          
          switch (type) {
            case CASS_VALUE_TYPE_TINY_INT: {
              cass_int8_t tinyint_val;
              cass_value_get_int8(value, &tinyint_val);
              row_data.push_back(std::to_string(tinyint_val));
              break;
            }
            case CASS_VALUE_TYPE_SMALL_INT: {
              cass_int16_t smallint_val;
              cass_value_get_int16(value, &smallint_val);
              row_data.push_back(std::to_string(smallint_val));
              break;
            }
            case CASS_VALUE_TYPE_INT: {
              cass_int32_t int_val;
              cass_value_get_int32(value, &int_val);
              row_data.push_back(std::to_string(int_val));
              break;
            }
            case CASS_VALUE_TYPE_BIGINT: {
              cass_int64_t bigint_val;
              cass_value_get_int64(value, &bigint_val);
              row_data.push_back(std::to_string(bigint_val));
              break;
            }
            case CASS_VALUE_TYPE_FLOAT: {
              cass_float_t float_val;
              cass_value_get_float(value, &float_val);
              row_data.push_back(std::to_string(float_val));
              break;
            }
            case CASS_VALUE_TYPE_DOUBLE: {
              cass_double_t double_val;
              cass_value_get_double(value, &double_val);
              row_data.push_back(std::to_string(double_val));
              break;
            }
            case CASS_VALUE_TYPE_BOOLEAN: {
              cass_bool_t bool_val;
              cass_value_get_bool(value, &bool_val);
              row_data.push_back(bool_val ? "1" : "0");
              break;
            }
            case CASS_VALUE_TYPE_TEXT:
            case CASS_VALUE_TYPE_VARCHAR:
            case CASS_VALUE_TYPE_ASCII: {
              const char* str_val;
              size_t str_len;
              cass_value_get_string(value, &str_val, &str_len);
              row_data.push_back(std::string(str_val, str_len));
              break;
            }
            case CASS_VALUE_TYPE_TIMESTAMP: {
              cass_int64_t timestamp_val;
              cass_value_get_int64(value, &timestamp_val);
              row_data.push_back(std::to_string(timestamp_val));
              break;
            }
            case CASS_VALUE_TYPE_DATE: {
              cass_uint32_t date_val;
              cass_value_get_uint32(value, &date_val);
              // CQL date is days since epoch (1970-01-01)
              // Convert to YYYY-MM-DD format
              time_t epoch_time = (time_t)date_val * 86400; // seconds
              struct tm* tm_info = gmtime(&epoch_time);
              char date_str[11];
              strftime(date_str, sizeof(date_str), "%Y-%m-%d", tm_info);
              row_data.push_back(std::string(date_str));
              break;
            }
            case CASS_VALUE_TYPE_UUID:
            case CASS_VALUE_TYPE_TIMEUUID: {
              char uuid_str[CASS_UUID_STRING_LENGTH];
              CassUuid uuid;
              cass_value_get_uuid(value, &uuid);
              cass_uuid_string(uuid, uuid_str);
              row_data.push_back(std::string(uuid_str));
              break;
            }
            case CASS_VALUE_TYPE_BLOB: {
              const cass_byte_t* bytes;
              size_t bytes_size;
              cass_value_get_bytes(value, &bytes, &bytes_size);
              
              // Convert to hex string
              std::ostringstream hex_stream;
              hex_stream << "0x";
              for (size_t j = 0; j < bytes_size; j++) {
                hex_stream << std::hex << std::setw(2) << std::setfill('0') 
                          << static_cast<int>(bytes[j]);
              }
              row_data.push_back(hex_stream.str());
              break;
            }
            case CASS_VALUE_TYPE_DECIMAL: {
              const cass_byte_t* varint;
              size_t varint_size;
              cass_int32_t scale;
              cass_value_get_decimal(value, &varint, &varint_size, &scale);
              
              // Convert varint bytes to a number
              int64_t value_int = 0;
              for (size_t i = 0; i < varint_size; i++) {
                value_int = (value_int << 8) | varint[i];
              }
              
              // Apply scale to create decimal string
              std::ostringstream decimal_stream;
              if (scale == 0) {
                decimal_stream << value_int;
              } else {
                // Insert decimal point at the right position
                std::string num_str = std::to_string(value_int);
                if (scale >= num_str.length()) {
                  // Pad with zeros if needed
                  decimal_stream << "0.";
                  for (int i = 0; i < scale - num_str.length(); i++) {
                    decimal_stream << "0";
                  }
                  decimal_stream << num_str;
                } else {
                  size_t decimal_pos = num_str.length() - scale;
                  decimal_stream << num_str.substr(0, decimal_pos) << "." 
                                << num_str.substr(decimal_pos);
                }
              }
              row_data.push_back(decimal_stream.str());
              break;
            }
            case CASS_VALUE_TYPE_VARINT: {
              const cass_byte_t* varint;
              size_t varint_size;
              cass_value_get_bytes(value, &varint, &varint_size);
              
              // Convert varint bytes to integer (big-endian, signed)
              bool is_negative = (varint[0] & 0x80) != 0;
              int64_t value_int = 0;
              
              if (is_negative) {
                // Two's complement for negative numbers
                value_int = -1;
                for (size_t i = 0; i < varint_size && i < 8; i++) {
                  value_int = (value_int << 8) | varint[i];
                }
              } else {
                // Positive number
                for (size_t i = 0; i < varint_size && i < 8; i++) {
                  value_int = (value_int << 8) | varint[i];
                }
              }
              
              row_data.push_back(std::to_string(value_int));
              break;
            }
            case CASS_VALUE_TYPE_TIME: {
              cass_int64_t time_val;
              cass_value_get_int64(value, &time_val);
              // CQL time is nanoseconds since midnight
              int64_t total_seconds = time_val / 1000000000LL;
              int hours = total_seconds / 3600;
              int minutes = (total_seconds % 3600) / 60;
              int seconds = total_seconds % 60;
              int micros = (time_val % 1000000000LL) / 1000;
              
              char time_str[20];
              snprintf(time_str, sizeof(time_str), "%02d:%02d:%02d.%06d", 
                      hours, minutes, seconds, micros);
              row_data.push_back(std::string(time_str));
              break;
            }
            case CASS_VALUE_TYPE_DURATION: {
              cass_int32_t months, days;
              cass_int64_t nanos;
              cass_value_get_duration(value, &months, &days, &nanos);
              
              // Format as ISO 8601 duration string
              std::ostringstream duration_stream;
              duration_stream << "P";
              if (months != 0) {
                duration_stream << months << "M";
              }
              if (days != 0) {
                duration_stream << days << "D";
              }
              if (nanos != 0) {
                int64_t total_seconds = nanos / 1000000000LL;
                int hours = total_seconds / 3600;
                int minutes = (total_seconds % 3600) / 60;
                int seconds = total_seconds % 60;
                duration_stream << "T";
                if (hours != 0) duration_stream << hours << "H";
                if (minutes != 0) duration_stream << minutes << "M";
                if (seconds != 0 || nanos % 1000000000LL != 0) {
                  duration_stream << seconds;
                  if (nanos % 1000000000LL != 0) {
                    duration_stream << "." << (nanos % 1000000000LL);
                  }
                  duration_stream << "S";
                }
              }
              if (months == 0 && days == 0 && nanos == 0) {
                duration_stream << "T0S";
              }
              row_data.push_back(duration_stream.str());
              break;
            }
            case CASS_VALUE_TYPE_INET: {
              CassInet inet;
              cass_value_get_inet(value, &inet);
              char inet_str[CASS_INET_STRING_LENGTH];
              cass_inet_string(inet, inet_str);
              row_data.push_back(std::string(inet_str));
              break;
            }
            default:
              row_data.push_back("[UNSUPPORTED_TYPE]");
              break;
          }
        }
      }
      
      result.push_back(row_data);
    }
    
    cass_iterator_free(row_iterator);
    cass_result_free(cass_result);
  }
  
  cass_future_free(query_future);
  cass_statement_free(statement);
  
  return true;
}

/**
 * Execute a CQL query without results
 */
bool ScyllaConnection::execute(const std::string &cql)
{
  std::vector<std::vector<std::string>> dummy_result;
  return execute(cql, dummy_result);
}

/**
 * Get current keyspace
 */
std::string ScyllaConnection::get_keyspace() const
{
  std::lock_guard<std::mutex> lock(mtx);
  return current_keyspace;
}

/**
 * Set connection timeout
 */
void ScyllaConnection::set_timeout(unsigned int timeout_ms)
{
  std::lock_guard<std::mutex> lock(mtx);
  
  if (cluster) {
    cass_cluster_set_connect_timeout(cluster, timeout_ms);
    cass_cluster_set_request_timeout(cluster, timeout_ms);
  }
}

/**
 * Set number of IO threads
 */
void ScyllaConnection::set_num_threads(unsigned int num_threads)
{
  std::lock_guard<std::mutex> lock(mtx);
  
  if (cluster) {
    cass_cluster_set_num_threads_io(cluster, num_threads);
  }
}
