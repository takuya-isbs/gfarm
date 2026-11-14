TERM_INHERIT = $${TERM+--env TERM='${TERM}'}

login:
	docker exec -u ${USER} $(TERM_INHERIT) \
		-w /home/${USER}/gfarm/docker/dist -it gfarm-c1 /bin/bash

down:
	$(MAKE) -C jwt-server down
	docker compose down -t 2
