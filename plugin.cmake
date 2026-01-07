# MariaDB ScyllaDB Storage Engine Plugin Configuration
# This file can be used for additional plugin-specific build configurations

# Plugin metadata
SET(PLUGIN_NAME "scylla")
SET(PLUGIN_AUTHOR "MariaDB Corporation")
SET(PLUGIN_DESCRIPTION "ScyllaDB storage engine for MariaDB")
SET(PLUGIN_VERSION "1.0")
SET(PLUGIN_LICENSE "GPL")
SET(PLUGIN_MATURITY "Experimental")

# Additional compiler flags for the plugin
IF(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-unused-parameter")
ENDIF()

# Enable position independent code for shared library
SET(CMAKE_POSITION_INDEPENDENT_CODE ON)
