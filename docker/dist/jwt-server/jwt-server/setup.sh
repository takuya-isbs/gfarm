#!/bin/sh
set -xeu

/jwt/wait-for-mariadb.sh
mysql < /jwt/sql/init.sql
mysql gfarmdb < /jwt-server/ddl/jwt-server.ddl
