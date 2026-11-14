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

setup: setup-keycloak setup-jwt-server

setup-keycloak-old:
	docker exec gfarm-keycloak /keycloak/setup.sh

.PHONY: setup-keycloak setup-jwt-server setup-jwt-server1 setup-jwt-server2
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
