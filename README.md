# MariaDB ScyllaDB Storage Engine

A MariaDB storage engine that allows you to query ScyllaDB tables directly from MariaDB. This storage engine acts as a bridge between MariaDB and ScyllaDB, translating SQL queries into CQL (Cassandra Query Language) and executing them on a remote ScyllaDB cluster.

## Features

- **Transparent ScyllaDB Access**: Create MariaDB tables that are backed by ScyllaDB tables on a remote cluster
- **Full CRUD Support**: SELECT, INSERT, UPDATE, DELETE, and TRUNCATE operations
- **Data Type Mapping**: Automatic conversion between MariaDB and ScyllaDB data types
- **Auto ALLOW FILTERING**: Automatically adds `ALLOW FILTERING` to CQL queries when necessary
- **Connection Pooling**: Efficient connection management using ScyllaDB's cpp-rs-driver
- **Configurable**: Flexible connection parameters via table comments or system variables

## Supported Versions

- **MariaDB**: All currently supported versions (10.5+)
- **ScyllaDB**: 2024.1 and later
- **Driver**: ScyllaDB cpp-rs-driver (Rust-based with C/C++ API)

## Supported Data Types

| MariaDB Type | ScyllaDB Type |
|--------------|---------------|
| TINYINT | tinyint |
| SMALLINT | smallint |
| INT/INTEGER | int |
| BIGINT | bigint |
| FLOAT | float |
| DOUBLE | double |
| DECIMAL | decimal |
| VARCHAR/TEXT | text |
| BLOB | blob |
| DATE | date |
| TIME | time |
| DATETIME/TIMESTAMP | timestamp |
| ENUM/SET | text |
| JSON | text |
| BIT | boolean |

## Installation

### Prerequisites

**Compile-time dependencies:**

1. **MariaDB Development Headers**
   
   The plugin requires headers from two sources:
   - **libmariadbd-dev package**: Provides built headers like `my_config.h`, `my_global.h`
   - **MariaDB source tree**: Provides `handler.h` and other storage engine headers
   
   ```bash
   # Ubuntu/Debian
   sudo apt-get install libmariadbd-dev
   
   # Then clone MariaDB source for handler.h
   git clone --depth 1 --branch mariadb-<version> https://github.com/MariaDB/server.git /usr/src/mariadb
   # Example: git clone --depth 1 --branch mariadb-10.11 https://github.com/MariaDB/server.git /usr/src/mariadb
   
   # RHEL/CentOS/Fedora
   sudo yum install mariadb-devel
   git clone --depth 1 --branch mariadb-<version> https://github.com/MariaDB/server.git /usr/src/mariadb
   
   # macOS (Homebrew)
   brew install mariadb
   git clone --depth 1 --branch mariadb-<version> https://github.com/MariaDB/server.git /usr/local/src/mariadb
   ```

2. **ScyllaDB cpp-rs-driver**
   
   The cpp-rs-driver is ScyllaDB's Rust-based driver with C/C++ bindings.
   
   ```bash
   # Ubuntu/Debian - Install dependencies first
   sudo apt-get install libuv1-dev libssl-dev libclang-dev pkg-config curl
   
   # Install Rust (required for building cpp-rs-driver)
   curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
   source $HOME/.cargo/env
   
   # Clone and build cpp-rs-driver
   git clone https://github.com/scylladb/cpp-rs-driver.git
   cd cpp-rs-driver
   mkdir build && cd build
   cmake ..
   make -j$(nproc)
   sudo make install
   sudo ldconfig
   
   # macOS (with Homebrew)
   brew install rust libuv openssl pkg-config
   git clone https://github.com/scylladb/cpp-rs-driver.git
   cd cpp-rs-driver
   mkdir build && cd build
   cmake ..
   make -j$(sysctl -n hw.ncpu)
   sudo make install
   ```

3. **Build Tools**
   ```bash
   # Ubuntu/Debian
   sudo apt-get install build-essential cmake
   
   # RHEL/CentOS
   sudo yum groupinstall "Development Tools"
   sudo yum install cmake
   
   # macOS
   xcode-select --install
   brew install cmake
   ```

### Building the Plugin

```bash
# Clone the repository
git clone https://github.com/yourusername/mariadb-scylla-storage-engine.git
cd mariadb-scylla-storage-engine

# Create build directory
mkdir build
cd build

# Configure
cmake ..

# Optional: Specify custom paths if needed
# cmake .. -DMARIADB_INCLUDE_DIR=/path/to/mariadb/include \
#          -DCASSANDRA_INCLUDE_DIR=/path/to/cassandra/include \
#          -DCASSANDRA_LIBRARY=/path/to/libcassandra.so

# Build
make

# Install (requires root/sudo)
sudo make install
```

### Loading the Plugin

1. **Start MariaDB** (if not already running)

2. **Install the plugin:**
   ```sql
   INSTALL SONAME 'ha_scylla';
   ```

3. **Verify installation:**
   ```sql
   SHOW ENGINES;
   -- Should show SCYLLA in the list
   
   SHOW PLUGINS;
   -- Should show scylla plugin
   ```

4. **Configure default settings (optional):**
   ```sql
   SET GLOBAL scylla_hosts = '192.168.1.100,192.168.1.101';
   SET GLOBAL scylla_port = 9042;
   SET GLOBAL scylla_keyspace = 'myapp';
   ```

   Add to `/etc/my.cnf` or `/etc/mysql/mariadb.conf.d/50-server.cnf` for persistence:
   ```ini
   [server]
   scylla_hosts = 192.168.1.100,192.168.1.101
   scylla_port = 9042
   scylla_keyspace = myapp
   ```

## Usage

### Creating a ScyllaDB-backed Table

#### Method 1: Using Global Defaults

```sql
-- Set defaults
SET GLOBAL scylla_hosts = '127.0.0.1';
SET GLOBAL scylla_keyspace = 'myapp';

-- Create table
CREATE TABLE users (
  id INT PRIMARY KEY,
  name VARCHAR(100),
  email VARCHAR(100),
  created_at TIMESTAMP
) ENGINE=SCYLLA;
```

#### Method 2: Using Table Comments

```sql
CREATE TABLE products (
  product_id INT PRIMARY KEY,
  product_name VARCHAR(200),
  price DECIMAL(10,2),
  stock INT
) ENGINE=SCYLLA
COMMENT='scylla_hosts=192.168.1.100;scylla_keyspace=inventory;scylla_table=products';
```

**Comment Parameters:**
- `scylla_hosts`: Comma-separated list of ScyllaDB contact points
- `scylla_port`: Native transport port (default: 9042)
- `scylla_keyspace`: ScyllaDB keyspace name
- `scylla_table`: ScyllaDB table name (defaults to MariaDB table name)

### Basic Operations

#### INSERT

```sql
INSERT INTO users (id, name, email, created_at)
VALUES (1, 'John Doe', 'john@example.com', NOW());

INSERT INTO products VALUES (101, 'Laptop', 999.99, 50);
```

#### SELECT

```sql
-- Simple select
SELECT * FROM users;

-- With WHERE clause
SELECT name, email FROM users WHERE id = 1;

-- With filtering (ALLOW FILTERING is added automatically)
SELECT * FROM products WHERE price > 500;
```

#### UPDATE

```sql
UPDATE users SET email = 'john.doe@example.com' WHERE id = 1;

UPDATE products SET stock = stock + 10 WHERE product_id = 101;
```

#### DELETE

```sql
DELETE FROM users WHERE id = 1;

DELETE FROM products WHERE product_id = 101;
```

#### TRUNCATE

```sql
TRUNCATE TABLE users;
```

### Advanced Usage

#### Connecting to Multiple ScyllaDB Clusters

```sql
-- Cluster 1: Users
CREATE TABLE cluster1_users (
  id INT PRIMARY KEY,
  name VARCHAR(100)
) ENGINE=SCYLLA
COMMENT='scylla_hosts=cluster1-node1.example.com,cluster1-node2.example.com;scylla_keyspace=users_ks';

-- Cluster 2: Analytics
CREATE TABLE cluster2_analytics (
  id INT PRIMARY KEY,
  event VARCHAR(100)
) ENGINE=SCYLLA
COMMENT='scylla_hosts=cluster2-node1.example.com;scylla_keyspace=analytics_ks';
```

#### Working with Complex Primary Keys

```sql
-- Composite primary key
CREATE TABLE events (
  user_id INT,
  event_time TIMESTAMP,
  event_type VARCHAR(50),
  data TEXT,
  PRIMARY KEY (user_id, event_time)
) ENGINE=SCYLLA;
```

## Configuration

### System Variables

| Variable | Type | Default | Description |
|----------|------|---------|-------------|
| `scylla_hosts` | String | "127.0.0.1" | Comma-separated list of ScyllaDB contact points |
| `scylla_port` | Integer | 9042 | ScyllaDB native transport port |
| `scylla_keyspace` | String | "mariadb" | Default keyspace name |

### Setting Variables

```sql
-- Session level
SET scylla_hosts = '192.168.1.100,192.168.1.101';

-- Global level (persists across sessions)
SET GLOBAL scylla_hosts = '192.168.1.100,192.168.1.101';
```

## Limitations

1. **No Transactions**: ScyllaDB is eventually consistent; ACID transactions are not supported
2. **Limited Index Support**: Only primary key lookups are efficient; secondary indexes require ALLOW FILTERING
3. **No Foreign Keys**: Foreign key constraints are not supported
4. **No Table Rename**: The `RENAME TABLE` operation is not supported
5. **Primary Key Required**: All tables must have a primary key defined
6. **No AUTO_INCREMENT**: ScyllaDB doesn't support auto-increment; use application-generated IDs or UUIDs

## Troubleshooting

### Plugin Won't Load

```sql
-- Check error log
SHOW VARIABLES LIKE 'log_error';

-- Common issues:
-- 1. Library not found: Check LD_LIBRARY_PATH includes cassandra driver location
-- 2. Permission denied: Ensure plugin file has correct permissions (chmod 755)
-- 3. Missing dependencies: Run 'ldd ha_scylla.so' to check for missing libraries
```

### Connection Failures

```sql
-- Test connectivity from command line first
cqlsh <scylla_host> <port>

-- Check MariaDB error log for detailed error messages

-- Verify hosts and port in table comment or global variables
SHOW GLOBAL VARIABLES LIKE 'scylla%';
```

### Query Performance

```sql
-- Ensure primary key is used in WHERE clauses when possible
-- Avoid full table scans on large tables
-- Consider adding appropriate indexes in ScyllaDB

-- Monitor slow queries
SET GLOBAL slow_query_log = 1;
SET GLOBAL long_query_time = 2;
```

## Development

### Building for Development

```bash
# Debug build
cmake .. -DCMAKE_BUILD_TYPE=Debug
make

# With verbose output
make VERBOSE=1
```

### Running Tests

```bash
# TODO: Add test suite
```

### Contributing

Contributions are welcome! Please:

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Add tests if applicable
5. Submit a pull request

## License

This project is licensed under the GNU General Public License v2.0 - see the LICENSE file for details.

## Support

- **Issues**: https://github.com/yourusername/mariadb-scylla-storage-engine/issues
- **Discussions**: https://github.com/yourusername/mariadb-scylla-storage-engine/discussions
- **ScyllaDB Documentation**: https://docs.scylladb.com/
- **MariaDB Storage Engine Documentation**: https://mariadb.com/kb/en/storage-engines/

## Acknowledgments

- ScyllaDB for the cpp-rs-driver
- MariaDB Corporation for storage engine infrastructure

## Roadmap

- [ ] Add support for prepared statements
- [ ] Implement connection pooling optimization
- [ ] Add materialized view support
- [ ] Support for ScyllaDB user-defined types (UDTs)
- [ ] Add comprehensive test suite
- [ ] Performance benchmarking tools
- [ ] Support for ScyllaDB lightweight transactions (LWT)
- [ ] Integration with MariaDB's INFORMATION_SCHEMA

## See Also

- [DOCKER-DEMO.md](DOCKER-DEMO.md) - Docker-based demonstration environment
- [GitHub Copilot Instructions](.github/copilot-instructions.md) - AI assistance guidelines for this project
