TERM_INHERIT = $${TERM+--env TERM='${TERM}'}

login-jwt-server:
	docker exec $(TERM_INHERIT) -it gfarm-jwt-server-back /bin/bash

login-jwt-server2:
	docker exec $(TERM_INHERIT) -it gfarm-jwt-server2-back /bin/bash

login-keycloak:
	docker exec $(TERM_INHERIT) -it gfarm-keycloak /bin/bash

login-rdesktop:
	docker exec $(TERM_INHERIT) -it gfarm-rdesktop /bin/bash

login-setup-keycloak:
	docker compose run $(TERM_INHERIT) --remove-orphans --rm -it setup-keycloak /bin/bash

build-jwt-server:
	docker compose build jwt-server-back --no-cache

build-setup-keycloak:
	docker compose build setup-keycloak --no-cache

build: build-jwt-server build-setup-keycloak

.PHONY: setup
setup:
	@printf '%s\n' "[ERROR] Initial setup now uses 'make init'; run it once with empty database volumes." >&2
	@exit 2

setup-keycloak-old:
	docker exec gfarm-keycloak /keycloak/setup.sh

.PHONY: init init-jwt-server1 test-init check-init-db-services
.PHONY: setup-keycloak setup-jwt-server setup-jwt-server1 setup-jwt-server2

check-init-db-services:
	@set -eu; \
	for service in $(INIT_DATABASE_SERVICES); do \
		if [ -n "$$(docker compose ps --quiet "$$service")" ]; then \
			echo "[ERROR] $$service is running; stop it before first-time initialization." >&2; \
			exit 1; \
		fi; \
	done

# Development-only test of the MariaDB readiness helper; it uses a mock command.
test-init:
	./tests/test-wait-for-mariadb.sh

init: INIT_DATABASE_SERVICES = jwt-server-db jwt-server2-db
init: check-init-db-services
	docker compose build	# use cache
	docker compose run --rm --name "gfarm-jwt-server-db-init-$$(date +%s)-$$$$" --no-deps -e GFARM_JWT_SERVER_DB_INIT=master1 jwt-server-db
	docker compose run --rm --name "gfarm-jwt-server2-db-init-$$(date +%s)-$$$$" --no-deps -e GFARM_JWT_SERVER_DB_INIT=master2 jwt-server2-db
	docker compose up -d keycloak
	$(MAKE) setup-keycloak

init-jwt-server1: INIT_DATABASE_SERVICES = jwt-server-db
init-jwt-server1: check-init-db-services
	docker compose run --rm --name "gfarm-jwt-server-db-init-$$(date +%s)-$$$$" --no-deps -e GFARM_JWT_SERVER_DB_INIT=master1 jwt-server-db
	docker compose up -d keycloak
	$(MAKE) setup-keycloak

setup-keycloak:
	docker compose run --remove-orphans --rm setup-keycloak \
    python3 /setup-keycloak/keycloak-config.py /setup-keycloak/keycloak-config.yaml

setup-jwt-server: setup-jwt-server1 setup-jwt-server2

setup-jwt-server1:
	docker exec gfarm-jwt-server-db /jwt/setup_master1.sh

setup-jwt-server2:
	docker exec gfarm-jwt-server2-db /jwt/setup_master2.sh

up:
	docker compose up -d

down:
	docker compose down -v --remove-orphans

ps:
	docker compose ps
