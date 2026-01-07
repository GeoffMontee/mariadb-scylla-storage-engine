#!/bin/bash

# Quick Start Script for MariaDB ScyllaDB Storage Engine Demo
# This script sets up and runs a complete demo environment

set -e  # Exit on error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}"
echo "=============================================="
echo "MariaDB ScyllaDB Storage Engine Quick Start"
echo "=============================================="
echo -e "${NC}"

# Check prerequisites
echo -e "${YELLOW}Checking prerequisites...${NC}"

if ! command -v docker &> /dev/null; then
    echo -e "${RED}Error: Docker is not installed${NC}"
    echo "Please install Docker: https://docs.docker.com/get-docker/"
    exit 1
fi

if ! command -v docker-compose &> /dev/null; then
    echo -e "${RED}Error: Docker Compose is not installed${NC}"
    echo "Please install Docker Compose: https://docs.docker.com/compose/install/"
    exit 1
fi

echo -e "${GREEN}✓ Prerequisites OK${NC}"

# Start containers
echo ""
echo -e "${YELLOW}Starting Docker containers...${NC}"
docker-compose up -d

# Wait for ScyllaDB
echo ""
echo -e "${YELLOW}Waiting for ScyllaDB to be ready (this may take 30-60 seconds)...${NC}"
sleep 30

# Check ScyllaDB status
echo -e "${YELLOW}Checking ScyllaDB status...${NC}"
for i in {1..10}; do
    if docker exec scylladb-node nodetool status &> /dev/null; then
        echo -e "${GREEN}✓ ScyllaDB is ready${NC}"
        break
    fi
    if [ $i -eq 10 ]; then
        echo -e "${RED}Error: ScyllaDB did not start properly${NC}"
        echo "Check logs with: docker-compose logs scylladb"
        exit 1
    fi
    echo "Still waiting... ($i/10)"
    sleep 5
done

# Create keyspace in ScyllaDB
echo ""
echo -e "${YELLOW}Creating demo keyspace in ScyllaDB...${NC}"
docker exec -i scylladb-node cqlsh <<EOF
CREATE KEYSPACE IF NOT EXISTS demo 
WITH replication = {'class': 'SimpleStrategy', 'replication_factor': 1};
USE demo;
DESCRIBE KEYSPACES;
EOF

# Wait for MariaDB
echo ""
echo -e "${YELLOW}Waiting for MariaDB to be ready...${NC}"
sleep 10

for i in {1..10}; do
    if docker exec mariadb-scylla mysqladmin ping -h localhost -u root -prootpassword --silent &> /dev/null; then
        echo -e "${GREEN}✓ MariaDB is ready${NC}"
        break
    fi
    if [ $i -eq 10 ]; then
        echo -e "${RED}Error: MariaDB did not start properly${NC}"
        echo "Check logs with: docker-compose logs mariadb"
        exit 1
    fi
    echo "Still waiting... ($i/10)"
    sleep 3
done

# Verify storage engine is loaded
echo ""
echo -e "${YELLOW}Verifying ScyllaDB storage engine is loaded...${NC}"
if docker exec mariadb-scylla mysql -u root -prootpassword -e "SHOW ENGINES" | grep -q "SCYLLA"; then
    echo -e "${GREEN}✓ ScyllaDB storage engine is loaded${NC}"
else
    echo -e "${RED}Warning: ScyllaDB storage engine may not be loaded properly${NC}"
    echo "Check plugin status with: docker exec -it mariadb-scylla mysql -u root -prootpassword -e \"SHOW PLUGINS\""
fi

# Run demo script
echo ""
echo -e "${YELLOW}Running demo script...${NC}"
docker exec -i mariadb-scylla mysql -u root -prootpassword <<EOF
-- Configure connection to ScyllaDB
SET GLOBAL scylla_hosts = 'scylladb-node';
SET GLOBAL scylla_keyspace = 'demo';

-- Create database
CREATE DATABASE IF NOT EXISTS scylla_demo;
USE scylla_demo;

-- Create a sample table
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

-- Query the data
SELECT 'Products in MariaDB (backed by ScyllaDB):' AS info;
SELECT * FROM products;

-- Show configuration
SELECT 'Storage Engine Configuration:' AS info;
SHOW GLOBAL VARIABLES LIKE 'scylla%';
EOF

# Verify data in ScyllaDB
echo ""
echo -e "${YELLOW}Verifying data in ScyllaDB...${NC}"
docker exec -i scylladb-node cqlsh <<EOF
USE demo;
SELECT * FROM products;
EOF

# Success message
echo ""
echo -e "${GREEN}"
echo "=============================================="
echo "Demo Setup Complete!"
echo "=============================================="
echo -e "${NC}"

echo ""
echo -e "${GREEN}Services are running:${NC}"
echo "  • MariaDB:  localhost:3306 (user: root, password: rootpassword)"
echo "  • ScyllaDB: localhost:9042"
echo ""

echo -e "${GREEN}Access the services:${NC}"
echo "  • MariaDB CLI:"
echo "    docker exec -it mariadb-scylla mysql -u root -prootpassword"
echo ""
echo "  • ScyllaDB CLI:"
echo "    docker exec -it scylladb-node cqlsh"
echo ""
echo "  • From host (if clients installed):"
echo "    mysql -h 127.0.0.1 -P 3306 -u root -prootpassword"
echo "    cqlsh 127.0.0.1 9042"
echo ""

echo -e "${GREEN}Try more examples:${NC}"
echo "  • MariaDB: docker exec -i mariadb-scylla mysql -u root -prootpassword < examples/example.sql"
echo "  • ScyllaDB: docker exec -i scylladb-node cqlsh < examples/example.cql"
echo ""

echo -e "${GREEN}View logs:${NC}"
echo "  • docker-compose logs -f mariadb"
echo "  • docker-compose logs -f scylladb"
echo ""

echo -e "${GREEN}Stop the demo:${NC}"
echo "  • docker-compose down"
echo "  • docker-compose down -v  (removes data volumes)"
echo ""

echo -e "${YELLOW}Read the documentation:${NC}"
echo "  • README.md - Full documentation"
echo "  • DOCKER-DEMO.md - Detailed Docker setup guide"
echo "  • examples/ - More example scripts"
echo ""

echo -e "${GREEN}Happy coding! 🚀${NC}"
