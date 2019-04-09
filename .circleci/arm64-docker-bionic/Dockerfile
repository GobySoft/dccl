FROM gobysoft/dccl-ubuntu-build-base:18.04.1

# replace amd64 lib*-dev packages with arm64 (this is necessary until they are all multi-arch compatible)

# arm64 depends, would rather use 'mk-build-deps --host-arch amd64 --build-arch arm64 -t "apt-get -y --no-install-recommends -o Debug::pkgProblemResolver=yes" -i "debian/control"' but this doesn't work correctly due to the fact that mk-build-deps adds a dependency on build-essential:arm64
RUN apt-get update && apt-get install -y libboost-dev:arm64 libprotobuf-dev:arm64 libprotoc-dev:arm64 libb64-dev:arm64 libcrypto++-dev:arm64 crossbuild-essential-arm64:amd64

# So we can run our protoc-gen-dccl executable
RUN apt-get update && apt-get install -y qemu-user-static

# Bug in Bionic: if this file isn't removed, we get
# dpkg: error processing archive /var/cache/apt/archives/libstdc++-7-dev_7.3.0-27ubuntu1~18.04_arm64.deb (--unpack):
#   trying to overwrite shared '/usr/include/c++/7/ext/random', which is different from other instances of package libstdc++-7-dev:arm64
RUN rm /usr/include/c++/7/ext/random && apt-get install -y libstdc++-7-dev:arm64
