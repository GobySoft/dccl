# DCCL

The Dynamic Compact Control Language (DCCL) is a marshalling and protocol language for very low throughput links (e.g. acoustic, satellite, low frequency radio). Formerly part of Goby (https://launchpad.net/goby) for releases 1 and 2, we believe DCCL has reached a level of maturity that it can be more easily standardized, adopted, and maintained as a separate project.

- Documentation: http://libdccl.org
- Release Ubuntu/Debian Packages: http://packages.gobysoft.org/ubuntu/release/
- Continuous build Ubuntu/Debian Packages: http://packages.gobysoft.org/ubuntu/continuous/
- Assigned DCCL IDs table: http://gobysoft.org/wiki/DcclIdTable

## Packages

The only officially supported distributions are Debian (stable and oldstable) and Ubuntu (currently supported LTS releases). Packages for these releases are built for the amd64, arm64, and armhf architectures and uploaded to http://packages.gobysoft.org:

To install release packages on Ubuntu, run:
```
echo "deb http://packages.gobysoft.org/ubuntu/release/ `lsb_release -c -s`/" | sudo tee /etc/apt/sources.list.d/gobysoft_release.list
```

or for Debian:
```
echo "deb http://packages.gobysoft.org/debian/release/ `lsb_release -c -s`/" | sudo tee /etc/apt/sources.list.d/gobysoft_release.list
```

In both cases, then run:
```
sudo apt-key adv --recv-key --keyserver keyserver.ubuntu.com 19478082E2F8D3FE
sudo apt update
# minimal
sudo apt install libdccl4-dev
# full
sudo apt install libdccl4-dev dccl4-compiler dccl4-apps
```

Instead of the release repository, you can use the continuous repository (every commit to the main branch build) by adding to your apt sources:
```
deb http://packages.gobysoft.org/[ubuntu|debian]/continuous/ {release-codename}/
```

## Continuous Integration

[![CircleCI](https://circleci.com/gh/GobySoft/dccl.svg?style=svg)](https://circleci.com/gh/GobySoft/dccl)
