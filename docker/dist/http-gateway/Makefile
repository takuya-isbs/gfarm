COMPOSE = docker compose --env-file .env -f docker-compose.yaml
DOCKER_DIST_USER ?= $(USER)
GFARM_HTTP_GATEWAY_DIR ?= ../../../gfarm-http-gateway
DOCKER_DIST_CLIENT ?= gfarm-c1

.PHONY: help setup up ps logs follow test down

help:
	@echo "make setup  - replace Gfarm CA certs from gfarm-c1"
	@echo "make up     - create minica certs, rebuild Gateway, and start the stack"
	@echo "make ps     - show the running services"
	@echo "make logs   - show recent service logs"
	@echo "make follow - follow service logs"
	@echo "make test   - run the gfarm-http-gateway integration test"
	@echo "make down   - stop the stack"

setup:
	@set -eu; \
	running="$$($(COMPOSE) ps -aq)"; \
	if [ -n "$$running" ]; then \
		echo "Cannot run make setup while Compose containers exist; run 'make down' first." >&2; \
		exit 1; \
	fi
	rm -rf config/certs
	mkdir -p config/certs
	docker cp $(DOCKER_DIST_CLIENT):/etc/grid-security/certificates/. config/certs/

up:
	cd .. && sh ./minica.sh
	$(COMPOSE) up -d --build --force-recreate gateway nginx

ps:
	$(COMPOSE) ps

logs:
	$(COMPOSE) logs --tail=100

follow:
	$(COMPOSE) logs -f

test:
	DOCKER_DIST_USER="$(DOCKER_DIST_USER)" DOCKER_DIST_CLIENT="$(DOCKER_DIST_CLIENT)" "$(GFARM_HTTP_GATEWAY_DIR)/server/bin/test-http-gateway-docker-dist.sh"

down:
	$(COMPOSE) down
