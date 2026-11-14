# gfarm-http-gateway in docker/dist

This runbook covers two ways to run `gfarm-http-gateway` in the Gfarm `docker/dist` development environment: the Docker Compose image and the source development server on c2. Authentication and c1 client instructions are shared by both methods.

Set up the Gfarm virtual cluster by following [Explore on virtual clusters](../README.md#explore-on-virtual-clusters). Start and configure `docker/dist/jwt-server` and Keycloak by following [For OAuth authentication](../README.md#for-oauth-authentication).

## Run with Docker Compose

Use this setup to verify the built Gateway image and HTTP/HTTPS access. One Compose project starts the Gateway, Redis, and Nginx. The source remains in the `gfarm-http-gateway` checkout; `.env` sets its Docker build context.

Run this directory's Compose Makefile commands on the Docker host, from this directory. The fixed `.env` settings and Redis password are for this isolated docker/dist test only. Redis is authenticated, private to the Compose network, and has no published host port. This Compose does not modify `jwt-server`.

Compose mounts `config/certs/` read-only into the Gateway container. After creating or rebuilding the `docker/dist` Gfarm containers, refresh the local CA certificates from c1 before starting the Compose project:

```sh
make down
make setup
```

`make setup` refuses to run while containers from this Compose project exist, preventing changes to CA files mounted by a running stack. Then run `make up` below; it creates or recreates the Gateway container so the entrypoint copies the current CA files into its Gfarm trust directory.

For HTTPS, trust `docker/dist/minica/minica.pem` in the browser. See the CA setup instructions in `docker/dist/README.md`.

Start the Compose project with `make up`. It also runs `docker/dist/minica.sh` to create the HTTPS certificate if missing. The HTTPS certificate and key are generated under `docker/dist/minica/gfarm-http-gateway-https/`.

| Check | Browser URL | Request path | Redirect URI pattern for both Keycloak fields |
|---|---|---|---|
| HTTP application | `http://gfarm-http-gateway-http:8000/` | Browser -> Squid forward proxy (external browser only) -> Gateway | `http://gfarm-http-gateway-http:8000/*` |
| HTTPS with Nginx | `https://gfarm-http-gateway-https/` | Browser -> Squid forward proxy (external browser only) -> Nginx reverse proxy -> Gateway | `https://gfarm-http-gateway-https/*` |

Squid is the forward proxy for browsers outside the Docker environment; configure the browser to use the Docker host's port 13128. It forwards each request to the host and port in the browser URL. Nginx is a reverse proxy only in the HTTPS Compose setup: it terminates TLS and forwards requests to the Gateway container.

### Compose Make targets

| Command | Action |
|---|---|
| `make up` | Build and start Gateway, Redis, and Nginx |
| `make ps` | Show service status |
| `make logs` | Show the last 100 log lines |
| `make follow` | Follow service logs |
| `GFARM_HTTP_TEST_PASSWORD=... make test` | Run the integration test against the real Gfarm services |
| `make list-webui-cases` | List Playwright test titles without Docker, Gfarm services, or a password |
| `GFARM_HTTP_TEST_PASSWORD=... make test-webui GFARM_HTTP_WEBUI_CASE='...'` | Run one Playwright test selected by its exact title |
| `make clean-test-cache` | Remove the cached Playwright `node_modules` |
| `make down` | Stop and remove the Compose services |

`make test` runs against the already-running Compose Gateway and the `docker/dist` Gfarm, Keycloak, and JWT Server services. It does not recreate them.

To run one case, copy its exact title from `make list-webui-cases`:

```sh
GFARM_HTTP_TEST_PASSWORD=... make test-webui \
  GFARM_HTTP_WEBUI_CASE='GET /file/{path}: download a Gfarm file'
```

| Variable | Purpose | Defined in |
|---|---|---|
| `GFARM_HTTP_TEST_PASSWORD` | Keycloak `user1` password (required). Not kept in this README; see [For OAuth authentication](../README.md#for-oauth-authentication) and `../init-jwt.sh`. | Environment only |
| `DOCKER_DIST_USER` | c1 user that runs `jwt-agent` (same as the `login` target) | `Makefile` |
| `DOCKER_DIST_CLIENT` | Container that runs the Gfarm client and `jwt-agent` | `Makefile` |
| `GFARM_HTTP_GATEWAY_DIR` | Path to the `gfarm-http-gateway` checkout | `Makefile` |
| `GFARM_HTTP_PLAYWRIGHT_CACHE_HOME` | Host cache home; the runner uses its `gfarm-http-gateway/playwright` subdirectory. Defaults to `$XDG_CACHE_HOME` or `$HOME/.cache` | `Makefile` and test script |
| `GFARM_HTTP_TEST_USERNAME` | Expected Gfarm username | Test script |
| `GFARM_HTTP_GATEWAY_URL` | Gateway under test | Test script |
| `GFARM_HTTP_WEBUI_CASE` | Exact Playwright title selected by `make test-webui` | Make command line |
| `DOCKER_DIST_NETWORK` | Docker network for the Playwright containers | Test script |
| `JWT_SERVER_URL` | JWT Server login page and `jwt-agent` server URL | Test script |
| `PLAYWRIGHT_DOCKER_IMAGE` | Playwright container image; must match `@playwright/test` in `package-lock.json` | Test script |

Defaults are not repeated here. Change them in the `Makefile` or at the top of `server/bin/test-http-gateway-docker-dist.sh` in the `gfarm-http-gateway` checkout. Environment variables override both.

Authentication effects on the c1 account:

- `make test` stops and restarts `jwt-agent` with a fresh JWT.
- `make test-webui` reuses a valid active JWT. If it is missing or expired, the test obtains a fresh JWT, stops and restarts `jwt-agent`, and invalidates the previous JWT.

Prerequisites:

- `make up` and `make setup` have completed.
- `gfarm-c1` has `go` and can download Go modules.
- Docker can run the Playwright image and reach npm when the dependency cache is first created or needs updating.

Steps for `make test`: obtain a fresh JWT through the JWT Server login (Playwright) -> restart `jwt-agent` -> run the CLI and Web UI suites.

For a single Web UI case, run `make list-webui-cases`, then pass one exact title as `GFARM_HTTP_WEBUI_CASE` to `make test-webui`. The runner rejects titles that match zero or multiple cases. It prepares only that case's fixture data and skips the CLI operation suite. A valid active `jwt-agent` credential is reused for fixture setup; the runner refreshes it only when needed. The Web UI login and the selected case still run against the configured real services.

Test preparation:

- React sources: Copied from the current checkout to a temporary directory for each run.
- Dependencies: A per-user `node_modules` cache is shared by full and selected-case runs. `npm ci` runs when `package.json`, `package-lock.json`, or the Playwright image changes.
- Cache cleanup: Old dependency versions remain until `make clean-test-cache` removes the cache.
- CLI helper: Rebuilt for each invocation.

Both test commands use a unique Gfarm `/tmp` directory and remove it on exit. See `gfarm-http-gateway/server/bin/test-http-gateway-docker-dist.sh` for setup and orchestration, and `client/test/gfarm-http-test.sh` and `server/frontend/app/react-app/tests/docker-dist.operations.spec.js` for case details.

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

5. Start the Gateway with its docker/dist Gfarm client configuration:

   ```sh
   GFARM_HTTP_GFARM_CONFIG_FILE="$HOME/gfarm/gfarm-http-gateway/server/gfarm2.conf-for-docker-dist" \
     bin/gfarm-http-gateway-dev-for-docker-dist.sh --port 8000
   ```

   This starts the Gateway in developer mode with debug logging and automatic reload. Set `GFARM_HTTP_GFARM_CONFIG_FILE` explicitly; without it, Gfarm commands may use the runtime user's `~/.gfarm2rc` and operate with that user's Gfarm identity and settings.

   To run another development instance on c3, repeat the dependency and Redis setup there before starting the Gateway.

| Check | Browser URL | Request path | Redirect URI pattern for both Keycloak fields |
|---|---|---|---|
| c2 development server | `http://c2:8000/` | Browser -> Squid forward proxy (external browser only) -> Gateway on c2:8000 | `http://c2:8000/*` |

## Common procedures

### Configure authentication and access the Web UI

The Compose Gateway HTTP and HTTPS redirect patterns are applied to the `hpci-pub` client by `make init`; their values are in `docker/dist/jwt-server/setup-keycloak/keycloak-config.yaml`. The login callback uses the Gateway's `/` endpoint by default. After OIDC logout, the identity provider returns the browser to the Gateway's `/logout` endpoint; the Gateway adds a changing `state` query parameter and then clears its local session.

For a browser outside the Docker environment, configure its HTTP proxy by following [Use http proxy instead of remote desktop](../README.md#use-http-proxy-instead-of-remote-desktop). A browser in the remote desktop environment does not need this proxy setting.

1. For a non-Compose URL such as c2, open the Keycloak admin console at `https://keycloak:8443/auth/admin/master/console/#/HPCI/` and log in (`admin/admin` in the docker/dist example).
2. Select the **HPCI** realm, open **Clients**, and edit **`hpci-pub`**.
3. Add the URL's Redirect URI pattern to both **Valid Redirect URIs** and **Valid Post Logout Redirect URIs**, then save.
4. Open the corresponding URL in a browser, click **Login**, then sign in as `user1` with the password described in [For OAuth authentication](../README.md#for-oauth-authentication) when redirected to Keycloak. The landing page includes API usage examples.

The login flow is the same for the Compose Gateway and the c2 development server; use the URL and matching Redirect URI pattern from the corresponding section. The `/*` suffix permits paths below that host and port, including the login callback and `/logout`. These wildcard entries are for this isolated `docker/dist` test setup. For production, register the specific public callback and post-logout URIs instead of broad wildcard patterns.

### Use the command-line client from c1

1. Prepare OAuth authentication in c1 by following [For OAuth authentication](../README.md#for-oauth-authentication). Run `jwt-agent` and `sh edconf.sh oauth2` as the same user that will run the client. The client reads the token saved for that user.
2. Build and install the client into the user's home directory:

   ```sh
   cd ~/gfarm/gfarm-http-gateway/client
   make
   make PREFIX="$HOME/.local" install
   export PATH="$HOME/.local/bin:$PATH"
   ```

3. Set `GFARM_HTTP_URL` to the Gateway you want to use, then run read-only commands:

   ```sh
   # Docker Compose Gateway
   export GFARM_HTTP_URL=http://gfarm-http-gateway-http:8000
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
   export GFARM_HTTP_URL=http://gfarm-http-gateway-http:8000
   # Or, for the development server on c2:
   # export GFARM_HTTP_URL=http://c2:8000
   ./gfarm-http whoami
   ./gfarm-http ls -la /
   ```

The installed and checkout-based commands were tested in c1 against the Compose Gateway; `whoami` returned `user1` and the root listing succeeded. The c2 URL is the alternative when using the development server.
