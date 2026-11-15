# Bake definitions for the docker/dist images owned by the gfarm repository.
#
# Prepared for GitHub Actions (docker/bake-action). Local runs can build these images with
# compose alone, so bake is optional for local use.
#
# Why not bake the compose files directly: they set build contexts relative to their own
# directories, but bake resolves contexts from the working directory. The contexts here are
# relative to the gfarm root, so one invocation builds all targets in parallel.
#
# Keep this file in sync with docker-compose.yaml and jwt-server/docker-compose.yaml: image
# names (tags), build contexts, dockerfile paths, and build args must match the compose files.
#
# Run bake from the gfarm repository root. Relative context paths are resolved from the
# current working directory, not from this file's location.
#   docker buildx bake -f docker/dist/docker-bake.hcl -f docker/dist/http-gateway/docker-bake.hcl gfarm gateway
# The gfarm group builds: minica, gfarm-ubuntu, jwt-server-back, setup-keycloak.
#
# Requirements:
#   - Set BUILD_USER and BUILD_UID to the Docker build user (environment variables, default empty).
#   - Set PROXY_HOST and PROXY_PORT if a proxy is needed (environment variables, default empty).
#   - Target and group names are shared across merged files; do not reuse "gfarm" in other files.

variable "BUILD_USER" {
  default = ""
}

variable "BUILD_UID" {
  default = ""
}

variable "PROXY_HOST" {
  default = ""
}

variable "PROXY_PORT" {
  default = ""
}

group "gfarm" {
  targets = ["minica", "gfarm-ubuntu", "jwt-server-back", "setup-keycloak"]
}

# Same image as the minica service in docker/dist/minica.
target "minica" {
  context = "docker/dist/minica"
  tags    = ["gfarm-minica"]
}

# Same image as docker/dist/docker-compose.yaml (DIST=ubuntu).
target "gfarm-ubuntu" {
  context    = "."
  dockerfile = "docker/dist/ubuntu/Dockerfile"
  tags       = ["gfarm-ubuntu"]
  args = {
    USERNAME = BUILD_USER
    UID      = BUILD_UID
  }
}

# Same image as docker/dist/jwt-server/docker-compose.yaml service jwt-server-back.
target "jwt-server-back" {
  context = "docker/dist/jwt-server/jwt-server"
  tags    = ["gfarm-jwt-server:local"]
  args = {
    MAVEN_OPTS = "-Dhttp.proxyHost=${PROXY_HOST} -Dhttp.proxyPort=${PROXY_PORT} -Dhttps.proxyHost=${PROXY_HOST} -Dhttps.proxyPort=${PROXY_PORT}"
  }
}

# Same image as docker/dist/jwt-server/docker-compose.yaml service setup-keycloak.
target "setup-keycloak" {
  context = "docker/dist/jwt-server/setup-keycloak"
  tags    = ["gfarm-setup-keycloak"]
}
