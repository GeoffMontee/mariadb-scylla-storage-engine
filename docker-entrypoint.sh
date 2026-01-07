#!/bin/bash
set -eo pipefail
shopt -s nullglob

# Logging functions
mysql_log() {
    echo "[Entrypoint] $@"
}

mysql_error() {
    echo "[Entrypoint] ERROR: $@" >&2
}

# Check if data directory is empty (first run)
if [ ! -d "/var/lib/mysql/mysql" ]; then
    mysql_log "Initializing database..."
    
    # Initialize MariaDB data directory
    mysql_install_db --user=mysql --datadir=/var/lib/mysql
    
    mysql_log "Database initialized"
    
    # Start temporary server for setup
    mysql_log "Starting temporary server..."
    mariadbd --user=mysql --datadir=/var/lib/mysql --skip-networking --socket=/tmp/mysql_init.sock &
    pid="$!"
    
    # Wait for server to be ready
    for i in {30..0}; do
        if mysqladmin --socket=/tmp/mysql_init.sock ping &>/dev/null; then
            break
        fi
        sleep 1
    done
    
    if [ "$i" = 0 ]; then
        mysql_error "Unable to start server."
        exit 1
    fi
    
    mysql_log "Temporary server started"
    
    # Set root password if provided
    if [ -n "$MYSQL_ROOT_PASSWORD" ]; then
        mysql_log "Setting root password..."
        mysql --socket=/tmp/mysql_init.sock <<-EOSQL
            SET @@SESSION.SQL_LOG_BIN=0;
            DELETE FROM mysql.user WHERE user NOT IN ('mysql.sys', 'mariadb.sys', 'root') OR host NOT IN ('localhost');
            SET PASSWORD FOR 'root'@'localhost'=PASSWORD('${MYSQL_ROOT_PASSWORD}');
            GRANT ALL ON *.* TO 'root'@'%' IDENTIFIED BY '${MYSQL_ROOT_PASSWORD}' WITH GRANT OPTION;
            FLUSH PRIVILEGES;
EOSQL
    fi
    
    # Create database if specified
    if [ -n "$MYSQL_DATABASE" ]; then
        mysql_log "Creating database ${MYSQL_DATABASE}..."
        mysql --socket=/tmp/mysql_init.sock <<-EOSQL
            CREATE DATABASE IF NOT EXISTS \`${MYSQL_DATABASE}\`;
EOSQL
    fi
    
    # Create user if specified
    if [ -n "$MYSQL_USER" ] && [ -n "$MYSQL_PASSWORD" ]; then
        mysql_log "Creating user ${MYSQL_USER}..."
        mysql --socket=/tmp/mysql_init.sock <<-EOSQL
            CREATE USER '${MYSQL_USER}'@'%' IDENTIFIED BY '${MYSQL_PASSWORD}';
            GRANT ALL ON \`${MYSQL_DATABASE}\`.* TO '${MYSQL_USER}'@'%';
            FLUSH PRIVILEGES;
EOSQL
    fi
    
    # Run initialization scripts
    for f in /docker-entrypoint-initdb.d/*; do
        case "$f" in
            *.sh)
                mysql_log "Running $f"
                . "$f"
                ;;
            *.sql)
                mysql_log "Running $f"
                mysql --socket=/tmp/mysql_init.sock < "$f"
                ;;
            *)
                mysql_log "Ignoring $f"
                ;;
        esac
    done
    
    # Shutdown temporary server
    mysql_log "Stopping temporary server..."
    if ! mysqladmin --socket=/tmp/mysql_init.sock shutdown; then
        mysql_error "Unable to shutdown server."
        exit 1
    fi
    wait "$pid"
    mysql_log "Temporary server stopped"
    mysql_log "MariaDB init process done. Ready for start up."
fi

# Start MariaDB server
exec mariadbd --user=mysql --datadir=/var/lib/mysql "$@"
