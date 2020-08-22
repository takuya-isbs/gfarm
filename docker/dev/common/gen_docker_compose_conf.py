#! /usr/bin/env python3

from ipaddress import IPv4Network, IPv6Network
from os import environ
import sys

num_gfmds = int(environ['GFDOCKER_NUM_GFMDS'])
num_gfsds = int(environ['GFDOCKER_NUM_GFSDS'])
num_clients = int(environ['GFDOCKER_NUM_CLIENTS'])
ip_version = environ['GFDOCKER_IP_VERSION']
subnet = environ['GFDOCKER_SUBNET']
start_host_addr = int(environ['GFDOCKER_START_HOST_ADDR'])
hostname_prefix_gfmd = environ['GFDOCKER_HOSTNAME_PREFIX_GFMD']
hostname_prefix_gfsd = environ['GFDOCKER_HOSTNAME_PREFIX_GFSD']
hostname_prefix_client = environ['GFDOCKER_HOSTNAME_PREFIX_CLIENT']

if ip_version == '4':
    nw = IPv4Network(subnet)
elif ip_version == '6':
    nw = IPv6Network(subnet)
else:
    sys.exit('invalid syntax: GFDOCKER_IP_VERSION')

class ContainerHost:
    def __init__(self, hostname, ipaddr):
        self.hostname = hostname
        self.ipaddr = ipaddr

hi = nw.hosts()
hosts = []

for i in range(0, start_host_addr - 1):
    next(hi)  # skip unused hosts

for i in range(0, num_gfmds):
    hosts.append(ContainerHost(
        '{}{}'.format(hostname_prefix_gfmd, i+1),
        next(hi)
    ))

for i in range(0, num_gfsds):
    hosts.append(ContainerHost(
        '{}{}'.format(hostname_prefix_gfsd, i+1),
        next(hi)
    ))

for i in range(0, num_clients):
    hosts.append(ContainerHost(
        '{}{}'.format(hostname_prefix_client, i+1),
        next(hi)
    ))

print('''\
# This file was automatically generated.
# Do not edit this file.

version: "3.4"

x-common:
  &common
  image: gfarm-dev:${GFDOCKER_PRJ_NAME}
  volumes:
    - ./mnt:/mnt:rw
    - /sys/fs/cgroup:/sys/fs/cgroup:ro
    - /run
  security_opt:
    - seccomp:unconfined
    - apparmor:unconfined
  cap_add:
    - SYS_ADMIN
    - SYS_PTRACE
  devices:
    - /dev/fuse
  privileged: false
  extra_hosts:
''', end='')

for h in hosts:
    print("    - {}:{}".format(h.hostname, str(h.ipaddr)))

print('''\

services:
''', end='')

for h in hosts:
    print('''\
  {}:
    hostname: {}
    networks:
      default:
        ipv{}_address: {}
    <<: *common
'''.format(h.hostname, h.hostname, ip_version, str(h.ipaddr)), end='')

print('''\

networks:
  default:
    ipam:
      config:
        - subnet: {}
'''.format(subnet), end='')
