# Bake definition for the gfarm-http-gateway image used by docker/dist.
# Kept next to docker-compose.yaml, which uses the same image name.
#
# Prepared for GitHub Actions (docker/bake-action). Locally, `make up` in this directory builds
# the same image with compose, so bake is optional for local use.
#
# Keep the tag and context in sync with docker-compose.yaml (service http-gw: image and build.context).
#
# Run bake from the gfarm repository root (see docker/dist/docker-bake.hcl for the full command).
# Relative context paths are resolved from the current working directory.
#
# The gfarm-http-gateway source is not in this repository. Its Dockerfile is read from
# gfarm-http-gateway/server, the same context as GFARM_HTTP_GATEWAY_CONTEXT in .env.
#
# Target and group names are shared across merged files; do not reuse "gateway" in other files.
#
# Kept separate from docker-compose.yaml for the reason given in docker/dist/docker-bake.hcl.

group "gateway" {
  targets = ["gateway"]
}

# Same image as the http-gw service in docker-compose.yaml.
target "gateway" {
  context = "gfarm-http-gateway/server"
  tags    = ["gfarm-http-gateway-docker-dist"]
}
