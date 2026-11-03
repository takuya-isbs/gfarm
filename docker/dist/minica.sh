#!/bin/sh

set -eu

UPDATE="${1:-}"

HOSTS="keycloak jwt-server jwt-server2 gfarm-http-gateway-https"

cd minica

remove_key() {
	HOST=$1
	rm -f "$HOST"/*.pem
}

if [ "$UPDATE" = "--update" ]; then
	rm -f minica.pem minica-key.pem
	for h in $HOSTS; do
		remove_key "$h"
	done
fi

# MINICA_SKIP_BUILD is set when the gfarm-minica image is already built
if [ -z "${MINICA_SKIP_BUILD:-}" ] && ! docker image inspect gfarm-minica >/dev/null 2>&1; then
	docker build . -t gfarm-minica
fi

MINICA="docker run --rm -u $(id -u):$(id -g) -w /minica -v .:/minica gfarm-minica"

create_key() {
	HOST=$1
	$MINICA -domains "$HOST"
	chmod go+r *.pem
	chmod go+rx "$HOST"
	chmod go+r "$HOST"/*.pem
}

for h in $HOSTS; do
	if [ ! -f "$h/cert.pem" ] || [ ! -f "$h/key.pem" ]; then
		create_key "$h"
	fi
done
