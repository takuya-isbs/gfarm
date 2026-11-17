# gfarm-http-gateway in docker/dist

This runbook covers two ways to run `gfarm-http-gateway` in the Gfarm `docker/dist` development environment: the Docker Compose image and the source development server on c2. Authentication and c1 client instructions are shared by both methods.

Set up the Gfarm virtual cluster by following [Explore on virtual clusters](../README.md#explore-on-virtual-clusters). Start and configure `docker/dist/jwt-server` and Keycloak by following [For OAuth authentication](../README.md#for-oauth-authentication).

## Run with Docker Compose

Use this setup to verify the built `http-gw` image and HTTP/HTTPS access. One Compose project starts `http-gw`, Redis, and Nginx. The source remains in the `gfarm-http-gateway` checkout; `.env` sets its Docker build context.

Run this directory's Compose Makefile commands on the Docker host, from this directory. The fixed `.env` settings and Redis password are for this isolated docker/dist test only. Redis is authenticated, private to the Compose network, and has no published host port. This Compose does not modify `jwt-server`.

After `docker/dist/all.sh` has initialized Gfarm in c1, `make setup` copies its CA certificates to `config/certs/` for `http-gw`. If this Compose project is running, stop it first with `make down`:

```sh
make down
make setup
```

Then run `make up` to start `http-gw`, which copies the certificates into its Gfarm trust directory.

For HTTPS, trust `docker/dist/minica/minica.pem` in the browser. See the CA setup instructions in `docker/dist/README.md`.

Start the Compose project with `make up`. It also runs `docker/dist/minica.sh` to create the HTTPS certificate if missing. The HTTPS certificate and key are generated under `docker/dist/minica/http-gw-https/`.

| Check | Browser URL | Request path | Redirect URI pattern for both Keycloak fields |
|---|---|---|---|
| HTTP application | `http://http-gw:8000/` | Browser -> Squid forward proxy (external browser only) -> `http-gw` | `http://http-gw:8000/*` |
| HTTPS with Nginx | `https://http-gw-https/` | Browser -> Squid forward proxy (external browser only) -> Nginx reverse proxy -> `http-gw` | `https://http-gw-https/*` |

Squid is the forward proxy for browsers outside the Docker environment; configure the browser to use the Docker host's port 13128. It forwards each request to the host and port in the browser URL. Nginx is a reverse proxy only in the HTTPS Compose setup: it terminates TLS and forwards requests to the `http-gw` container.

### Compose Make targets

| Command | Action |
|---|---|
| `make setup` | Copy the Gfarm CA certificates from c1 for the `http-gw` container |
| `make up` | Build and start `http-gw`, Redis, and Nginx |
| `make up-nobuild` | Start the same services without building; the `http-gw` image must already exist (GitHub Actions uses this after bake) |
| `make -C .. bake-images` | Build all docker/dist images with bake from the gfarm root (manual check only; GitHub Actions does not call it) |
| `make ps` | Show service status |
| `make logs` | Show the last 100 log lines |
| `make follow` | Follow service logs |
| `make down` | Stop and remove the Compose services |

### Run the real-Gfarm E2E suite

Start the Compose project and the docker/dist Gfarm, Keycloak, and JWT Server services first. From this directory, run:

```sh
GFARM_HTTP_TEST_PASSWORD=... make test-e2e
```

The test runner, case selection, cache, and authentication behavior are documented in the `gfarm-http-gateway` checkout's root README under “Test against a real Gfarm.”

This directory owns the Compose and docker/dist-specific settings. The `gfarm-http-gateway` checkout owns the integration test runner and its behavior; this Makefile includes its test targets while preserving the command entry point.

### docker/dist test settings

These settings adapt the test runner to the docker/dist layout. Defaults are in this directory's `Makefile`.

| Variable | Purpose |
|---|---|
| `GFARM_HTTP_TEST_PASSWORD` | Keycloak `user1` password required by the E2E suite. See [For OAuth authentication](../README.md#for-oauth-authentication) and `../init-jwt.sh`. |
| `DOCKER_DIST_USER` | c1 user that runs `jwt-agent` (same as the `login` target) |
| `DOCKER_DIST_CLIENT` | Container that runs the Gfarm client and `jwt-agent` |
| `DOCKER_DIST_NETWORK` | Compose network used by the test containers |
| `DOCKER_DIST_CHECKOUT_ROOT` | docker/dist checkout path inside the client container |
| `DOCKER_DIST_WORKDIR` | docker/dist working directory inside the client container |
| `GFARM_HTTP_GATEWAY_DIR` | Path to the `gfarm-http-gateway` checkout |
| `GFARM_HTTP_GATEWAY_URL` | Gateway URL reachable from docker/dist and test containers |
| `JWT_SERVER_URL` | JWT Server URL used by the test login and `jwt-agent` |

## Run the source development server on c2

Use this setup to develop against the checkout on c2 with automatic source reload. It runs the application directly and does not use the Docker image.

1. Clone `gfarm-http-gateway` into the Gfarm source checkout if it is not already present:

   ```sh
   cd (Gfarm source checkout)
   git clone https://github.com/oss-tsukuba/gfarm-http-gateway.git
   ```

2. Log in to c1 from the Docker host, then connect to c2:

   ```sh
   cd (Gfarm source checkout)/docker/dist
   make
   ```

   In the c1 container:

   ```sh
   ssh c2
   ```

3. Install the c2 development dependencies:

   ```sh
   cd ~/gfarm/gfarm-http-gateway/server
   make setup-latest-with-sys-packages
   ```

   This installs Python 3.12+, Node.js 26 via nvm, and the Redis server package.

4. Start Redis and confirm it is ready:

   ```sh
   redis-server ./redis-for-docker-dist.conf
   redis-cli -h 127.0.0.1 -p 6379 ping  # PONG
   ```

   Stop Redis with `redis-cli -h 127.0.0.1 -p 6379 shutdown`.

5. Start `http-gw` with its docker/dist Gfarm client configuration:

   ```sh
   GFARM_HTTP_GFARM_CONFIG_FILE="$HOME/gfarm/gfarm-http-gateway/server/gfarm2.conf-for-docker-dist" \
     bin/gfarm-http-gateway-dev-for-docker-dist.sh --port 8000
   ```

   This starts `http-gw` in developer mode with debug logging and automatic reload. Set `GFARM_HTTP_GFARM_CONFIG_FILE` explicitly; without it, Gfarm commands may use the runtime user's `~/.gfarm2rc` and operate with that user's Gfarm identity and settings.

   To run another development instance on c3, repeat the dependency and Redis setup there before starting `http-gw`.

| Check | Browser URL | Request path | Redirect URI pattern for both Keycloak fields |
|---|---|---|---|
| c2 development server | `http://c2:8000/` | Browser -> Squid forward proxy (external browser only) -> `http-gw` on c2:8000 | `http://c2:8000/*` |

## Common procedures

### Configure authentication and access the Web UI

The Compose `http-gw` HTTP and HTTPS redirect patterns are applied to the `hpci-pub` client by `make init`; their values are in `docker/dist/jwt-server/setup-keycloak/keycloak-config.yaml`. The login callback uses the `http-gw` `/` endpoint by default. After OIDC logout, the identity provider returns the browser to the `http-gw` `/logout` endpoint; `http-gw` adds a changing `state` query parameter and then clears its local session.

For a browser outside the Docker environment, configure its HTTP proxy by following [Use http proxy instead of remote desktop](../README.md#use-http-proxy-instead-of-remote-desktop). A browser in the remote desktop environment does not need this proxy setting.

1. For a non-Compose URL such as c2, open the Keycloak admin console at `https://keycloak:8443/auth/admin/master/console/#/HPCI/` and log in (`admin/admin` in the docker/dist example).
2. Select the **HPCI** realm, open **Clients**, and edit **`hpci-pub`**.
3. Add the URL's Redirect URI pattern to both **Valid Redirect URIs** and **Valid Post Logout Redirect URIs**, then save.
4. Open the corresponding URL in a browser, click **Login**, then sign in as `user1` with the password described in [For OAuth authentication](../README.md#for-oauth-authentication) when redirected to Keycloak. The landing page includes API usage examples.

The login flow is the same for Compose `http-gw` and the c2 development server; use the URL and matching Redirect URI pattern from the corresponding section. The `/*` suffix permits paths below that host and port, including the login callback and `/logout`. These wildcard entries are for this isolated `docker/dist` test setup. For production, register the specific public callback and post-logout URIs instead of broad wildcard patterns.

### Use the command-line client from c1

1. Prepare OAuth authentication in c1 by following [For OAuth authentication](../README.md#for-oauth-authentication). Run `jwt-agent` and `sh edconf.sh oauth2` as the same user that will run the client. The client reads the token saved for that user.
2. Build and install the client into the user's home directory:

   ```sh
   cd ~/gfarm/gfarm-http-gateway/client
   make
   make PREFIX="$HOME/.local" install
   export PATH="$HOME/.local/bin:$PATH"
   ```

3. Set `GFARM_HTTP_URL` to the `http-gw` instance you want to use, then run read-only commands:

   ```sh
   # Docker Compose http-gw
   export GFARM_HTTP_URL=http://http-gw:8000
   # Or, development server on c2
   # export GFARM_HTTP_URL=http://c2:8000

   gfarm-http whoami
   gfarm-http ls -la /
   ```

   The Compose hostname is available to c1 on `gfarm_net`. Uncomment the c2 URL instead when using the development server.

4. To use the built client without installing it, run it from the checkout:

   ```sh
   cd ~/gfarm/gfarm-http-gateway/client
   make
   export GFARM_HTTP_URL=http://http-gw:8000
   # Or, for the development server on c2:
   # export GFARM_HTTP_URL=http://c2:8000
   ./gfarm-http whoami
   ./gfarm-http ls -la /
   ```

The installed and checkout-based commands were tested in c1 against Compose `http-gw`; `whoami` returned `user1` and the root listing succeeded. The c2 URL is the alternative when using the development server.
