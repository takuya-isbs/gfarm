TERM_INHERIT = $${TERM+--env TERM='${TERM}'}

login:
	docker exec -u ${USER} $(TERM_INHERIT) \
		-w /home/${USER}/gfarm/docker/dist -it gfarm-c1 /bin/bash

down:
	$(MAKE) -C jwt-server down
	docker compose down -t 2

# Build the docker/dist images with bake. Run from the gfarm root, because bake resolves
# context paths from the current working directory.
#
# Purpose: manual verification and a memo of the build steps. This performs the same build
# as the docker/bake-action step in gfarm-http-gateway/.github/workflows/test.yaml, but
# GitHub Actions does not call this target. For local runs, compose alone is enough
# (for example, `make up` in http-gateway builds the http-gw image).
#
# The http-gw Bake definition is maintained in the gfarm-http-gateway checkout.
# Gfarm's docker/dist settings are provided by docker/dist/http-gateway/Makefile.
#
# BUILD_USER and BUILD_UID default to the current user; PROXY_HOST and PROXY_PORT are optional.
BUILD_USER ?= $(shell id -un)
BUILD_UID ?= $(shell id -u)

bake-images:
	cd ../.. && BUILD_USER=$(BUILD_USER) BUILD_UID=$(BUILD_UID) \
		docker buildx bake --load \
		-f docker/dist/docker-bake.hcl \
		-f gfarm-http-gateway/server/e2e-test/docker-dist/docker-bake.hcl gfarm http-gw
