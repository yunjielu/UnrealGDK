# Used for the license scan job

FROM ubuntu:16.04

# Install build dependency packages.
# Keep these alpha-sorted.
# The remove command at the end is because Chrome adds its own PPA when it installs, so we get duplicate sources unless we nuke one.
RUN apt-get -qq update && \
  apt-get -qq install -y  --no-install-recommends \
  apt-transport-https \
  curl \
  openjdk-8-jdk \
  software-properties-common \
  rm -rf /var/lib/apt/lists/*

# Download and install gosu, so we can drop privs after the container starts.
RUN curl -LSs -o /usr/local/bin/gosu -SL "https://github.com/tianon/gosu/releases/download/1.4/gosu-$(dpkg --print-architecture)" \
  && chmod +x /usr/local/bin/gosu

# Copy the entrypoint into the image and set it as the target.
COPY bin/ /usr/local/bin
ENTRYPOINT ["/usr/local/bin/entrypoint.sh"]
