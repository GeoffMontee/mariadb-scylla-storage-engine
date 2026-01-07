-- ScyllaDB CQL Examples
-- These examples show how to interact directly with ScyllaDB
-- and verify data created by the MariaDB storage engine

-- ============================================================================
-- CONNECTING TO SCYLLADB
-- ============================================================================

-- From Docker:
-- docker exec -it scylladb-node cqlsh

-- From host (if cqlsh is installed):
-- cqlsh localhost 9042

-- ============================================================================
-- SETUP - Create Keyspace
-- ============================================================================

-- Create keyspace with SimpleStrategy (for demo/dev)
CREATE KEYSPACE IF NOT EXISTS demo 
WITH replication = {'class': 'SimpleStrategy', 'replication_factor': 1};

-- For production with multiple nodes:
-- CREATE KEYSPACE IF NOT EXISTS demo 
-- WITH replication = {'class': 'NetworkTopologyStrategy', 'datacenter1': 3};

-- Use the keyspace
USE demo;

-- ============================================================================
-- VIEWING SCHEMA
-- ============================================================================

-- List all keyspaces
DESCRIBE KEYSPACES;

-- Describe specific keyspace
DESCRIBE KEYSPACE demo;

-- List tables in current keyspace
DESCRIBE TABLES;

-- Describe specific table (created by MariaDB)
DESCRIBE TABLE users;

-- ============================================================================
-- QUERYING DATA
-- ============================================================================

-- Select all from users table (created by MariaDB)
SELECT * FROM users;

-- Select with WHERE clause on primary key
SELECT * FROM users WHERE user_id = 1;

-- Select with ALLOW FILTERING (for non-primary-key queries)
SELECT * FROM users WHERE email = 'alice@example.com' ALLOW FILTERING;

-- Select from products table
SELECT * FROM products;
SELECT * FROM products WHERE product_id = 101;
SELECT * FROM products WHERE category = 'Electronics' ALLOW FILTERING;

-- ============================================================================
-- DIRECT DATA MANIPULATION IN SCYLLADB
-- ============================================================================

-- Insert data directly in ScyllaDB
INSERT INTO users (user_id, username, email, created_at, is_active) 
VALUES (100, 'scylla_user', 'scylla@example.com', toTimestamp(now()), true);

-- Update data
UPDATE users SET email = 'updated@example.com' WHERE user_id = 100;

-- Delete data
DELETE FROM users WHERE user_id = 100;

-- ============================================================================
-- CREATING TABLES DIRECTLY IN SCYLLADB
-- ============================================================================

-- Create a table that will be accessible from MariaDB
CREATE TABLE IF NOT EXISTS customers (
  customer_id int PRIMARY KEY,
  first_name text,
  last_name text,
  email text,
  phone text,
  created_at timestamp
);

-- Insert sample data
INSERT INTO customers (customer_id, first_name, last_name, email, phone, created_at)
VALUES (1, 'John', 'Doe', 'john.doe@example.com', '+1234567890', toTimestamp(now()));

INSERT INTO customers (customer_id, first_name, last_name, email, phone, created_at)
VALUES (2, 'Jane', 'Smith', 'jane.smith@example.com', '+1987654321', toTimestamp(now()));

-- Query the data
SELECT * FROM customers;

-- This table can now be accessed from MariaDB:
-- CREATE TABLE customers (
--   customer_id INT PRIMARY KEY,
--   first_name VARCHAR(100),
--   last_name VARCHAR(100),
--   email VARCHAR(100),
--   phone VARCHAR(20),
--   created_at TIMESTAMP
-- ) ENGINE=SCYLLA
-- COMMENT='scylla_hosts=scylladb-node;scylla_keyspace=demo;scylla_table=customers';

-- ============================================================================
-- COMPOSITE PRIMARY KEY EXAMPLE
-- ============================================================================

-- Create table with composite primary key
CREATE TABLE IF NOT EXISTS time_series_data (
  sensor_id int,
  timestamp bigint,
  temperature double,
  humidity double,
  PRIMARY KEY (sensor_id, timestamp)
) WITH CLUSTERING ORDER BY (timestamp DESC);

-- Insert time series data
INSERT INTO time_series_data (sensor_id, timestamp, temperature, humidity)
VALUES (1, 1704556800000, 22.5, 45.0);

INSERT INTO time_series_data (sensor_id, timestamp, temperature, humidity)
VALUES (1, 1704556860000, 22.7, 46.0);

INSERT INTO time_series_data (sensor_id, timestamp, temperature, humidity)
VALUES (1, 1704556920000, 22.6, 45.5);

INSERT INTO time_series_data (sensor_id, timestamp, temperature, humidity)
VALUES (2, 1704556800000, 21.8, 50.0);

-- Query time series data
SELECT * FROM time_series_data WHERE sensor_id = 1;
SELECT * FROM time_series_data WHERE sensor_id = 1 AND timestamp > 1704556800000;

-- ============================================================================
-- COLLECTION TYPES (Not directly supported by MariaDB storage engine yet)
-- ============================================================================

-- Example with SET
CREATE TABLE IF NOT EXISTS user_tags (
  user_id int PRIMARY KEY,
  tags set<text>
);

INSERT INTO user_tags (user_id, tags) 
VALUES (1, {'developer', 'python', 'scylladb'});

-- Example with LIST
CREATE TABLE IF NOT EXISTS user_activities (
  user_id int PRIMARY KEY,
  activities list<text>
);

INSERT INTO user_activities (user_id, activities)
VALUES (1, ['login', 'view_page', 'logout']);

-- Example with MAP
CREATE TABLE IF NOT EXISTS user_preferences (
  user_id int PRIMARY KEY,
  preferences map<text, text>
);

INSERT INTO user_preferences (user_id, preferences)
VALUES (1, {'theme': 'dark', 'language': 'en', 'timezone': 'UTC'});

-- Query collections
SELECT * FROM user_tags;
SELECT * FROM user_activities;
SELECT * FROM user_preferences;

-- ============================================================================
-- BATCH OPERATIONS
-- ============================================================================

-- Batch insert
BEGIN BATCH
  INSERT INTO users (user_id, username, email) VALUES (201, 'batch_user1', 'batch1@example.com');
  INSERT INTO users (user_id, username, email) VALUES (202, 'batch_user2', 'batch2@example.com');
  INSERT INTO users (user_id, username, email) VALUES (203, 'batch_user3', 'batch3@example.com');
APPLY BATCH;

-- Verify batch insert
SELECT * FROM users WHERE user_id IN (201, 202, 203) ALLOW FILTERING;

-- ============================================================================
-- AGGREGATION QUERIES
-- ============================================================================

-- Count rows
SELECT COUNT(*) FROM users;
SELECT COUNT(*) FROM products;

-- Note: AVG, SUM, MIN, MAX are limited to columns in WHERE clause
-- SELECT AVG(price) FROM products;  -- May require ALLOW FILTERING

-- ============================================================================
-- TTL (Time To Live) - Auto-expiring Data
-- ============================================================================

-- Create table with TTL support
CREATE TABLE IF NOT EXISTS session_data (
  session_id text PRIMARY KEY,
  user_id int,
  data text,
  created_at timestamp
);

-- Insert with TTL (expires after 3600 seconds / 1 hour)
INSERT INTO session_data (session_id, user_id, data, created_at)
VALUES ('sess_12345', 1, '{"cart": []}', toTimestamp(now()))
USING TTL 3600;

-- Check TTL
SELECT session_id, TTL(data) FROM session_data WHERE session_id = 'sess_12345';

-- ============================================================================
-- SECONDARY INDEXES
-- ============================================================================

-- Create secondary index on email column
CREATE INDEX IF NOT EXISTS users_email_idx ON users (email);

-- Now can query by email without ALLOW FILTERING
SELECT * FROM users WHERE email = 'alice@example.com';

-- Drop index
-- DROP INDEX IF EXISTS users_email_idx;

-- ============================================================================
-- MATERIALIZED VIEWS
-- ============================================================================

-- Create materialized view for querying products by category
CREATE MATERIALIZED VIEW IF NOT EXISTS products_by_category AS
  SELECT * FROM products
  WHERE category IS NOT NULL AND product_id IS NOT NULL
  PRIMARY KEY (category, product_id);

-- Query the materialized view
SELECT * FROM products_by_category WHERE category = 'Electronics';

-- Drop materialized view
-- DROP MATERIALIZED VIEW IF EXISTS products_by_category;

-- ============================================================================
-- TABLE STATISTICS
-- ============================================================================

-- Note: These commands work in nodetool, not cqlsh
-- docker exec -it scylladb-node nodetool tablestats demo

-- ============================================================================
-- TRUNCATE TABLE
-- ============================================================================

-- Truncate table (removes all data)
-- TRUNCATE TABLE session_data;

-- ============================================================================
-- DROP OBJECTS
-- ============================================================================

-- Drop table
-- DROP TABLE IF EXISTS session_data;

-- Drop keyspace (removes all tables in it)
-- DROP KEYSPACE IF EXISTS demo;

-- ============================================================================
-- SYSTEM QUERIES
-- ============================================================================

-- Show ScyllaDB version
SELECT release_version FROM system.local;

-- Show cluster information
SELECT * FROM system.local;

-- Show all peers in the cluster
SELECT * FROM system.peers;

-- ============================================================================
-- CONSISTENCY LEVELS
-- ============================================================================

-- Set consistency level for current session
-- CONSISTENCY QUORUM;
-- CONSISTENCY ALL;
-- CONSISTENCY ONE;
-- CONSISTENCY LOCAL_QUORUM;

-- Check current consistency level
-- CONSISTENCY;

-- ============================================================================
-- TRACING
-- ============================================================================

-- Enable tracing for next query
-- TRACING ON;
-- SELECT * FROM users WHERE user_id = 1;
-- TRACING OFF;

-- ============================================================================
-- NOTES
-- ============================================================================

-- 1. ScyllaDB is compatible with Apache Cassandra CQL
-- 2. Primary keys must be specified in WHERE clauses or use ALLOW FILTERING
-- 3. ScyllaDB supports eventual consistency by default
-- 4. Use appropriate replication factor for production
-- 5. Partition keys should be chosen carefully for good data distribution
-- 6. Clustering keys determine sort order within a partition

-- ============================================================================
-- USEFUL NODETOOL COMMANDS (from shell, not cqlsh)
-- ============================================================================

-- docker exec -it scylladb-node nodetool status
-- docker exec -it scylladb-node nodetool info
-- docker exec -it scylladb-node nodetool tablestats demo
-- docker exec -it scylladb-node nodetool cfstats demo.users
-- docker exec -it scylladb-node nodetool flush demo users
-- docker exec -it scylladb-node nodetool repair demo users
