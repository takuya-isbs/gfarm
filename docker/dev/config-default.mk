#GFDOCKER_PROXY_HOST = 192.168.0.1
#GFDOCKER_PROXY_PORT = 8080
GFDOCKER_PROXY_HOST =
GFDOCKER_PROXY_PORT =
GFDOCKER_NUM_JOBS = 8

# number of containers
#GFDOCKER_NUM_GFMDS >= 1
#GFDOCKER_NUM_GFSDS >= 1
#GFDOCKER_NUM_CLIENTS >= 1
GFDOCKER_NUM_GFMDS = 3
GFDOCKER_NUM_GFSDS = 4
GFDOCKER_NUM_CLIENTS = 2

# number of local/global accounts
GFDOCKER_NUM_USERS = 4

# requirements:
#GFDOCKER_IP_VERSION: syntax: 4 or 6
#GFDOCKER_SUBNET: syntax: ${ip_address}/${prefix}
#GFDOCKER_START_HOST_ADDR: syntax: intager
GFDOCKER_IP_VERSION = 4
GFDOCKER_SUBNET = 192.168.224.0/24
GFDOCKER_START_HOST_ADDR = 11

# syntax: [A-Za-z0-9]([A-Za-z0-9]|-|_)*
GFDOCKER_HOSTNAME_PREFIX_GFMD = gfmd
GFDOCKER_HOSTNAME_PREFIX_GFSD = gfsd
GFDOCKER_HOSTNAME_PREFIX_CLIENT = client

# syntax: sharedsecret, gsi or gsi_auth
GFDOCKER_AUTH_TYPE = gsi_auth

# --no-cache for docker build (0: disable, 1: enable)
GFDOCKER_NO_CACHE = 0
SUDO = sudo
