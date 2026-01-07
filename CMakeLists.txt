CMAKE_MINIMUM_REQUIRED(VERSION 3.5)

PROJECT(mariadb_scylla_storage_engine)

# Set C++ standard
SET(CMAKE_CXX_STANDARD 11)
SET(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find MariaDB server headers (from source tree)
FIND_PATH(MARIADB_INCLUDE_DIR
  NAMES handler.h
  PATHS
    /usr/src/mariadb/sql
    /usr/local/src/mariadb/sql
    /opt/mariadb/sql
  NO_DEFAULT_PATH
)

# Also need the include directory
FIND_PATH(MARIADB_BASE_INCLUDE_DIR
  NAMES my_global.h mysql_version.h
  PATHS
    /usr/include/mariadb/server
    /usr/include/mysql/server
    /usr/src/mariadb/include
    /usr/local/src/mariadb/include
    /opt/mariadb/include
  NO_DEFAULT_PATH
)

# Also need the source tree include directory for headers like my_bitmap.h
FIND_PATH(MARIADB_SOURCE_INCLUDE_DIR
  NAMES my_bitmap.h
  PATHS
    /usr/src/mariadb/include
    /usr/local/src/mariadb/include
    /opt/mariadb/include
  NO_DEFAULT_PATH
)

FIND_PATH(MARIADB_PLUGIN_DIR
  NAMES ha_example.so ha_example.dll
  PATHS
    /usr/lib/mysql/plugin
    /usr/lib64/mysql/plugin
    /usr/local/lib/mysql/plugin
    /usr/local/lib64/mysql/plugin
    /opt/mariadb/lib/plugin
  NO_DEFAULT_PATH
)

# Find ScyllaDB cpp-rs-driver
# Note: cpp-rs-driver provides a cassandra.h compatible API
FIND_PATH(CASSANDRA_INCLUDE_DIR
  NAMES cassandra.h
  PATHS
    /usr/include
    /usr/local/include
    /opt/scylladb-driver/include
  DOC "ScyllaDB cpp-rs-driver include directory"
)

FIND_LIBRARY(CASSANDRA_LIBRARY
  NAMES scylla-cpp-driver cassandra
  PATHS
    /usr/lib
    /usr/lib64
    /usr/local/lib
    /usr/local/lib64
    /opt/scylladb-driver/lib
  DOC "ScyllaDB cpp-rs-driver library"
)

IF(NOT MARIADB_INCLUDE_DIR)
  MESSAGE(FATAL_ERROR "MariaDB sql/ directory not found. Please install MariaDB source headers.")
ENDIF()

IF(NOT MARIADB_BASE_INCLUDE_DIR)
  MESSAGE(FATAL_ERROR "MariaDB include/ directory not found. Please install MariaDB source headers.")
ENDIF()

IF(NOT MARIADB_SOURCE_INCLUDE_DIR)
  MESSAGE(FATAL_ERROR "MariaDB source include/ directory not found. Please install MariaDB source tree.")
ENDIF()

IF(NOT CASSANDRA_INCLUDE_DIR)
  MESSAGE(FATAL_ERROR "Cassandra/ScyllaDB driver include directory not found. Please install cpp-rs-driver.")
ENDIF()

IF(NOT CASSANDRA_LIBRARY)
  MESSAGE(FATAL_ERROR "Cassandra/ScyllaDB driver library not found. Please install cpp-rs-driver.")
ENDIF()

MESSAGE(STATUS "MariaDB sql dir: ${MARIADB_INCLUDE_DIR}")
MESSAGE(STATUS "MariaDB include dir: ${MARIADB_BASE_INCLUDE_DIR}")
MESSAGE(STATUS "MariaDB source include dir: ${MARIADB_SOURCE_INCLUDE_DIR}")
MESSAGE(STATUS "Cassandra include dir: ${CASSANDRA_INCLUDE_DIR}")
MESSAGE(STATUS "Cassandra library: ${CASSANDRA_LIBRARY}")

# Include directories
INCLUDE_DIRECTORIES(
  ${MARIADB_INCLUDE_DIR}
  ${MARIADB_BASE_INCLUDE_DIR}
  ${MARIADB_SOURCE_INCLUDE_DIR}
  ${CASSANDRA_INCLUDE_DIR}
)

# Source files
SET(SCYLLA_SOURCES
  ha_scylla.cc
  scylla_connection.cc
  scylla_types.cc
  scylla_query.cc
)

# Build shared library
ADD_LIBRARY(ha_scylla SHARED ${SCYLLA_SOURCES})

# Link libraries
TARGET_LINK_LIBRARIES(ha_scylla ${CASSANDRA_LIBRARY})

# Set output properties
SET_TARGET_PROPERTIES(ha_scylla PROPERTIES
  PREFIX ""
  SUFFIX ".so"
)

# Installation
IF(MARIADB_PLUGIN_DIR)
  INSTALL(TARGETS ha_scylla
    LIBRARY DESTINATION ${MARIADB_PLUGIN_DIR}
  )
  MESSAGE(STATUS "Will install to: ${MARIADB_PLUGIN_DIR}")
ELSE()
  MESSAGE(WARNING "MariaDB plugin directory not found. Use 'make install DESTDIR=/path/to/plugin/dir' or set -DMARIADB_PLUGIN_DIR")
  INSTALL(TARGETS ha_scylla
    LIBRARY DESTINATION lib/mysql/plugin
  )
ENDIF()

# Add documentation files
INSTALL(FILES
  README.md
  DOCKER-DEMO.md
  DESTINATION share/doc/mariadb-scylla-storage-engine
)

# Print build information
MESSAGE(STATUS "")
MESSAGE(STATUS "================================")
MESSAGE(STATUS "MariaDB ScyllaDB Storage Engine")
MESSAGE(STATUS "================================")
MESSAGE(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
MESSAGE(STATUS "C++ compiler: ${CMAKE_CXX_COMPILER}")
MESSAGE(STATUS "C++ flags: ${CMAKE_CXX_FLAGS}")
MESSAGE(STATUS "")
MESSAGE(STATUS "To build:")
MESSAGE(STATUS "  mkdir build && cd build")
MESSAGE(STATUS "  cmake ..")
MESSAGE(STATUS "  make")
MESSAGE(STATUS "")
MESSAGE(STATUS "To install:")
MESSAGE(STATUS "  sudo make install")
MESSAGE(STATUS "")
