FROM mariadb:latest

# Extract MariaDB version first for version-specific installations
RUN MARIADB_VERSION=$(/usr/sbin/mariadbd --version | grep -oP '\d+\.\d+\.\d+' | head -1) && \
    echo "Detected MariaDB version: $MARIADB_VERSION" && \
    echo "MARIADB_VERSION=$MARIADB_VERSION" > /etc/mariadb_version.env

# Install build dependencies with version-matched libmariadbd-dev
RUN . /etc/mariadb_version.env && \
    apt-get update && \
    apt-get install -y \
        build-essential \
        cmake \
        git \
        wget \
        curl \
        pkg-config \
        libssl-dev \
        libuv1-dev \
        libclang-dev && \
    apt-get install -y libmariadbd-dev=$(apt-cache madison libmariadbd-dev | grep -F "$MARIADB_VERSION" | head -1 | awk '{print $3}') || \
    apt-get install -y libmariadbd-dev && \
    rm -rf /var/lib/apt/lists/*

# Get MariaDB source headers (needed for plugin development)
# Clone the exact version tag to ensure compatibility
RUN . /etc/mariadb_version.env && \
    echo "Downloading MariaDB $MARIADB_VERSION source headers..." && \
    cd /tmp && \
    git clone --depth 1 --branch mariadb-$MARIADB_VERSION https://github.com/MariaDB/server.git mariadb-src && \
    mkdir -p /usr/src/mariadb && \
    cp -r /tmp/mariadb-src/include /usr/src/mariadb/ && \
    cp -r /tmp/mariadb-src/sql /usr/src/mariadb/ && \
    cp /tmp/mariadb-src/include/probes_mysql_nodtrace.h.in /usr/src/mariadb/include/probes_mysql_nodtrace.h && \
    echo "MariaDB source version: $(cat /tmp/mariadb-src/VERSION 2>/dev/null || echo 'unknown')" && \
    rm -rf /tmp/mariadb-src

# Install Rust (required for cpp-rs-driver)
RUN curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y
ENV PATH="/root/.cargo/bin:${PATH}"

# Build and install ScyllaDB cpp-rs-driver from source
RUN git clone https://github.com/scylladb/cpp-rs-driver.git /tmp/cpp-rs-driver \
    && cd /tmp/cpp-rs-driver \
    && mkdir build && cd build \
    && cmake .. \
    && make -j$(nproc) \
    && make install \
    && ldconfig \
    && cd / && rm -rf /tmp/cpp-rs-driver

# Copy source code
COPY . /tmp/mariadb-scylla-storage-engine/

# Build the plugin
WORKDIR /tmp/mariadb-scylla-storage-engine
RUN mkdir -p build && cd build && \
    cmake .. && \
    make && \
    mkdir -p /usr/lib/mysql/plugin && \
    cp ha_scylla.so /usr/lib/mysql/plugin/ && \
    chmod 755 /usr/lib/mysql/plugin/ha_scylla.so

# Clean up build files
RUN rm -rf /tmp/mariadb-scylla-storage-engine

# Create initialization script
RUN echo "INSTALL SONAME 'ha_scylla';" > /docker-entrypoint-initdb.d/00-install-scylla-plugin.sql

# Expose MariaDB port
EXPOSE 3306

# Use the default MariaDB entrypoint
CMD ["mariadbd"]
