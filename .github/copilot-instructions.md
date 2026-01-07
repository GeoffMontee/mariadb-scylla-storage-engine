# GitHub Copilot Instructions for MariaDB ScyllaDB Storage Engine

This document provides guidelines for AI assistants (like GitHub Copilot) when helping with development of the MariaDB ScyllaDB Storage Engine.

## Project Overview

This is a MariaDB storage engine plugin that bridges MariaDB and ScyllaDB, allowing MariaDB tables to be backed by ScyllaDB tables on a remote cluster. The storage engine translates SQL operations to CQL (Cassandra Query Language).

### Key Technologies
- **Language**: C++11
- **MariaDB API**: Storage engine handler interface
- **ScyllaDB Driver**: cpp-rs-driver (ScyllaDB's Rust-based driver with C/C++ API)
- **Build System**: CMake
- **Containerization**: Docker, Docker Compose

### Build Requirements
The plugin requires MariaDB headers from two sources:
1. **libmariadbd-dev package**: Provides generated/built headers (`my_config.h`, `my_global.h`, etc.)
2. **MariaDB source tree**: Provides storage engine headers (`handler.h` in `sql/` directory)

Both are required because:
- `my_config.h` is generated during MariaDB build and not in the source tree
- `handler.h` is in the source tree but not packaged in libmariadbd-dev

The Dockerfile installs libmariadbd-dev and clones the MariaDB source. CMakeLists.txt searches for headers in both locations.

## Architecture

### Core Components

1. **ha_scylla.{h,cc}**: Main storage engine handler
   - Inherits from MariaDB's `handler` base class
   - Implements table operations (open, close, create, delete, truncate)
   - Implements row operations (read, write, update, delete)
   - Manages scanning and indexing operations

2. **scylla_connection.{h,cc}**: Connection manager
   - Wraps ScyllaDB cpp-rs-driver
   - Manages cluster connections
   - Handles query execution and result retrieval
   - Thread-safe with mutex protection

3. **scylla_types.{h,cc}**: Data type mapping
   - Converts MariaDB types to ScyllaDB/CQL types
   - Handles value serialization/deserialization
   - Supports most common data types

4. **scylla_query.{h,cc}**: CQL query builder
   - Builds CQL queries from MariaDB operations
   - Automatically adds ALLOW FILTERING when needed
   - Handles primary keys and WHERE clauses

## Development Guidelines

### Code Style

- **Naming Conventions**:
  - Classes: PascalCase (e.g., `ScyllaConnection`)
  - Methods: snake_case (e.g., `connect_to_scylla`)
  - Member variables: snake_case with no prefix (e.g., `keyspace_name`)
  - Constants: UPPER_SNAKE_CASE (e.g., `MAX_RETRIES`)

- **Comments**:
  - Use Doxygen-style comments for public APIs
  - Add inline comments for complex logic
  - Include copyright header in all files

- **Error Handling**:
  - Use MariaDB's error reporting: `my_printf_error()`
  - Return appropriate HA_ERR_* codes
  - Log errors for debugging

### MariaDB Storage Engine API

When implementing storage engine methods:

- Always use `DBUG_ENTER()` and `DBUG_RETURN()` macros
- Handle table locks properly with `THR_LOCK`
- Use `bitmap` APIs to access table fields safely
- Respect `table->read_set` and `table->write_set` bitmaps

Example:
```cpp
int ha_scylla::write_row(uchar *buf)
{
  DBUG_ENTER("ha_scylla::write_row");
  
  my_bitmap_map *org_bitmap = dbug_tmp_use_all_columns(table, table->read_set);
  
  // ... your code ...
  
  dbug_tmp_restore_column_map(table->read_set, org_bitmap);
  
  DBUG_RETURN(0);
}
```

### ScyllaDB Driver Usage

When working with the cpp-rs-driver:

- Always check return codes from Cassandra API calls
- Free resources properly (futures, results, statements)
- Use prepared statements for repeated queries (future enhancement)
- Handle NULL values explicitly

Example:
```cpp
CassStatement* statement = cass_statement_new(cql.c_str(), 0);
CassFuture* future = cass_session_execute(session, statement);
cass_future_wait(future);

if (cass_future_error_code(future) != CASS_OK) {
  // Handle error
  cass_future_free(future);
  cass_statement_free(statement);
  return HA_ERR_GENERIC;
}

cass_future_free(future);
cass_statement_free(statement);
```

### Type Mapping Rules

When adding new type support:

1. Update `mariadb_to_cql_type()` for type name conversion
2. Update `get_cql_value()` for serialization to CQL
3. Update `store_field_value()` for deserialization from CQL
4. Update `is_supported_type()` to mark as supported
5. Test thoroughly with NULL values

### Query Building

When constructing CQL queries:

- Always properly escape string values
- Handle NULL values explicitly
- Use primary keys in WHERE clauses when possible
- Add ALLOW FILTERING for non-primary-key queries
- Quote identifiers only when necessary (lowercase, no reserved words)

## Common Tasks

### Adding a New Data Type

1. Edit [scylla_types.cc](scylla_types.cc):
   - Add case to `mariadb_to_cql_type()`
   - Add case to `get_cql_value()`
   - Add case to `store_field_value()`
   - Add case to `is_supported_type()`

2. Test with:
   - NULL values
   - Boundary values
   - Round-trip (write then read)

### Adding a New System Variable

1. Edit [ha_scylla.cc](ha_scylla.cc):
   ```cpp
   static MYSQL_SYSVAR_TYPE(var_name, variable,
     PLUGIN_VAR_RQCMDARG,
     "Description",
     NULL, NULL, default_value, ...);
   ```

2. Add to `scylla_system_variables` array

3. Document in [README.md](README.md)

### Enhancing Query Builder

When modifying query generation:

1. Update relevant method in [scylla_query.cc](scylla_query.cc)
2. Consider edge cases:
   - Composite primary keys
   - NULL values
   - Special characters in identifiers
   - Empty result sets

3. Test with various table structures

### Adding Connection Features

When adding connection capabilities:

1. Add method to [scylla_connection.h](scylla_connection.h)
2. Implement in [scylla_connection.cc](scylla_connection.cc)
3. Use mutex for thread safety
4. Handle connection failures gracefully

## Testing

### Manual Testing

Use the Docker demo environment:

```bash
docker-compose up -d
docker exec -it mariadb-scylla mysql -u root -prootpassword
```

Test scenarios:
- Table creation with various data types
- INSERT, SELECT, UPDATE, DELETE operations
- NULL values
- Primary key and non-primary-key queries
- Connection failures (stop ScyllaDB)
- Large data sets
- Concurrent operations

### Debugging

Enable debugging:

```bash
# In MariaDB
SET GLOBAL debug = 'd:t:i:o,/tmp/mariadb-debug.log';

# Check logs
docker logs mariadb-scylla
tail -f /tmp/mariadb-debug.log
```

Use GDB:
```bash
docker exec -it mariadb-scylla gdb mariadbd
```

## Common Issues and Solutions

### Issue: Plugin Won't Load

**Possible causes:**
- Missing dependencies (check with `ldd ha_scylla.so`)
- Incorrect permissions on plugin file
- MariaDB version incompatibility

**Solution:**
```bash
# Check dependencies
ldd /usr/lib/mysql/plugin/ha_scylla.so

# Fix permissions
chmod 755 /usr/lib/mysql/plugin/ha_scylla.so

# Check MariaDB error log
tail -f /var/lib/mysql/*.err
```

### Issue: Connection to ScyllaDB Fails

**Possible causes:**
- ScyllaDB not ready
- Wrong host/port
- Network issues

**Solution:**
```bash
# Test connectivity
docker exec -it mariadb-scylla ping scylladb-node

# Check ScyllaDB status
docker exec -it scylladb-node nodetool status

# Test CQL connection
docker exec -it scylladb-node cqlsh
```

### Issue: Data Type Mismatch

**Possible causes:**
- Unsupported type
- Incorrect type mapping
- NULL handling issue

**Solution:**
- Check `scylla_types.cc` for type support
- Add debug logging to identify the problematic field
- Test with simpler types first

## Best Practices

### Performance

- Use primary key lookups when possible
- Minimize use of ALLOW FILTERING
- Consider ScyllaDB partition key design
- Batch operations when appropriate
- Use connection pooling effectively

### Reliability

- Always check return codes
- Free all allocated resources
- Handle NULL values explicitly
- Validate input parameters
- Log errors with context

### Security

- Escape all user input in CQL queries
- Validate connection parameters
- Don't log sensitive data
- Use prepared statements (future enhancement)

### Maintainability

- Keep functions focused and small
- Document complex logic
- Use meaningful variable names
- Write self-documenting code
- Add comments for non-obvious decisions

## Resources

### Documentation

- [MariaDB Storage Engine Development](https://mariadb.com/kb/en/storage-engine-development/)
- [ScyllaDB Documentation](https://docs.scylladb.com/)
- [ScyllaDB cpp-rs-driver](https://github.com/scylladb/cpp-rs-driver)
- [CQL Reference](https://cassandra.apache.org/doc/latest/cql/)

### Code References

- MariaDB source: `/storage/` directory
- Example storage engines: `ha_example.cc`, `ha_tina.cc`
- CONNECT storage engine: `storage/connect/`

### Tools

- `cqlsh`: ScyllaDB command-line interface
- `nodetool`: ScyllaDB administration tool
- `mysql`: MariaDB command-line client
- `gdb`: GNU debugger for C++

## Contributing

When contributing to this project:

1. **Follow the coding style** defined in this document
2. **Test thoroughly** using the Docker demo environment
3. **Update documentation** for new features
4. **Add comments** for complex logic
5. **Handle errors** gracefully
6. **Consider backwards compatibility**

## Future Enhancements

Ideas for future development:

- [ ] Prepared statement support
- [ ] Connection pooling optimization
- [ ] ScyllaDB materialized views
- [ ] User-defined types (UDTs)
- [ ] Collection types (list, set, map)
- [ ] Secondary indexes
- [ ] Lightweight transactions (LWT)
- [ ] Batch operations optimization
- [ ] Async query execution
- [ ] Monitoring and metrics

## Questions?

When asking for help:

1. **Provide context**: What are you trying to achieve?
2. **Show code**: Include relevant code snippets
3. **Include errors**: Full error messages and logs
4. **Describe environment**: OS, MariaDB version, ScyllaDB version
5. **Show what you tried**: What debugging steps have you taken?

Example question format:
```
I'm trying to add support for the TIME type in scylla_types.cc.
I've added cases to mariadb_to_cql_type() and get_cql_value(), 
but I'm getting a segfault in store_field_value().

Code:
[paste code snippet]

Error:
[paste error/stack trace]

Environment:
- MariaDB 10.11
- ScyllaDB 5.2
- Ubuntu 22.04

What I've tried:
- Added NULL check
- Verified field type
- Tested with simple values
```

---

Last updated: 2025-01-06
