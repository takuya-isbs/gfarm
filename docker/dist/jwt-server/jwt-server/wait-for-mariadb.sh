#!/bin/sh
set -eu

MARIADB_ADMIN_COMMAND=${MARIADB_ADMIN_COMMAND:-mariadb-admin}
MARIADB_ADMIN_HOST=${MARIADB_ADMIN_HOST:-}
MARIADB_ADMIN_CONNECT_TIMEOUT_SECONDS=${MARIADB_ADMIN_CONNECT_TIMEOUT_SECONDS:-2}
MYSQL_WAIT_INTERVAL_SECONDS=${MYSQL_WAIT_INTERVAL_SECONDS:-2}
MYSQL_WAIT_TIMEOUT_SECONDS=${MYSQL_WAIT_TIMEOUT_SECONDS:-300}

case "$MYSQL_WAIT_INTERVAL_SECONDS" in
    ''|*[!0-9]*|0)
        echo "MYSQL_WAIT_INTERVAL_SECONDS must be a positive integer." >&2
        exit 2
        ;;
esac

case "$MYSQL_WAIT_TIMEOUT_SECONDS" in
    ''|*[!0-9]*)
        echo "MYSQL_WAIT_TIMEOUT_SECONDS must be a non-negative integer." >&2
        exit 2
        ;;
esac

case "$MARIADB_ADMIN_CONNECT_TIMEOUT_SECONDS" in
    ''|*[!0-9]*|0)
        echo "MARIADB_ADMIN_CONNECT_TIMEOUT_SECONDS must be a positive integer." >&2
        exit 2
        ;;
esac

mariadb_admin_ping() {
    if [ -n "$MARIADB_ADMIN_HOST" ]; then
        "$MARIADB_ADMIN_COMMAND" \
            --host="$MARIADB_ADMIN_HOST" \
            --protocol=tcp \
            --connect-timeout="$MARIADB_ADMIN_CONNECT_TIMEOUT_SECONDS" \
            ping --silent
    else
        "$MARIADB_ADMIN_COMMAND" ping --silent
    fi
}

elapsed_seconds=0
printf '[WAIT] Waiting for MariaDB readiness...'
while ! mariadb_admin_ping >/dev/null 2>&1; do
    if [ "$elapsed_seconds" -ge "$MYSQL_WAIT_TIMEOUT_SECONDS" ]; then
        echo
        echo '[ERROR] Timed out waiting for MariaDB.' >&2
        exit 1
    fi

    sleep "$MYSQL_WAIT_INTERVAL_SECONDS"
    elapsed_seconds=$((elapsed_seconds + MYSQL_WAIT_INTERVAL_SECONDS))
done

echo ' ready'
