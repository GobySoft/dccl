# creates gobysoft/dccl-ubuntu-build-base:22.04.N
FROM gobysoft/ubuntu-build-base:22.04.1

WORKDIR /root

SHELL ["/bin/bash", "-c"]

# Clone the Debian packaging project and install the dependencies it declares
RUN git clone \
    https://github.com/GobySoft/dccl-debian -b 3.0 debian    

RUN ARCHS=(${CROSS_COMPILE_ARCHS}) && \
    apt-get update && \
    mk-build-deps -t "apt-get -y --no-install-recommends" -i "debian/control" && \    
    for ARCH in "${ARCHS[@]}"; \
    do mk-build-deps -a ${ARCH} --host-arch ${ARCH} --build-arch amd64 -t "apt-get -y --no-install-recommends -o Debug::pkgProblemResolver=yes" -i "debian/control"; \
    done && \
    rm -rf /var/lib/apt/lists/*
