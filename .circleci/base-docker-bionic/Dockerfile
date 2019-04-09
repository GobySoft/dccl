FROM ubuntu:bionic

WORKDIR /root

SHELL ["/bin/bash", "-c"]

RUN apt-get update && \
    apt-get -y upgrade && \
    apt-get -y --no-install-recommends install \
            git build-essential ssh cmake \
            devscripts equivs ca-certificates

# Allow us to install ARM64 packages as well
RUN ARCHS=(arm64) && \
    ARCHS_LIST=$(IFS=,; echo "${ARCHS[*]}") && \
    for ARCH in "${ARCHS[@]}"; do dpkg --add-architecture "$ARCH"; done && \
    sed -i 's/deb /deb [arch=amd64] /' /etc/apt/sources.list && \
    egrep "^deb " /etc/apt/sources.list | sed "s/amd64/${ARCHS_LIST}/" \
        | sed 's|http://.*.ubuntu.com/ubuntu/|http://ports.ubuntu.com/|' \
        >> /etc/apt/sources.list

# Clone the Debian packaging project and install the dependencies it declares
RUN git clone \
    https://github.com/GobySoft/dccl-debian -b 3.0 debian
RUN apt-get update && \
    mk-build-deps -t "apt-get -y --no-install-recommends" -i "debian/control"
