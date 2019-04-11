FROM ubuntu:bionic

WORKDIR /root

SHELL ["/bin/bash", "-c"]

RUN apt-get update && \
    apt-get -y upgrade && \
    apt-get -y --no-install-recommends install \
            git build-essential ssh cmake gpg gpg-agent \
            devscripts equivs ca-certificates quilt \
            clang clang-tools \
            dput \
            && rm -rf /var/lib/apt/lists/*

# Clone the Debian packaging project and install the dependencies it declares
RUN git clone \
    https://github.com/GobySoft/dccl-debian -b 3.0 debian    

ENV CROSS_COMPILE_ARCHS "arm64 armhf"

# Allow us to install packages for cross-compiling architectures
RUN ARCHS=(${CROSS_COMPILE_ARCHS}) && \
    ARCHS_LIST=$(IFS=,; echo "${ARCHS[*]}") && \
    for ARCH in "${ARCHS[@]}"; do dpkg --add-architecture "$ARCH"; done && \
    sed -i 's/deb /deb [arch=amd64] /' /etc/apt/sources.list && \
    egrep "^deb " /etc/apt/sources.list | sed "s/amd64/${ARCHS_LIST}/" \
        | sed 's|http://.*.ubuntu.com/ubuntu/|http://ports.ubuntu.com/|' \
        >> /etc/apt/sources.list

RUN ARCHS=(${CROSS_COMPILE_ARCHS}) && \
    apt-get update && \
    mk-build-deps -t "apt-get -y --no-install-recommends" -i "debian/control" && \    
    for ARCH in "${ARCHS[@]}"; \
    do mk-build-deps -a ${ARCH} --host-arch ${ARCH} --build-arch amd64 -t "apt-get -y --no-install-recommends -o Debug::pkgProblemResolver=yes" -i "debian/control"; \
    done && \
    rm -rf /var/lib/apt/lists/*

# Overwrite non-multiarch packages
RUN apt-get update && \
    apt-get -y install libcrypto++-dev && \
    rm -rf /var/lib/apt/lists/*
