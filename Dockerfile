FROM ubuntu:24.04

# MariaDB version to build (configurable via --build-arg)
ARG MARIADB_VERSION=12.1.2

ENV DEBIAN_FRONTEND=noninteractive

# Install MariaDB build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    wget \
    curl \
    pkg-config \
    bison \
    libncurses-dev \
    libssl-dev \
    libreadline-dev \
    zlib1g-dev \
    libxml2-dev \
    libevent-dev \
    libpcre2-dev \
    liblz4-dev \
    libzstd-dev \
    libsnappy-dev \
    libbz2-dev \
    libkrb5-dev \
    libpam0g-dev \
    libaio-dev \
    libjemalloc-dev \
    libnuma-dev \
    libsystemd-dev \
    liburing-dev \
    gnutls-dev \
    libuv1-dev \
    libclang-dev \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# Install Rust (required for cpp-rs-driver)
RUN curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y
ENV PATH="/root/.cargo/bin:${PATH}"

# Build and install ScyllaDB cpp-rs-driver
RUN git clone https://github.com/scylladb/cpp-rs-driver.git /tmp/cpp-rs-driver && \
    cd /tmp/cpp-rs-driver && \
    mkdir build && cd build && \
    cmake .. && \
    make -j$(nproc) && \
    make install && \
    ldconfig && \
    rm -rf /tmp/cpp-rs-driver

# Clone MariaDB source at specific version
RUN echo "Cloning MariaDB version ${MARIADB_VERSION}..." && \
    git clone --depth 1 --branch mariadb-${MARIADB_VERSION} \
    https://github.com/MariaDB/server.git /usr/src/mariadb

# Copy ScyllaDB storage engine into MariaDB source tree
COPY ha_scylla.cc ha_scylla.h \
     scylla_connection.cc scylla_connection.h \
     scylla_types.cc scylla_types.h \
     scylla_query.cc scylla_query.h \
     plugin.cmake \
     CMakeLists.txt \
     /usr/src/mariadb/storage/scylla/

# Build MariaDB with ScyllaDB storage engine
# Using Debug build type for full debug symbols in MariaDB and plugin
WORKDIR /usr/src/mariadb
RUN mkdir build && cd build && \
    cmake .. \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_INSTALL_PREFIX=/usr \
    -DMYSQL_DATADIR=/var/lib/mysql \
    -DSYSCONFDIR=/etc/mysql \
    -DPLUGIN_SCYLLA=DYNAMIC \
    -DWITH_EMBEDDED_SERVER=OFF \
    -DWITH_UNIT_TESTS=OFF \
    && make -j$(nproc) && \
    make install && \
    cd / && rm -rf /usr/src/mariadb

# Create MariaDB user and directories
RUN useradd -r -s /bin/false mysql && \
    mkdir -p /var/lib/mysql /var/run/mysqld /docker-entrypoint-initdb.d && \
    chown -R mysql:mysql /var/lib/mysql /var/run/mysqld && \
    chmod 1777 /var/run/mysqld

# Create initialization script to install the plugin
RUN echo "INSTALL SONAME 'ha_scylla';" > /docker-entrypoint-initdb.d/00-install-scylla-plugin.sql

# Copy entrypoint script
COPY docker-entrypoint.sh /usr/local/bin/
RUN chmod +x /usr/local/bin/docker-entrypoint.sh

# Expose MariaDB port
EXPOSE 3306

ENTRYPOINT ["docker-entrypoint.sh"]
CMD ["/usr/bin/mariadbd"]
