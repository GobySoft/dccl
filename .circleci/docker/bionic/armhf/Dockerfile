FROM gobysoft/dccl-ubuntu-build-base:18.04.1

# replace amd64 lib*-dev packages with armhf (this is necessary until they are all multi-arch compatible)

# armhf depends, would rather use 'mk-build-deps --host-arch amd64 --build-arch armhf -t "apt-get -y --no-install-recommends -o Debug::pkgProblemResolver=yes" -i "debian/control"' but this doesn't work correctly due to the fact that mk-build-deps adds a dependency on build-essential:armhf
RUN apt-get update && apt-get install -y libboost-dev:armhf libprotobuf-dev:armhf libprotoc-dev:armhf libb64-dev:armhf libcrypto++-dev:armhf crossbuild-essential-armhf:amd64

# Bug in Bionic: if this file isn't removed, we get
# dpkg: error processing archive /var/cache/apt/archives/libstdc++-7-dev_7.3.0-27ubuntu1~18.04_armhf.deb (--unpack):
#   trying to overwrite shared '/usr/include/c++/7/ext/random', which is different from other instances of package libstdc++-7-dev:armhf
RUN rm /usr/include/c++/7/ext/random && apt-get install -y libstdc++-7-dev:armhf
