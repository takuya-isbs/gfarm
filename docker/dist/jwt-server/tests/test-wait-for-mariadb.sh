#!/bin/sh
set -eu

script_dir=$(CDPATH='' cd -- "$(dirname -- "$0")" && pwd)
wait_script="${script_dir}/../jwt-server/wait-for-mariadb.sh"
mock_admin="${script_dir}/mock-bin/mariadb-admin"

if output=$(MARIADB_ADMIN_COMMAND="$mock_admin" \
    MARIADB_ADMIN_MODE=ready \
    MYSQL_WAIT_TIMEOUT_SECONDS=1 \
    "$wait_script" 2>&1); then
    case "$output" in
        *ready*) ;;
        *) echo "Expected readiness output, got: $output" >&2; exit 1 ;;
    esac
else
    echo "Expected MariaDB readiness to succeed, got: $output" >&2
    exit 1
fi

if output=$(MARIADB_ADMIN_COMMAND="$mock_admin" \
    MARIADB_ADMIN_MODE=unavailable \
    MYSQL_WAIT_TIMEOUT_SECONDS=0 \
    "$wait_script" 2>&1); then
    echo "Expected unavailable MariaDB to time out, got: $output" >&2
    exit 1
else
    case "$output" in
        *"Timed out waiting for MariaDB"*) ;;
        *) echo "Expected timeout output, got: $output" >&2; exit 1 ;;
    esac
fi
