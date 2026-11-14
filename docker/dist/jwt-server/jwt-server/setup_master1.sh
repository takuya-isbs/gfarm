#!/bin/sh
set -xeu
/jwt/setup.sh

mysql < /jwt/sql/init_master1.sql
