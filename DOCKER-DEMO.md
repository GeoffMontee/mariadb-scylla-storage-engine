# Docker Demo Environment for MariaDB ScyllaDB Storage Engine

This guide will help you set up a complete demo environment using Docker containers to test the MariaDB ScyllaDB storage engine. The setup includes:

1. A ScyllaDB container for the database backend
2. A MariaDB container with the ScyllaDB storage engine installed
3. A shared network for communication between containers
4. Port forwarding for easy access from the host system

## Prerequisites

- Docker Engine 20.10 or later
- Docker Compose 1.29 or later
- At least 4GB of available RAM
- At least 10GB of available disk space

Install Docker and Docker Compose:

```bash
# Ubuntu/Debian
sudo apt-get update
sudo apt-get install docker.io docker-compose

# RHEL/CentOS
sudo yum install docker docker-compose

# macOS
# Download Docker Desktop from https://www.docker.com/products/docker-desktop
```

## Quick Start

### Option 1: Using Docker Compose (Recommended)

1. **Create the demo environment:**

```bash
# Navigate to the project directory
cd mariadb-scylla-storage-engine

# Start the services
docker-compose up -d
```

2. **Wait for services to be ready:**

```bash
# Check status
docker-compose ps

# Wait for ScyllaDB to be ready (may take 30-60 seconds)
docker-compose logs -f scylladb

# Wait for "Starting listening for CQL clients" message
# Press Ctrl+C when ready

# Check MariaDB logs
docker-compose logs -f mariadb
```

3. **Access the services:**

```bash
# Connect to MariaDB
docker exec -it mariadb-scylla mariadb -u root -prootpassword

# Connect to ScyllaDB
docker exec -it scylladb-node cqlsh
```

4. **Stop the environment:**

```bash
docker-compose down

# To remove volumes as well (WARNING: deletes all data)
docker-compose down -v
```

### Option 2: Manual Docker Setup

If you prefer to set up containers manually:

#### Step 1: Create Docker Network

```bash
docker network create scylla-mariadb-network
```

#### Step 2: Start ScyllaDB Container

```bash
docker run -d \
  --name scylladb-node \
  --hostname scylladb-node \
  --network scylla-mariadb-network \
  -p 9042:9042 \
  -p 9160:9160 \
  -p 10000:10000 \
  scylladb/scylla:5.2 \
  --smp 1 \
  --memory 2G
```

**Ports exposed:**
- `9042`: Native transport (CQL)
- `9160`: Thrift client API
- `10000`: REST API

Wait for ScyllaDB to be ready:

```bash
# Check logs
docker logs -f scylladb-node

# Or check with nodetool
docker exec -it scylladb-node nodetool status

# Or test with cqlsh
docker exec -it scylladb-node cqlsh
```

#### Step 3: Build MariaDB Container with Plugin

Create a Dockerfile for MariaDB:

```bash
# This is already provided in the repository as Dockerfile
# Build the image
docker build -t mariadb-scylla:latest .
```

#### Step 4: Start MariaDB Container

```bash
docker run -d \
  --name mariadb-scylla \
  --hostname mariadb-scylla \
  --network scylla-mariadb-network \
  -p 3306:3306 \
  -e MYSQL_ROOT_PASSWORD=rootpassword \
  -e MYSQL_DATABASE=testdb \
  mariadb-scylla:latest
```

**Ports exposed:**
- `3306`: MySQL/MariaDB protocol

## Testing the Setup

### 1. Verify ScyllaDB is Running

```bash
# Connect to ScyllaDB container
docker exec -it scylladb-node cqlsh

# In cqlsh, run:
# CREATE KEYSPACE demo WITH replication = {'class': 'SimpleStrategy', 'replication_factor': 1};
# USE demo;
# CREATE TABLE users (id int PRIMARY KEY, name text, email text);
# INSERT INTO users (id, name, email) VALUES (1, 'Alice', 'alice@example.com');
# SELECT * FROM users;
# exit
```

### 2. Verify MariaDB Storage Engine

```bash
# Connect to MariaDB container
docker exec -it mariadb-scylla mariadb -u root -prootpassword

# In MariaDB prompt, run:
# SHOW ENGINES;
# -- Should show SCYLLA in the list
# 
# SHOW PLUGINS;
# -- Should show scylla plugin
```

### 3. Create a ScyllaDB-backed Table in MariaDB

```sql
-- In MariaDB prompt:

-- Configure connection to ScyllaDB
SET GLOBAL scylla_hosts = 'scylladb-node';
SET GLOBAL scylla_port = 9042;
SET GLOBAL scylla_keyspace = 'demo';

-- Create a table
CREATE TABLE users (
  id INT PRIMARY KEY,
  name VARCHAR(100),
  email VARCHAR(100),
  created_at TIMESTAMP
) ENGINE=SCYLLA;

-- Insert data
INSERT INTO users (id, name, email, created_at) 
VALUES (1, 'Bob', 'bob@example.com', NOW());

INSERT INTO users (id, name, email, created_at)
VALUES (2, 'Charlie', 'charlie@example.com', NOW());

-- Query data
SELECT * FROM users;

-- Update data
UPDATE users SET email = 'bob.smith@example.com' WHERE id = 1;

-- Verify update
SELECT * FROM users WHERE id = 1;

-- Delete data
DELETE FROM users WHERE id = 2;

-- Verify deletion
SELECT * FROM users;
```

### 4. Verify Data in ScyllaDB

```bash
# Connect to ScyllaDB
docker exec -it scylladb-node cqlsh

# In cqlsh:
# USE demo;
# SELECT * FROM users;
# exit
```

## Complete Demo Script

Here's a complete example you can run:

```bash
#!/bin/bash

echo "Starting MariaDB ScyllaDB Storage Engine Demo"
echo "=============================================="

# Start containers
echo "Starting containers..."
docker-compose up -d

# Wait for ScyllaDB
echo "Waiting for ScyllaDB to be ready..."
sleep 30

# Create keyspace in ScyllaDB
echo "Creating ScyllaDB keyspace..."
docker exec -i scylladb-node cqlsh <<EOF
CREATE KEYSPACE IF NOT EXISTS demo WITH replication = {'class': 'SimpleStrategy', 'replication_factor': 1};
USE demo;
DESCRIBE KEYSPACES;
EOF

# Wait for MariaDB
echo "Waiting for MariaDB to be ready..."
sleep 10

# Configure and test MariaDB
echo "Testing MariaDB with ScyllaDB storage engine..."
docker exec -i mariadb-scylla mariadb -u root -prootpassword <<EOF
-- Show storage engines
SHOW ENGINES;

-- Configure ScyllaDB connection
SET GLOBAL scylla_hosts = 'scylladb-node';
SET GLOBAL scylla_keyspace = 'demo';

-- Create table
CREATE DATABASE IF NOT EXISTS demo;
USE demo;

CREATE TABLE IF NOT EXISTS products (
  product_id INT PRIMARY KEY,
  product_name VARCHAR(200),
  price DECIMAL(10,2),
  stock INT
) ENGINE=SCYLLA;

-- Insert sample data
INSERT INTO products VALUES (1, 'Laptop', 999.99, 10);
INSERT INTO products VALUES (2, 'Mouse', 29.99, 50);
INSERT INTO products VALUES (3, 'Keyboard', 79.99, 30);

-- Query data
SELECT * FROM products;

-- Show that data is in ScyllaDB
EOF

echo ""
echo "Verifying data in ScyllaDB..."
docker exec -i scylladb-node cqlsh <<EOF
USE demo;
SELECT * FROM products;
EOF

echo ""
echo "Demo completed successfully!"
echo "=============================================="
echo ""
echo "Access the services:"
echo "  MariaDB:  mariadb -h 127.0.0.1 -P 3306 -u root -prootpassword"
echo "  ScyllaDB: docker exec -it scylladb-node cqlsh"
echo ""
echo "Stop the environment:"
echo "  docker-compose down"
```

Save this as `demo.sh`, make it executable, and run it:

```bash
chmod +x demo.sh
./demo.sh
```

## Accessing Services from Host

### MariaDB from Host

```bash
# Using MariaDB client
mariadb -h 127.0.0.1 -P 3306 -u root -prootpassword

# Using Docker
docker exec -it mariadb-scylla mariadb -u root -prootpassword
```

### ScyllaDB from Host

```bash
# If cqlsh is installed on host
cqlsh 127.0.0.1 9042

# Using Docker
docker exec -it scylladb-node cqlsh

# Using REST API
curl http://localhost:10000/storage_service/scylla_release_version
```

## Troubleshooting

### ScyllaDB Won't Start

```bash
# Check logs
docker logs scylladb-node

# Common issues:
# 1. Insufficient memory - increase --memory parameter
# 2. Port conflicts - check if ports are already in use
# 3. CPU requirements - ScyllaDB requires specific CPU features

# Try with more resources
docker stop scylladb-node
docker rm scylladb-node
docker run -d \
  --name scylladb-node \
  --network scylla-mariadb-network \
  -p 9042:9042 \
  scylladb/scylla:5.2 \
  --smp 2 \
  --memory 4G
```

### MariaDB Can't Connect to ScyllaDB

```bash
# Check if containers are on the same network
docker network inspect scylla-mariadb-network

# Test connectivity from MariaDB container
docker exec -it mariadb-scylla ping scylladb-node

# Check ScyllaDB is listening
docker exec -it scylladb-node netstat -tlnp | grep 9042
```

### Plugin Not Loaded

```bash
# Check if plugin file exists
docker exec -it mariadb-scylla ls -la /usr/lib/mysql/plugin/ | grep scylla

# Check MariaDB error log
docker exec -it mariadb-scylla tail -f /var/lib/mysql/*.err

# Manually load plugin
docker exec -it mariadb-scylla mariadb -u root -prootpassword -e "INSTALL SONAME 'ha_scylla';"
```

### Connection Timeouts

```bash
# Increase ScyllaDB timeout settings
# In MariaDB:
docker exec -it mariadb-scylla mariadb -u root -prootpassword -e \
  "SET GLOBAL net_read_timeout=60; SET GLOBAL net_write_timeout=60;"
```

## Cleanup

### Remove All Containers and Networks

```bash
# Using Docker Compose
docker-compose down -v

# Manual cleanup
docker stop mariadb-scylla scylladb-node
docker rm mariadb-scylla scylladb-node
docker network rm scylla-mariadb-network

# Remove images (optional)
docker rmi mariadb-scylla:latest
docker rmi scylladb/scylla:5.2
```

### Remove All Data Volumes

```bash
# List volumes
docker volume ls

# Remove specific volumes
docker volume rm mariadb-scylla-data scylladb-data

# Remove all unused volumes
docker volume prune
```

## Advanced Configuration

### Custom Configuration Files

Mount custom configuration files:

```yaml
# In docker-compose.yml
services:
  mariadb:
    volumes:
      - ./config/my.cnf:/etc/mysql/my.cnf
  
  scylladb:
    volumes:
      - ./config/scylla.yaml:/etc/scylla/scylla.yaml
```

### Multi-Node ScyllaDB Cluster

For a more realistic setup with multiple ScyllaDB nodes:

```yaml
services:
  scylladb-node1:
    image: scylladb/scylla:5.2
    container_name: scylladb-node1
    command: --seeds=scylladb-node1 --smp 1 --memory 2G
    networks:
      - scylla-mariadb-network
  
  scylladb-node2:
    image: scylladb/scylla:5.2
    container_name: scylladb-node2
    command: --seeds=scylladb-node1 --smp 1 --memory 2G
    networks:
      - scylla-mariadb-network
    depends_on:
      - scylladb-node1
```

### Performance Monitoring

Enable monitoring with Prometheus and Grafana:

```yaml
services:
  prometheus:
    image: prom/prometheus
    ports:
      - "9090:9090"
    volumes:
      - ./prometheus.yml:/etc/prometheus/prometheus.yml
    networks:
      - scylla-mariadb-network
  
  grafana:
    image: grafana/grafana
    ports:
      - "3000:3000"
    networks:
      - scylla-mariadb-network
```

## Next Steps

- Review [README.md](README.md) for detailed usage instructions
- Check [examples/](examples/) for more complex scenarios
- Read [GitHub Copilot Instructions](.github/copilot-instructions.md) for development guidelines

## Support

If you encounter issues with the Docker setup:

1. Check the troubleshooting section above
2. Review container logs: `docker-compose logs`
3. Verify system requirements are met
4. Open an issue on GitHub with detailed logs and system information
