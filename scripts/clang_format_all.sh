#!/bin/bash

cd ../src
find . -regex '.*\.\(cpp\|hpp\|h\|cc\|cxx\|proto\)' -type f | grep --invert-match "thirdparty" | parallel clang-format -style=file -i {} --verbose
