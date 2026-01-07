# Contributing to MariaDB ScyllaDB Storage Engine

Thank you for your interest in contributing! This document provides guidelines for contributing to the project.

## Getting Started

1. Fork the repository on GitHub
2. Clone your fork locally
3. Create a feature branch from `main`
4. Make your changes
5. Test your changes using the Docker demo environment
6. Submit a pull request

## Development Setup

```bash
# Clone your fork
git clone https://github.com/YOUR_USERNAME/mariadb-scylla-storage-engine.git
cd mariadb-scylla-storage-engine

# Create a branch
git checkout -b feature/your-feature-name

# Set up development environment
docker-compose up -d
```

## Code Style

Please follow the coding conventions outlined in [.github/copilot-instructions.md](.github/copilot-instructions.md):

- **Classes**: PascalCase (e.g., `ScyllaConnection`)
- **Methods**: snake_case (e.g., `connect_to_scylla`)
- **Variables**: snake_case (e.g., `keyspace_name`)
- **Constants**: UPPER_SNAKE_CASE (e.g., `MAX_RETRIES`)

## Testing

Before submitting a PR:

1. **Build the project**: Ensure it compiles without errors
   ```bash
   mkdir build && cd build
   cmake ..
   make
   ```

2. **Test with Docker**: Use the demo environment
   ```bash
   docker-compose up -d
   docker exec -it mariadb-scylla mysql -u root -prootpassword < examples/example.sql
   ```

3. **Test edge cases**:
   - NULL values
   - Large datasets
   - Connection failures
   - Various data types

## Pull Request Process

1. **Update documentation**: If you change functionality, update relevant docs
2. **Add examples**: If appropriate, add examples to `examples/` directory
3. **Describe your changes**: Write a clear PR description explaining:
   - What problem does this solve?
   - How does it work?
   - Are there any breaking changes?
   - How has this been tested?

4. **Link issues**: Reference any related issues with "Fixes #123" or "Relates to #456"

## Commit Messages

Write clear, concise commit messages:

```
Add support for DECIMAL type in scylla_types

- Implement mariadb_to_cql_type conversion
- Add get_cql_value serialization
- Add store_field_value deserialization
- Add tests for boundary values

Fixes #42
```

## Types of Contributions

### Bug Reports

When reporting bugs, include:
- MariaDB version
- ScyllaDB version
- Operating system
- Steps to reproduce
- Expected vs actual behavior
- Error messages and logs

### Feature Requests

Feature requests should include:
- Clear description of the feature
- Use case / motivation
- Example usage
- Implementation suggestions (if any)

### Code Contributions

Areas where contributions are especially welcome:
- Additional data type support
- Performance optimizations
- Better error handling
- Test coverage
- Documentation improvements
- Example queries and use cases

## Code Review

All submissions require review. We'll provide feedback on:
- Code quality and style
- Test coverage
- Documentation
- Performance implications
- Security considerations

## License

By contributing, you agree that your contributions will be licensed under the same license as the project (GPL v2 for storage engine code, per MariaDB requirements).

## Questions?

- Open a [discussion](https://github.com/yourusername/mariadb-scylla-storage-engine/discussions)
- Check existing [issues](https://github.com/yourusername/mariadb-scylla-storage-engine/issues)
- Review the [documentation](README.md)

## Code of Conduct

Be respectful and professional in all interactions. We're all here to build something useful together.

Thank you for contributing! 🎉
