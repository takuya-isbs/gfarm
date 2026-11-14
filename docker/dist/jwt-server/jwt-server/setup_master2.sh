#!/bin/sh
set -xeu
/jwt/setup.sh

mysql < /jwt/sql/init_master2.sql
