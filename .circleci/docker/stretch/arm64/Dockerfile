FROM gobysoft/dccl-debian-build-base:9.1

# replace amd64 lib*-dev packages with arm64 (this is necessary until they are all multi-arch compatible)

# arm64 depends, would rather use 'mk-build-deps --host-arch amd64 --build-arch arm64 -t "apt-get -y --no-install-recommends -o Debug::pkgProblemResolver=yes" -i "debian/control"' but this doesn't work correctly due to the fact that mk-build-deps adds a dependency on build-essential:arm64
RUN apt-get update && apt-get install -y libboost-dev:arm64 libprotobuf-dev:arm64 libprotoc-dev:arm64 libb64-dev:arm64 libcrypto++-dev:arm64 crossbuild-essential-arm64:amd64
