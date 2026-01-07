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

#ifndef SCYLLA_CONNECTION_H
#define SCYLLA_CONNECTION_H

#include <string>
#include <vector>
#include <memory>
#include <mutex>

// ScyllaDB cpp-rs-driver types
// Note: cpp-rs-driver provides a cassandra.h compatible C API
extern "C" {
  #include <cassandra.h>
}

/**
 * ScyllaConnection - Manages connection to ScyllaDB cluster
 * 
 * This class wraps the ScyllaDB cpp-rs-driver to provide connection
 * management and query execution. The cpp-rs-driver is a Rust-based
 * driver that provides a C API compatible with the Cassandra driver interface.
 */
class ScyllaConnection
{
private:
  CassCluster* cluster;
  CassSession* session;
  std::string current_keyspace;
  bool connected;
  mutable std::mutex mtx;
  
  // Helper methods
  void cleanup();
  std::string get_error_message(CassFuture* future);
  
public:
  ScyllaConnection();
  ~ScyllaConnection();
  
  // Prevent copying
  ScyllaConnection(const ScyllaConnection&) = delete;
  ScyllaConnection& operator=(const ScyllaConnection&) = delete;
  
  /**
   * Connect to ScyllaDB cluster
   * @param hosts Comma-separated list of contact points
   * @param port Native transport port (default 9042)
   * @return true if connection successful
   */
  bool connect(const std::string &hosts, int port = 9042);
  
  /**
   * Disconnect from ScyllaDB cluster
   */
  void disconnect();
  
  /**
   * Check if connected
   */
  bool is_connected() const;
  
  /**
   * Use a specific keyspace
   * @param keyspace Keyspace name
   * @return true if successful
   */
  bool use_keyspace(const std::string &keyspace);
  
  /**
   * Execute a CQL query
   * @param cql CQL query string
   * @param result Result set (vector of rows, each row is a vector of strings)
   * @return true if successful
   */
  bool execute(const std::string &cql, std::vector<std::vector<std::string>> &result);
  
  /**
   * Execute a CQL query without returning results
   * @param cql CQL query string
   * @return true if successful
   */
  bool execute(const std::string &cql);
  
  /**
   * Get current keyspace
   */
  std::string get_keyspace() const;
  
  /**
   * Set connection timeout
   * @param timeout_ms Timeout in milliseconds
   */
  void set_timeout(unsigned int timeout_ms);
  
  /**
   * Set number of IO threads
   * @param num_threads Number of threads
   */
  void set_num_threads(unsigned int num_threads);
};

#endif // SCYLLA_CONNECTION_H
