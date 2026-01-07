# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
- Initial release of MariaDB ScyllaDB Storage Engine
- Core storage engine handler implementing MariaDB handler interface
- Connection management using ScyllaDB cpp-rs-driver
- Data type mapping between MariaDB and ScyllaDB
- CQL query builder with automatic ALLOW FILTERING
- Support for CREATE, INSERT, SELECT, UPDATE, DELETE, TRUNCATE operations
- Support for composite primary keys
- Docker-based demo environment
- Comprehensive documentation and examples
- GitHub Copilot instructions for development assistance

### Supported Data Types
- Integer types: TINYINT, SMALLINT, INT, BIGINT
- Floating point: FLOAT, DOUBLE, DECIMAL
- String types: VARCHAR, TEXT, BLOB
- Date/Time: DATE, TIME, DATETIME, TIMESTAMP
- Other: BIT (boolean), ENUM, SET, JSON

### Supported Operations
- Table: CREATE, DROP, TRUNCATE
- Row: INSERT, SELECT, UPDATE, DELETE
- Scanning: Full table scans, primary key lookups
- Filtering: Automatic ALLOW FILTERING for non-PK queries

### Configuration
- System variables: scylla_hosts, scylla_port, scylla_keyspace
- Per-table configuration via COMMENT clause
- Connection pooling and timeout settings

### Documentation
- README.md with complete usage guide
- DOCKER-DEMO.md with Docker setup instructions
- Example SQL and CQL scripts
- Contributing guidelines
- Copilot instructions for AI-assisted development

## [1.0.0] - 2025-01-06

### Added
- Initial public release

---

## Version History

### Version 1.0.0 (2025-01-06)
- First stable release
- Production-ready for basic use cases
- Tested with MariaDB 10.5+ and ScyllaDB 2024.1+

---

## Upcoming Features

Planned for future releases:

- [ ] Prepared statement support for better performance
- [ ] Connection pooling optimization
- [ ] ScyllaDB materialized view support
- [ ] User-defined types (UDTs)
- [ ] Collection types (LIST, SET, MAP)
- [ ] Secondary index support
- [ ] Lightweight transactions (LWT)
- [ ] Batch operation optimization
- [ ] Asynchronous query execution
- [ ] Monitoring and metrics integration
- [ ] Comprehensive test suite
- [ ] Performance benchmarking tools

---

## Migration Guide

### From Direct ScyllaDB Access
If you're currently using ScyllaDB directly and want to add MariaDB access:

1. Install the storage engine plugin
2. Configure connection parameters
3. Create MariaDB tables pointing to existing ScyllaDB tables
4. Existing data will be accessible through MariaDB

### Breaking Changes
None yet - this is the initial release.

---

## Support

For issues, questions, or feature requests:
- GitHub Issues: https://github.com/yourusername/mariadb-scylla-storage-engine/issues
- Discussions: https://github.com/yourusername/mariadb-scylla-storage-engine/discussions
