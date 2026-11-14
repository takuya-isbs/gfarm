#!/usr/bin/env bash
set -Eeuo pipefail

exec /usr/local/bin/container-service "${1:-tomcat}"
