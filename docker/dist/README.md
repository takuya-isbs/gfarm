# Docker containers for Gfarm developers

## Prerequisite

- Install `docker compose` ([Ubuntu](https://docs.docker.com/engine/install/ubuntu/) | [CentOS](https://docs.docker.com/engine/install/centos/)) and `make`.

- To allow docker compose to run with user privileges, add $USER to the docker group by `sudo usermod -aG docker $USER`

- Clone Gfarm
```
   % git clone https://github.com/oss-tsukuba/gfarm.git
```

## Proxy configuration (optional)

If you are in a proxy environment, configure proxy-related
environment variables before pulling Docker images or executing
Docker-related commands such as `minica.sh`, `docker compose build`,
or `docker compose up`.

The following environment variables are required in a proxy environment.

`PROXY_HOST` and `PROXY_PORT` are used for the `jwt-server`
related Docker images, and in this case `jwt-server`,
`jwt-server2`, and `keycloak` must be included in
`no_proxy`/`NO_PROXY`.

Example:

    % export PROXY_HOST=proxy.example.com
    % export PROXY_PORT=8080

    % export http_proxy=http://${PROXY_HOST}:${PROXY_PORT}
    % export https_proxy=http://${PROXY_HOST}:${PROXY_PORT}
    % export HTTP_PROXY=http://${PROXY_HOST}:${PROXY_PORT}
    % export HTTPS_PROXY=http://${PROXY_HOST}:${PROXY_PORT}

    % export no_proxy=localhost,127.0.0.1,jwt-server,jwt-server2,keycloak
    % export NO_PROXY=localhost,127.0.0.1,jwt-server,jwt-server2,keycloak

If Docker daemon proxy configuration is required, create a systemd
drop-in file.

    % sudo mkdir -p /etc/systemd/system/docker.service.d

    % sudo vi /etc/systemd/system/docker.service.d/http-proxy.conf

Example:

    [Service]
    Environment="http_proxy=..."
    Environment="https_proxy=..."
    Environment="HTTP_PROXY=..."
    Environment="HTTPS_PROXY=..."
    Environment="no_proxy=localhost,127.0.0.1,jwt-server,jwt-server2,keycloak"
    Environment="NO_PROXY=localhost,127.0.0.1,jwt-server,jwt-server2,keycloak"

Reload systemd configuration and restart Docker.

    % sudo systemctl daemon-reload
    % sudo systemctl restart docker

If Docker client proxy configuration is required, create
`~/.docker/config.json`.

    % mkdir -p ~/.docker

    % vi ~/.docker/config.json

Example:

    {
      "proxies": {
        "default": {
          "httpProxy": "...",
          "httpsProxy": "...",
          "noProxy": "localhost,127.0.0.1,jwt-server,jwt-server2,keycloak"
        }
      }
    }


## Explore on virtual clusters by VS Code dev containers

This section is an option only for VS Code users.

- Open gfarm/ directory
- Open Terminal -> New Terminal
```
   % cd docker/dist
   % sh ./minica.sh
```
- Install VS Code and Dev Containers extension
- Open a command palette by Ctrl+Shift+p and execute "Dev Containers: Rebulid and Reopen in Container"
- Open Terminal -> New Terminal
```
   % cd docker/dist
```
- follow the instructions below after "(in a container)"

## Explore on virtual clusters

    % cd gfarm/docker/dist
    % sh ./minica.sh
    % DIST=<distribution> docker compose build --build-arg UID=$(id -u) c1
    % DIST=<distribution> docker compose up -d
    ubuntu, rockylinux10, rockylinux9, almalinux8, and centos7 are available as $DIST.  Default is ubuntu
    % make          # login to a container

    (in a container)
    % sh ./all.sh
    ./all-rpm.sh also available in case of rocylinux9, almalinux8 and centos7
    This will install and setup Gfarm.  Enjoy!
    % Ctrl-D

    % docker compose down -v --remove-orphans

If the gfarm source repository is not clean, link errors may happen during the `all.sh` execution.  In this case, clean the repository.

    % cd ~/gfarm
    % git clean -dxf

When you would like to execute `all.sh` (or `all-rpm.sh`) again, execute `unconfig.sh`.

When you change the source code, execute `docker/dist/install.sh` in the top source directory and `restart.sh`.

When you install Gfarm by `all.sh`, `regress.sh` and `failover.sh` are available for tests.

## For OAuth authentication

For Docker-based `gfarm-http-gateway` checks, see [http-gateway/README.md](http-gateway/README.md).

View JWT Server and Keycloak access logs with `docker compose logs -f jwt-server-front jwt-server-back keycloak` from `docker/dist/jwt-server`.

### Initialize both JWT Servers

Before either initialization target, build the images from the current checkout:

    % cd jwt-server
    % make build

Run once on first setup with empty database volumes:

    # Run once for the initial setup with empty database volumes.
    % make init
    % docker compose up -d
    % cd ..

- `make init`: initializes both databases and Keycloak.
- Keycloak client: registers the `http-gw` HTTP and HTTPS redirect URIs from `setup-keycloak/keycloak-config.yaml`.
- `make setup` is retired; batch tests initialize the JWT Server stack with `make init`.
- Later startup: `docker compose up -d` starts all initialized services.

### Initialize JWT Server 1 only

Use this setup for GitHub Actions and environments that need only JWT Server 1:

- First setup with an empty database volume: `make init-jwt-server1`
- Start or restart: `docker compose up -d jwt-server-front`

### Startup behavior

- Readiness: each JWT Server backend waits for MariaDB and Keycloak on every start.
- Keycloak not yet configured: the Keycloak OIDC discovery URL returns an error until `make init` creates the HPCI realm. A backend started before that exits after `JWT_SERVER_DEPENDENCY_WAIT_TIMEOUT_SECONDS`, so run `make init` before `docker compose up -d`.
- Initialization containers: stop after setup; `make down` removes them with the Compose services.
- Volume reset: top-level `make down` calls `docker/dist/jwt-server`'s `make down`, removing the JWT Server database and Keycloak volumes.

Wait settings can be overridden with environment variables:

| Variable | Default | Purpose |
|---|---:|---|
| `JWT_SERVER_DEPENDENCY_WAIT_INTERVAL_SECONDS` | `2` | Delay between Keycloak readiness checks |
| `JWT_SERVER_DEPENDENCY_WAIT_TIMEOUT_SECONDS` | `300` | Maximum wait for dependencies |
| `JWT_SERVER_OIDC_DISCOVERY_URL` | `https://keycloak:8443/auth/realms/HPCI/.well-known/openid-configuration` | Keycloak OIDC discovery URL |
| `MYSQL_WAIT_INTERVAL_SECONDS` | `2` | Delay between MariaDB readiness checks |
| `MYSQL_WAIT_TIMEOUT_SECONDS` | `300` | Maximum wait for MariaDB |
| `MARIADB_ADMIN_CONNECT_TIMEOUT_SECONDS` | `2` | Per-check MariaDB connection timeout |

- connect remote desktop to localhost:13389
- login ubuntu/ubuntu
- launch Firefox (or Chromium (/rdesktop/chromium-start.sh))
- open a terminal
- execute `/rdesktop/install-ca-for-browser.sh`
- close Firefox
- launch Firefox
- connect to https://jwt-server/ by Firefox
- (Automatically redirect to http://keycloak:8443/)
- login user1/PASSWORD
- click "Generate and Store a JSON Web Token" button
- user name and passphrase are displayed

```
(in a host)
% make          # login to a container
(in a container at docker/dist/)
% jwt-agent -s https://jwt-server/ -l user1
Passphrase: (paste the passphrase displayed)
% gfuser -A $(id -un) SASL user1
% sh edconf.sh oauth2
```

### Use http proxy instead of remote desktop

- proxy server settings for any web browser on your desktop
  - protocol: HTTP
  - server: (IP address of the host where Docker containers for Gfarm is running)
  - port: 13128
- connect to https://jwt-server/ by web browser
- ignore the certificate warning on https://jwt-server/
- ignore the certificate warning on https://keycloak/

## How to update keys in minica/ if those expires

```
(in a host)
% sh ./minica.sh --update
% cd jwt-server
% docker compose down -v --remove-orphans
% make init
% docker compose up -d
% cd ..
```

## For HPCI Storage

    execute all.sh or all-rpm.sh
    % sh ./hpci.sh
    % mv ~/.globus ~/.globus.bak
    % export GFARM_CONFIG_FILE=$HOME/.gfarm2rc.hpci
    % jwt-agent -s https://elpis.hpci.nii.ac.jp/ -l HPCI_ID

To execute gfperf, execute the following after editing at least $PROJ and $HPCI\_ID

    % myproxy-logon -s portal.hpci.nii.ac.jp -t 168 -l HPCI-ID
    % sh gfperf.sh

## Batch tests

Build, install and setup tests for all distributions.

    % sh ./batchtest.sh [ regress | regress_full ]

When regress or regress\_full option is specified this test includes regression tests.  For all authentications, use regress\_full not regress.  The log files of regression tests are stored in gfarm/build/regress.

The `batchtest.sh` scripts run `make down` before and after testing. This removes the JWT Server database and Keycloak volumes; each run initializes them again with `make init`.

`batchtest.sh` accepts the following options.

- ubuntu, rockylinux10, rockylinux9, almalinux8, centos7 - specifies a distrubution to test.  mutiple distributions can be specified

## Sanitizer test

### ASAN/LSAN/UBSAN test

    (in a container)
    % sh ./all.sh asan clean
    % sh ./regress.sh
    % sh ./restart.sh
    ... and check /var/tmp/gfarm.log.{a,l,ub}san.* on host c{1..8}

### TSAN test

    (in a host)
    % sudo sysctl -w kernel.randomize_va_space=0

    (in a container)
    % sh ./all.sh tsan clean
    % sh ./regress.sh
    % sh ./restart.sh
    ... and check /var/tmp/gfarm.log.tsan.* on host c{1..8}

If ASLR is not disabled on the host, `all.sh tsan clean` fails in configure with "cannot run C compiled programs", and config.log in the build directory shows "FATAL: ThreadSanitizer: unexpected memory mapping".  The sysctl setting is reset when the host (or WSL) restarts, so check it by `cat /proc/sys/kernel/randomize_va_space` (it should be 0) before the TSAN test.

## Create RPM packages

    % sh ./devrpm.sh
    (in the container)
    # sh gfarm/docker/dist/mkrpm.sh gfarm 2.8.0
    # rpm -Uvh rpmbuild/RPMS/x86_64/gfarm-gsi-*
    # sh gfarm/docker/dist/mkrpm.sh gfarm2fs 1.2.19
    # Ctrl-D

    % export PKG=gfarm
    % export VER=2.8.0
    % docker cp alma8:/root/rpmbuild/SRPMS/$PKG-$VER-1.src.rpm .
    % docker stop alma8
    % docker rm alma8
