# gfarm-http-gateway in docker/dist

This runbook covers the Docker Compose setup and `make test-e2e` entry point for `gfarm-http-gateway` in the Gfarm `docker/dist` environment. See `DEVELOPMENT.md` in the `gfarm-http-gateway` checkout for c2 development, authentication, CLI usage, and E2E tests.

Set up the Gfarm virtual cluster by following [Explore on virtual clusters](../README.md#explore-on-virtual-clusters). Start and configure `docker/dist/jwt-server` and Keycloak by following [For OAuth authentication](../README.md#for-oauth-authentication).

## Run with Docker Compose

Use this setup to verify `gfarm-http-gateway` against the docker/dist Gfarm environment. The Compose file and http-gw image build definition are maintained in `gfarm-http-gateway/server/e2e-test/docker-dist/`; this directory supplies the Gfarm network and host paths.

Run this directory's Make targets on the Docker host. `.env` contains the Redis password for this isolated test. The Makefile sets the external Gfarm network name and passes the checkout and certificate paths to Compose. Redis is authenticated, private to its Compose network, and has no published host port. These targets do not modify `jwt-server`.

After `docker/dist/all.sh` has initialized Gfarm in c1, `make setup` copies its CA certificates to `config/certs/` for `http-gw`. If this Compose project is running, stop it first with `make down`:

```sh
make down
make setup
```

Run `make up` to build and start both Python http-gw services, Redis, and Nginx. `http-gw` uses Python 3.15, `http-gw-python3-15` is its alias, and `http-gw-python3-11` reaches the Python 3.11 service. The target creates the HTTPS certificate if missing under `docker/dist/minica/http-gw-https/`.

For HTTPS, trust `docker/dist/minica/minica.pem` in the browser. See the CA setup instructions in `docker/dist/README.md`.

| Check | Browser URL | Request path | Redirect URI pattern for both Keycloak fields |
|---|---|---|---|
| HTTP application | `http://http-gw:8000/` | Browser -> Squid forward proxy (external browser only) -> `http-gw` | `http://http-gw:8000/*` |
| HTTPS with Nginx | `https://http-gw-https/` | Browser -> Squid forward proxy (external browser only) -> Nginx reverse proxy -> `http-gw` | `https://http-gw-https/*` |

Squid is the forward proxy for browsers outside the Docker environment; configure the browser to use the Docker host's port 13128. It forwards each request to the host and port in the browser URL. Nginx is a reverse proxy only in the HTTPS Compose setup: it terminates TLS and forwards requests to the `http-gw` container.

## Make targets and settings

Run `make help` in this directory for the available targets. To build all images manually with Bake, run `make -C .. bake-images`.

This directory’s `Makefile` sets the docker/dist defaults. See “docker/dist test settings” in `DEVELOPMENT.md` for what each variable controls.
