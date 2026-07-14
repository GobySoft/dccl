# DCCL

The Dynamic Compact Control Language (DCCL) is a marshalling and protocol language for very low throughput links (e.g. acoustic, satellite, low frequency radio). Formerly part of Goby (https://launchpad.net/goby) for releases 1 and 2, we believe DCCL has reached a level of maturity that it can be more easily standardized, adopted, and maintained as a separate project.

- Documentation: http://libdccl.org
- Release Ubuntu/Debian Packages: http://packages.gobysoft.org/ubuntu/release/
- Continuous build Ubuntu/Debian Packages: http://packages.gobysoft.org/ubuntu/continuous/
- Assigned DCCL IDs table: http://gobysoft.org/wiki/DcclIdTable

## Packages

The only officially supported distributions are Debian (stable and oldstable) and Ubuntu (currently supported LTS releases). Packages for these releases are built for the amd64, arm64, and armhf architectures and uploaded to http://packages.gobysoft.org:

To install release packages on Ubuntu or Debian, install the appropriate apt sources.list and GobySoft PGP signing key:
```
export COMPONENT=release
export GOBYSOFT_SIGNING_KEY=19478082E2F8D3FE
install -d -m 0755 /etc/apt/keyrings
gpg --keyserver keyserver.ubuntu.com --recv-keys ${GOBYSOFT_SIGNING_KEY} && gpg --export ${GOBYSOFT_SIGNING_KEY} >/etc/apt/keyrings/gobysoft.gpg
echo "deb [signed-by=/etc/apt/keyrings/gobysoft.gpg] http://packages.gobysoft.org/$(. /etc/os-release; echo "$ID")/${COMPONENT}/ $(. /etc/os-release; echo "$VERSION_CODENAME")/" >/etc/apt/sources.list.d/gobysoft_${COMPONENT}.list
```

Then run:
```
sudo apt update
# minimal
sudo apt install libdccl5-dev
# full
sudo apt install libdccl5-dev dccl5-compiler dccl5-apps
```

Instead of the release repository, you can use the continuous repository (every commit to the main branch build):
```
export COMPONENT=continuous
echo "deb [signed-by=/etc/apt/keyrings/gobysoft.gpg] http://packages.gobysoft.org/$(. /etc/os-release; echo "$ID")/${COMPONENT}/ $(. /etc/os-release; echo "$VERSION_CODENAME")/" >/etc/apt/sources.list.d/gobysoft_${COMPONENT}.list
```

## Continuous Integration

[![CircleCI](https://circleci.com/gh/GobySoft/dccl.svg?style=svg)](https://circleci.com/gh/GobySoft/dccl)

### Security Metrics

[![OpenSSF Scorecard](https://api.scorecard.dev/projects/github.com/GobySoft/dccl/badge)](https://scorecard.dev/viewer/?uri=github.com/GobySoft/dccl)
[![OpenSSF Baseline](https://www.bestpractices.dev/projects/12204/baseline)](https://www.bestpractices.dev/projects/12204)
