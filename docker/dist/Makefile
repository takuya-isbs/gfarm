TERM_INHERIT = $${TERM+--env TERM='${TERM}'}

login:
	docker exec -u ${USER} $(TERM_INHERIT) \
		-w /home/${USER}/gfarm/docker/dist -it gfarm-c1 /bin/bash

down:
	$(MAKE) -C jwt-server down
	docker compose down -t 2

BUILD_USER ?= $(shell id -un)
BUILD_UID ?= $(shell id -u)

.PHONY: bake-images
bake-images:
	cd ../.. && BUILD_USER=$(BUILD_USER) BUILD_UID=$(BUILD_UID) \
		docker buildx bake --load -f docker/dist/docker-bake.hcl gfarm
	$(MAKE) -C http-gateway bake-images
