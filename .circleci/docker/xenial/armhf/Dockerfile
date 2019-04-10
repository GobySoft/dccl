FROM gobysoft/dccl-ubuntu-build-base:16.04.1

# replace amd64 lib*-dev packages with armhf (this is necessary until they are all multi-arch compatible)

# armhf depends, would rather use 'mk-build-deps --host-arch amd64 --build-arch armhf -t "apt-get -y --no-install-recommends -o Debug::pkgProblemResolver=yes" -i "debian/control"' but this doesn't work correctly due to the fact that mk-build-deps adds a dependency on build-essential:armhf
RUN apt-get update && apt-get install -y libboost-dev:armhf libprotobuf-dev:armhf libprotoc-dev:armhf libb64-dev:armhf libcrypto++-dev:armhf crossbuild-essential-armhf:amd64
