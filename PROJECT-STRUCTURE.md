# MariaDB ScyllaDB Storage Engine - Project Structure

This document provides an overview of the project structure and all files.

## Core Source Files

### Storage Engine Handler
- **ha_scylla.h** - Storage engine handler class definition
- **ha_scylla.cc** - Storage engine handler implementation
  - Inherits from MariaDB's `handler` base class
  - Implements all required storage engine methods
  - Manages table and row operations
  - Handles plugin initialization and configuration

### Connection Management
- **scylla_connection.h** - ScyllaDB connection manager interface
- **scylla_connection.cc** - Connection manager implementation
  - Wraps ScyllaDB cpp-rs-driver (Rust-based with C/C++ API)
  - Thread-safe with mutex protection
  - Manages cluster connections and query execution

### Data Type Mapping
- **scylla_types.h** - Type conversion interface
- **scylla_types.cc** - Type conversion implementation
  - Maps MariaDB types to ScyllaDB/CQL types
  - Handles serialization and deserialization
  - Supports most common data types

### Query Building
- **scylla_query.h** - CQL query builder interface
- **scylla_query.cc** - Query builder implementation
  - Constructs CQL queries from MariaDB operations
  - Handles ALLOW FILTERING automatically
  - Manages WHERE clauses and primary keys

## Build System

- **CMakeLists.txt** - Main CMake build configuration
  - Finds MariaDB and ScyllaDB driver dependencies
  - Configures compilation and linking
  - Sets up installation targets

- **plugin.cmake** - Plugin-specific build configuration
  - Plugin metadata
  - Compiler flags
  - Position-independent code settings

## Docker Demo Environment

- **Dockerfile** - MariaDB container with storage engine
  - Based on official MariaDB image
  - Installs build dependencies
  - Builds and installs the storage engine plugin
  - Auto-loads plugin on startup

- **docker-compose.yml** - Complete demo environment
  - ScyllaDB container configuration
  - MariaDB container configuration
  - Network setup for inter-container communication
  - Port forwarding for host access
  - Volume management for data persistence

- **quickstart.sh** - Automated demo setup script
  - Starts all containers
  - Waits for services to be ready
  - Creates sample keyspace and tables
  - Runs demo queries
  - Provides helpful next steps

## Documentation

- **README.md** - Main project documentation
  - Features and capabilities
  - Installation instructions
  - Usage examples
  - Configuration options
  - Troubleshooting guide

- **DOCKER-DEMO.md** - Docker demonstration guide
  - Detailed Docker setup instructions
  - Manual and automated setup options
  - Testing procedures
  - Troubleshooting for Docker environment

- **CONTRIBUTING.md** - Contribution guidelines
  - Development setup
  - Code style guidelines
  - Pull request process
  - Testing requirements

- **CHANGELOG.md** - Version history and changes
  - Release notes
  - Feature additions
  - Bug fixes
  - Roadmap for future releases

- **.github/copilot-instructions.md** - AI assistance guidelines
  - Project architecture overview
  - Development patterns and best practices
  - Common tasks and solutions
  - Code examples and references

## Examples

### examples/example.sql
Comprehensive MariaDB/SQL examples demonstrating:
- Table creation with ScyllaDB engine
- INSERT, SELECT, UPDATE, DELETE operations
- Various data types
- Composite primary keys
- NULL value handling
- Bulk operations
- TRUNCATE operations

### examples/example.cql
ScyllaDB/CQL examples showing:
- Direct ScyllaDB interaction
- Keyspace and table creation
- Data manipulation in CQL
- Collection types (LIST, SET, MAP)
- Secondary indexes
- Materialized views
- Batch operations
- TTL (time-to-live) features

## Project Metadata

- **LICENSE** - Project license (GPL v2 for storage engine)
- **.gitignore** - Git ignore patterns
  - Build artifacts
  - IDE files
  - Docker volumes
  - Temporary files

## Directory Structure

```
mariadb-scylla-storage-engine/
├── .github/
│   └── copilot-instructions.md
├── examples/
│   ├── example.sql
│   └── example.cql
├── ha_scylla.h
├── ha_scylla.cc
├── scylla_connection.h
├── scylla_connection.cc
├── scylla_types.h
├── scylla_types.cc
├── scylla_query.h
├── scylla_query.cc
├── CMakeLists.txt
├── plugin.cmake
├── Dockerfile
├── docker-compose.yml
├── quickstart.sh
├── README.md
├── DOCKER-DEMO.md
├── CONTRIBUTING.md
├── CHANGELOG.md
├── LICENSE
└── .gitignore
```

## Build Artifacts (not in repository)

When you build the project, these are created:

```
build/                  # CMake build directory
├── CMakeCache.txt
├── CMakeFiles/
├── Makefile
├── cmake_install.cmake
└── ha_scylla.so       # The compiled plugin
```

## Docker Volumes (created at runtime)

```
scylladb-data/         # ScyllaDB data persistence
mariadb-data/          # MariaDB data persistence
```

## Quick Navigation

### For Users
1. Start here: [README.md](README.md)
2. Try the demo: [DOCKER-DEMO.md](DOCKER-DEMO.md) or run `./quickstart.sh`
3. Learn by example: [examples/example.sql](examples/example.sql)

### For Developers
1. Read: [.github/copilot-instructions.md](.github/copilot-instructions.md)
2. Check: [CONTRIBUTING.md](CONTRIBUTING.md)
3. Explore: Core source files (ha_scylla.*, scylla_*.*)

### For Building
1. Build config: [CMakeLists.txt](CMakeLists.txt)
2. Dependencies: See README.md prerequisites section
3. Docker build: [Dockerfile](Dockerfile)

## File Statistics

- Total source files: 8 (.h and .cc files)
- Total lines of code: ~3,000+
- Documentation files: 5
- Example files: 2
- Build configuration: 2
- Docker configuration: 3

## Key Features by File

| Feature | Primary File | Supporting Files |
|---------|--------------|------------------|
| Table operations | ha_scylla.cc | scylla_query.cc |
| Data type support | scylla_types.cc | ha_scylla.cc |
| Connection management | scylla_connection.cc | ha_scylla.cc |
| Query generation | scylla_query.cc | scylla_types.cc |
| Plugin registration | ha_scylla.cc | plugin.cmake |
| Docker demo | docker-compose.yml | Dockerfile, quickstart.sh |

## Testing the Complete Setup

```bash
# 1. Quick start (automated)
./quickstart.sh

# 2. Or manual setup
docker-compose up -d
docker exec -it mariadb-scylla mysql -u root -prootpassword

# 3. Run examples
docker exec -i mariadb-scylla mysql -u root -prootpassword < examples/example.sql
docker exec -i scylladb-node cqlsh < examples/example.cql

# 4. Build from source
mkdir build && cd build
cmake ..
make
sudo make install
```

## Getting Help

- Check [README.md](README.md) for usage documentation
- See [DOCKER-DEMO.md](DOCKER-DEMO.md) for Docker-specific issues
- Review [examples/](examples/) for code examples
- Read [CONTRIBUTING.md](CONTRIBUTING.md) for development guidelines
- Consult [.github/copilot-instructions.md](.github/copilot-instructions.md) for architecture details

## Next Steps

1. **Try it**: Run `./quickstart.sh` to see it in action
2. **Learn**: Read through the example files
3. **Customize**: Modify for your use case
4. **Contribute**: See CONTRIBUTING.md to help improve the project
