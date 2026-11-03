COMPOSE = docker compose --env-file .env -f docker-compose.yaml

.PHONY: help setup up ps logs follow down

help:
	@echo "make setup  - replace Gfarm CA certs from gfarm-c1"
	@echo "make up     - create minica certs, rebuild Gateway, and start the stack"
	@echo "make ps     - show the running services"
	@echo "make logs   - show recent service logs"
	@echo "make follow - follow service logs"
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
	docker cp gfarm-c1:/etc/grid-security/certificates/. config/certs/

up:
	cd .. && sh ./minica.sh
	$(COMPOSE) up -d --build --force-recreate gateway nginx

ps:
	$(COMPOSE) ps

logs:
	$(COMPOSE) logs --tail=100

follow:
	$(COMPOSE) logs -f

down:
	$(COMPOSE) down
