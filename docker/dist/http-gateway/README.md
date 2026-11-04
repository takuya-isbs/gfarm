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
| HTTP application | `http://gfarm-http-gateway-http:8000/` | Browser → Squid forward proxy (external browser only) → Gateway | `http://gfarm-http-gateway-http:8000/*` |
| HTTPS with Nginx | `https://gfarm-http-gateway-https/` | Browser → Squid forward proxy (external browser only) → Nginx reverse proxy → Gateway | `https://gfarm-http-gateway-https/*` |

Squid is the forward proxy for browsers outside the Docker environment; configure the browser to use the Docker host's port 13128. It forwards each request to the host and port in the browser URL. Nginx is a reverse proxy only in the HTTPS Compose setup: it terminates TLS and forwards requests to the Gateway container.

### Compose Make targets

| Command | Action |
|---|---|
| `make up` | Build and start Gateway, Redis, and Nginx |
| `make ps` | Show service status |
| `make logs` | Show the last 100 log lines |
| `make follow` | Follow service logs |
| `make down` | Stop and remove the Compose services |

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
| c2 development server | `http://c2:8000/` | Browser → Squid forward proxy (external browser only) → Gateway on c2:8000 | `http://c2:8000/*` |

## Common procedures

### Configure authentication and access the Web UI

Register the browser-facing URL for each gateway you plan to use in the Keycloak client. The login callback uses the Gateway's `/` endpoint by default. After OIDC logout, the identity provider returns the browser to the Gateway's `/logout` endpoint; the Gateway adds a changing `state` query parameter and then clears its local session.

For a browser outside the Docker environment, configure its HTTP proxy by following [Use http proxy instead of remote desktop](../README.md#use-http-proxy-instead-of-remote-desktop). A browser in the remote desktop environment does not need this proxy setting.

1. Open the Keycloak admin console at `https://keycloak:8443/auth/admin/master/console/#/HPCI/` and log in (`admin/admin` in the docker/dist example).
2. Select the **HPCI** realm, open **Clients**, and edit **`hpci-pub`**.
3. For each gateway URL above that you plan to use, copy its Redirect URI pattern into both **Valid Redirect URIs** and **Valid Post Logout Redirect URIs**.
4. Save the client settings.
5. Open the corresponding URL in a browser, click **Login**, then sign in as `user1 / PASSWORD` when redirected to Keycloak. The landing page includes API usage examples.

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
