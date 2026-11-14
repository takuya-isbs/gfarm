#!/bin/sh
set -xeu

while [ ! -e /var/lib/mysql/mysql.sock ]; do
	sleep 1
done
mysql < /jwt/sql/init.sql
mysql gfarmdb < /jwt-server/ddl/jwt-server.ddl
