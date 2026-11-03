# gfarm-http-gateway in docker/dist

Dockerfile verification for `gfarm-http-gateway` in `gfarm/docker/dist`. The source stays in `gfarm-http-gateway/server`; `.env` sets the build context.

## Prerequisites

- Start the `docker/dist` Gfarm containers and `docker/dist/jwt-server`; `gfarm_net` and Keycloak must be available.
- Run the Makefile commands on the Docker host, from this directory.
- Compose mounts `config/certs/` read-only into the gateway container. After creating or rebuilding the `docker/dist` Gfarm containers, run these commands from this directory to replace the local CA certificates with the current files from c1:

  ```sh
  make down
  make setup
  make up
  ```

  `make setup` refuses to run while containers from this Compose project exist, preventing changes to CA files mounted by a running stack. `make up` recreates the Gateway container so the entrypoint copies the updated CA files into its Gfarm trust directory.

- For HTTPS, trust `docker/dist/minica/minica.pem` in the browser. See the CA setup instructions in `docker/dist/README.md`.

The fixed `.env` settings and Redis password are for this isolated docker/dist test only. Redis is authenticated, private to the Compose network, and has no published host port. This Compose does not modify `jwt-server`.

## Start and access

One Compose starts Gateway, Redis, and Nginx. It also runs `docker/dist/minica.sh` to create the HTTPS certificate if missing.

```sh
make up
```

| Check | URL through the docker/dist proxy | Path | Add to both Keycloak URI fields |
|---|---|---|---|
| HTTP application | `http://gfarm-http-gateway-http:8000/` | Browser → Gateway | `http://gfarm-http-gateway-http:8000/*` |
| HTTPS with Nginx | `https://gfarm-http-gateway-https/` | Browser → Nginx → Gateway | `https://gfarm-http-gateway-https/*` |

The HTTPS certificate and key are generated under `docker/dist/minica/gfarm-http-gateway-https/`.

## Configure Keycloak redirect URIs

Register the browser-facing URL for each check you plan to use in the Keycloak client. The login callback uses the gateway's `/` endpoint by default. After OIDC logout, the identity provider returns the browser to the gateway's `/logout` endpoint; the gateway adds a changing `state` query parameter and then clears its local session.

1. Open the Keycloak admin console at `https://keycloak:8443/auth/admin/master/console/#/HPCI/` and log in (`admin/admin` in the docker/dist example).
2. Select the **HPCI** realm, open **Clients**, and edit **`hpci-pub`**.
3. Add the URI pattern from the table above to both **Valid Redirect URIs** and **Valid Post Logout Redirect URIs**. Add only the HTTP entry, HTTPS entry, or both depending on which checks you will use.
4. Save the client settings.

The `/*` suffix permits paths below that host and port, including the login callback and `/logout`. These wildcard entries are for this isolated `docker/dist` test setup. For production, register the specific public callback and post-logout URIs instead of broad wildcard patterns.

## Make targets

| Command | Action |
|---|---|
| `make up` | Build and start Gateway, Redis, and Nginx |
| `make ps` | Show service status |
| `make logs` | Show the last 100 log lines |
| `make follow` | Follow service logs |
| `make down` | Stop and remove the Compose services |
