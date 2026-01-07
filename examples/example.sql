-- MariaDB Example SQL Script for ScyllaDB Storage Engine
-- This script demonstrates various operations using the ScyllaDB storage engine

-- ============================================================================
-- SETUP
-- ============================================================================

-- Install the plugin (if not already installed)
-- INSTALL SONAME 'ha_scylla';

-- Configure connection to ScyllaDB
SET GLOBAL scylla_hosts = 'scylladb-node';  -- Use 'localhost' or actual IP if not using Docker
SET GLOBAL scylla_port = 9042;
SET GLOBAL scylla_keyspace = 'demo';

-- Create database for our examples
CREATE DATABASE IF NOT EXISTS scylla_demo;
USE scylla_demo;

-- ============================================================================
-- EXAMPLE 1: Simple Table with Integer Primary Key
-- ============================================================================

DROP TABLE IF EXISTS users;
CREATE TABLE users (
  user_id INT PRIMARY KEY,
  username VARCHAR(50),
  email VARCHAR(100),
  created_at TIMESTAMP,
  is_active BIT
) ENGINE=SCYLLA;

-- Insert sample data
INSERT INTO users VALUES (1, 'alice', 'alice@example.com', NOW(), 1);
INSERT INTO users VALUES (2, 'bob', 'bob@example.com', NOW(), 1);
INSERT INTO users VALUES (3, 'charlie', 'charlie@example.com', NOW(), 0);

-- Query data
SELECT * FROM users;
SELECT * FROM users WHERE user_id = 2;
SELECT username, email FROM users WHERE is_active = 1;

-- Update data
UPDATE users SET email = 'alice.smith@example.com' WHERE user_id = 1;
UPDATE users SET is_active = 1 WHERE user_id = 3;

-- Verify updates
SELECT * FROM users WHERE user_id IN (1, 3);

-- Delete data
DELETE FROM users WHERE user_id = 2;

-- Verify deletion
SELECT * FROM users;

-- ============================================================================
-- EXAMPLE 2: E-commerce Products Table
-- ============================================================================

DROP TABLE IF EXISTS products;
CREATE TABLE products (
  product_id INT PRIMARY KEY,
  product_name VARCHAR(200),
  description TEXT,
  price DECIMAL(10,2),
  stock_quantity INT,
  category VARCHAR(50),
  created_at TIMESTAMP
) ENGINE=SCYLLA;

-- Insert products
INSERT INTO products VALUES 
  (101, 'Laptop Pro 15', 'High-performance laptop with 16GB RAM', 1299.99, 25, 'Electronics', NOW()),
  (102, 'Wireless Mouse', 'Ergonomic wireless mouse with 2.4GHz connectivity', 29.99, 150, 'Electronics', NOW()),
  (103, 'Mechanical Keyboard', 'RGB mechanical keyboard with blue switches', 89.99, 75, 'Electronics', NOW()),
  (104, 'USB-C Hub', '7-in-1 USB-C hub with HDMI and ethernet', 49.99, 100, 'Accessories', NOW()),
  (105, 'Monitor 27"', '4K UHD monitor with IPS panel', 399.99, 30, 'Electronics', NOW());

-- Query products
SELECT * FROM products;
SELECT product_name, price FROM products WHERE product_id = 102;
SELECT * FROM products WHERE category = 'Electronics';
SELECT product_name, price, stock_quantity FROM products WHERE price < 100;

-- Update stock
UPDATE products SET stock_quantity = stock_quantity - 5 WHERE product_id = 101;
UPDATE products SET price = 799.99 WHERE product_id = 105;

-- Query updated data
SELECT product_name, price, stock_quantity FROM products WHERE product_id IN (101, 105);

-- ============================================================================
-- EXAMPLE 3: Composite Primary Key (User Events)
-- ============================================================================

DROP TABLE IF EXISTS user_events;
CREATE TABLE user_events (
  user_id INT,
  event_timestamp BIGINT,
  event_type VARCHAR(50),
  event_data TEXT,
  PRIMARY KEY (user_id, event_timestamp)
) ENGINE=SCYLLA;

-- Insert events
INSERT INTO user_events VALUES (1, UNIX_TIMESTAMP(NOW()) * 1000, 'login', '{"ip": "192.168.1.100"}');
INSERT INTO user_events VALUES (1, UNIX_TIMESTAMP(NOW()) * 1000 + 1000, 'page_view', '{"page": "/home"}');
INSERT INTO user_events VALUES (1, UNIX_TIMESTAMP(NOW()) * 1000 + 2000, 'page_view', '{"page": "/products"}');
INSERT INTO user_events VALUES (2, UNIX_TIMESTAMP(NOW()) * 1000, 'login', '{"ip": "192.168.1.101"}');
INSERT INTO user_events VALUES (2, UNIX_TIMESTAMP(NOW()) * 1000 + 1000, 'logout', '{}');

-- Query events
SELECT * FROM user_events WHERE user_id = 1;
SELECT event_type, event_data FROM user_events WHERE user_id = 2;

-- ============================================================================
-- EXAMPLE 4: Different Data Types
-- ============================================================================

DROP TABLE IF EXISTS data_types_demo;
CREATE TABLE data_types_demo (
  id INT PRIMARY KEY,
  tiny_col TINYINT,
  small_col SMALLINT,
  int_col INT,
  big_col BIGINT,
  float_col FLOAT,
  double_col DOUBLE,
  decimal_col DECIMAL(10,2),
  varchar_col VARCHAR(100),
  text_col TEXT,
  date_col DATE,
  time_col TIME,
  datetime_col DATETIME,
  timestamp_col TIMESTAMP,
  bit_col BIT
) ENGINE=SCYLLA;

-- Insert data with various types
INSERT INTO data_types_demo VALUES (
  1,
  127,                                    -- TINYINT
  32000,                                  -- SMALLINT
  2147483647,                             -- INT
  9223372036854775807,                    -- BIGINT
  3.14,                                   -- FLOAT
  2.718281828,                            -- DOUBLE
  12345.67,                               -- DECIMAL
  'varchar data',                         -- VARCHAR
  'This is a longer text field that can contain much more data', -- TEXT
  '2025-01-06',                           -- DATE
  '14:30:00',                             -- TIME
  '2025-01-06 14:30:00',                  -- DATETIME
  NOW(),                                  -- TIMESTAMP
  1                                       -- BIT
);

-- Insert data with NULL values
INSERT INTO data_types_demo (id, int_col, varchar_col) VALUES (2, 12345, 'partial data');

-- Query data
SELECT * FROM data_types_demo;
SELECT * FROM data_types_demo WHERE id = 1;

-- ============================================================================
-- EXAMPLE 5: Using Table Comments for Connection Parameters
-- ============================================================================

-- Create table with explicit connection parameters in comment
DROP TABLE IF EXISTS remote_data;
CREATE TABLE remote_data (
  id INT PRIMARY KEY,
  data_value VARCHAR(255),
  updated_at TIMESTAMP
) ENGINE=SCYLLA
COMMENT='scylla_hosts=scylladb-node;scylla_keyspace=demo;scylla_table=remote_data';

INSERT INTO remote_data VALUES (1, 'Data from custom config', NOW());
INSERT INTO remote_data VALUES (2, 'Another record', NOW());

SELECT * FROM remote_data;

-- ============================================================================
-- EXAMPLE 6: Bulk Operations
-- ============================================================================

DROP TABLE IF EXISTS bulk_test;
CREATE TABLE bulk_test (
  id INT PRIMARY KEY,
  value VARCHAR(50)
) ENGINE=SCYLLA;

-- Insert multiple rows
INSERT INTO bulk_test VALUES (1, 'value1');
INSERT INTO bulk_test VALUES (2, 'value2');
INSERT INTO bulk_test VALUES (3, 'value3');
INSERT INTO bulk_test VALUES (4, 'value4');
INSERT INTO bulk_test VALUES (5, 'value5');
INSERT INTO bulk_test VALUES (6, 'value6');
INSERT INTO bulk_test VALUES (7, 'value7');
INSERT INTO bulk_test VALUES (8, 'value8');
INSERT INTO bulk_test VALUES (9, 'value9');
INSERT INTO bulk_test VALUES (10, 'value10');

-- Query all
SELECT COUNT(*) FROM bulk_test;
SELECT * FROM bulk_test;

-- ============================================================================
-- EXAMPLE 7: TRUNCATE Operation
-- ============================================================================

-- Create and populate a test table
DROP TABLE IF EXISTS truncate_test;
CREATE TABLE truncate_test (
  id INT PRIMARY KEY,
  data VARCHAR(100)
) ENGINE=SCYLLA;

INSERT INTO truncate_test VALUES (1, 'test data 1');
INSERT INTO truncate_test VALUES (2, 'test data 2');
INSERT INTO truncate_test VALUES (3, 'test data 3');

-- Verify data
SELECT * FROM truncate_test;

-- Truncate table
TRUNCATE TABLE truncate_test;

-- Verify table is empty
SELECT COUNT(*) FROM truncate_test;

-- ============================================================================
-- EXAMPLE 8: Working with NULL Values
-- ============================================================================

DROP TABLE IF EXISTS null_test;
CREATE TABLE null_test (
  id INT PRIMARY KEY,
  required_field VARCHAR(100),
  optional_field VARCHAR(100),
  nullable_int INT,
  nullable_date DATE
) ENGINE=SCYLLA;

-- Insert with NULL values
INSERT INTO null_test VALUES (1, 'required value', NULL, NULL, NULL);
INSERT INTO null_test VALUES (2, 'another required', 'optional value', 42, '2025-01-06');
INSERT INTO null_test VALUES (3, 'third row', NULL, 0, NULL);

-- Query with NULL handling
SELECT * FROM null_test;
SELECT * FROM null_test WHERE optional_field IS NULL;
SELECT * FROM null_test WHERE nullable_int IS NOT NULL;

-- Update with NULL
UPDATE null_test SET optional_field = NULL WHERE id = 2;
UPDATE null_test SET nullable_int = 100 WHERE id = 1;

-- Verify
SELECT * FROM null_test;

-- ============================================================================
-- CLEANUP (Optional - uncomment to clean up)
-- ============================================================================

-- DROP TABLE IF EXISTS users;
-- DROP TABLE IF EXISTS products;
-- DROP TABLE IF EXISTS user_events;
-- DROP TABLE IF EXISTS data_types_demo;
-- DROP TABLE IF EXISTS remote_data;
-- DROP TABLE IF EXISTS bulk_test;
-- DROP TABLE IF EXISTS truncate_test;
-- DROP TABLE IF EXISTS null_test;
-- DROP DATABASE IF EXISTS scylla_demo;

-- ============================================================================
-- NOTES
-- ============================================================================

-- 1. All tables are stored in ScyllaDB, not in MariaDB's local storage
-- 2. ALLOW FILTERING is automatically added to queries when necessary
-- 3. Primary keys are required for all tables
-- 4. Transactions are not supported (ScyllaDB is eventually consistent)
-- 5. Use primary key lookups for best performance
-- 6. Connection parameters can be set globally or per-table via COMMENT

-- ============================================================================
-- VERIFICATION - Check in ScyllaDB
-- ============================================================================

-- From the host or ScyllaDB container, run:
-- docker exec -it scylladb-node cqlsh
-- 
-- In cqlsh:
-- USE demo;
-- DESCRIBE TABLES;
-- SELECT * FROM users;
-- SELECT * FROM products;
