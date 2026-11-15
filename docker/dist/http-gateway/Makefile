COMPOSE = docker compose --env-file .env -f docker-compose.yaml
DOCKER_DIST_USER ?= $(USER)
GFARM_HTTP_GATEWAY_DIR ?= ../../../gfarm-http-gateway
DOCKER_DIST_CLIENT ?= gfarm-c1
GFARM_HTTP_PLAYWRIGHT_CACHE_HOME ?= $(if $(XDG_CACHE_HOME),$(XDG_CACHE_HOME),$(HOME)/.cache)

.PHONY: help setup up up-nobuild ps logs follow test list-webui-cases test-webui clean-test-cache down

export GFARM_HTTP_WEBUI_CASE

help:
	@echo "make setup  - replace Gfarm CA certs from gfarm-c1"
	@echo "make up     - create minica certs, rebuild Gateway, and start the stack"
	@echo "make up-nobuild - like up, but without build; run 'make bake-images' in docker/dist first"
	@echo "make ps     - show the running services"
	@echo "make logs   - show recent service logs"
	@echo "make follow - follow service logs"
	@echo "make test   - run the gfarm-http-gateway integration test"
	@echo "make list-webui-cases - list real-Gfarm Playwright case titles"
	@echo "make test-webui GFARM_HTTP_WEBUI_CASE='title' - run one Web UI case"
	@echo "make clean-test-cache - remove cached Playwright dependencies"
	@echo "make down   - stop the stack"

setup:
	@set -eu; \
	running="$$($(COMPOSE) ps -aq)"; \
	if [ -n "$$running" ]; then \
		echo "Cannot run make setup while Compose containers exist; run 'make down' first." >&2; \
		exit 1; \
	fi
	@docker exec "$(DOCKER_DIST_CLIENT)" test -d /etc/grid-security/certificates || { \
		echo "Gfarm CA certificates are missing in $(DOCKER_DIST_CLIENT); complete docker/dist/all.sh first." >&2; \
		exit 1; \
	}
	mkdir -p config/certs
	find config/certs -mindepth 1 -maxdepth 1 ! -name .gitkeep -exec rm -rf -- {} +
	docker cp $(DOCKER_DIST_CLIENT):/etc/grid-security/certificates/. config/certs/

up:
	cd .. && sh ./minica.sh
	$(COMPOSE) up -d --build --force-recreate gateway nginx

# Same as up, but does not build: uses an existing gateway image. Used by GitHub Actions,
# which builds the image with docker/bake-action first. Locally, use `make up` unless you
# have already built the image (for example, with `make bake-images` in docker/dist).
# Compose fails if the image is missing, because --no-build does not build it.
up-nobuild:
	cd .. && sh ./minica.sh
	$(COMPOSE) up -d --no-build --force-recreate gateway nginx

ps:
	$(COMPOSE) ps

logs:
	$(COMPOSE) logs --tail=100

follow:
	$(COMPOSE) logs -f

test:
	GFARM_HTTP_PLAYWRIGHT_CACHE_HOME="$(GFARM_HTTP_PLAYWRIGHT_CACHE_HOME)" DOCKER_DIST_USER="$(DOCKER_DIST_USER)" DOCKER_DIST_CLIENT="$(DOCKER_DIST_CLIENT)" "$(GFARM_HTTP_GATEWAY_DIR)/server/bin/test-http-gateway-docker-dist.sh"

list-webui-cases:
	"$(GFARM_HTTP_GATEWAY_DIR)/server/bin/test-http-gateway-docker-dist.sh" --list-webui-cases

test-webui:
	GFARM_HTTP_PLAYWRIGHT_CACHE_HOME="$(GFARM_HTTP_PLAYWRIGHT_CACHE_HOME)" DOCKER_DIST_USER="$(DOCKER_DIST_USER)" DOCKER_DIST_CLIENT="$(DOCKER_DIST_CLIENT)" "$(GFARM_HTTP_GATEWAY_DIR)/server/bin/test-http-gateway-docker-dist.sh" --webui-case

clean-test-cache:
	GFARM_HTTP_PLAYWRIGHT_CACHE_HOME="$(GFARM_HTTP_PLAYWRIGHT_CACHE_HOME)" "$(GFARM_HTTP_GATEWAY_DIR)/server/bin/test-http-gateway-docker-dist.sh" --clean-cache

down:
	$(COMPOSE) down
