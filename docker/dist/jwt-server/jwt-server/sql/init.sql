-- Test environment: allow connections from any host. In production, keep the
-- database on a private network and restrict this account to required hosts or subnets.
CREATE USER 'gfarm'@'%' IDENTIFIED BY 'gfarm123';
CREATE DATABASE gfarmdb;
GRANT ALL PRIVILEGES ON gfarmdb.* TO 'gfarm'@'%';
