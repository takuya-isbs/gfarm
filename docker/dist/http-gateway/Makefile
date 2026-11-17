COMPOSE = docker compose --env-file .env -f docker-compose.yaml
GFARM_HTTP_GATEWAY_DIR ?= ../../../gfarm-http-gateway
DOCKER_DIST_USER ?= $(USER)
DOCKER_DIST_CLIENT ?= gfarm-c1
DOCKER_DIST_NETWORK ?= gfarm_net
DOCKER_DIST_CHECKOUT_ROOT ?= /home/$(USER)/gfarm
DOCKER_DIST_WORKDIR ?= $(DOCKER_DIST_CHECKOUT_ROOT)/docker/dist
GFARM_HTTP_GATEWAY_URL ?= http://http-gw:8000
GFARM_HTTP_TEST_USERNAME ?= user1
JWT_SERVER_URL ?= https://jwt-server/

.PHONY: help setup up up-nobuild ps logs follow down

export DOCKER_DIST_USER DOCKER_DIST_CLIENT DOCKER_DIST_NETWORK
export DOCKER_DIST_CHECKOUT_ROOT DOCKER_DIST_WORKDIR
export GFARM_HTTP_GATEWAY_URL GFARM_HTTP_TEST_USERNAME JWT_SERVER_URL

help:
	@echo "make setup  - replace Gfarm CA certs from gfarm-c1"
	@echo "make up     - create minica certs, rebuild http-gw, and start the stack"
	@echo "make up-nobuild - like up, but without build; run 'make bake-images' in docker/dist first"
	@echo "make ps     - show the running services"
	@echo "make logs   - show recent service logs"
	@echo "make follow - follow service logs"
	@echo "make down   - stop the stack"
	@$(MAKE) --no-print-directory help-integration-test

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
	$(COMPOSE) up -d --build --force-recreate http-gw nginx

# Same as up, but does not build: uses an existing http-gw image. Used by GitHub Actions,
# which builds the image with docker/bake-action first. Locally, use `make up` unless you
# have already built the image (for example, with `make bake-images` in docker/dist).
# Compose fails if the image is missing, because --no-build does not build it.
up-nobuild:
	cd .. && sh ./minica.sh
	$(COMPOSE) up -d --no-build --force-recreate http-gw nginx

ps:
	$(COMPOSE) ps

logs:
	$(COMPOSE) logs --tail=100

follow:
	$(COMPOSE) logs -f

down:
	$(COMPOSE) down

# Keep docker/dist-specific service and connection settings here. The gateway
# checkout owns the integration test runner and its Make targets.
include $(GFARM_HTTP_GATEWAY_DIR)/server/e2e-test/docker-dist.mk
