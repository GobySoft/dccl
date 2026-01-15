# Build and Install

This repo builds the C++ library and a Python extension that links against it. The Python
package expects the C++ build outputs in `./build`.

## Prerequisites
- CMake + a C++17-capable compiler
- Protobuf (compiler + headers + libs)
- Python >= 3.12 with pip
- `pybind11` (installed via pip or system package)

Optional features: Boost, Crypto++/B64/Lua (see `CMakeLists.txt`).

## C++ Build (Library + Tools)
From the repo root:
```bash
./build.sh
```

Optional install:
```bash
cd build
sudo make install
```

You can pass extra CMake or Make flags via env vars:
```bash
DCCL_CMAKE_FLAGS="-Denable_testing=ON" DCCL_MAKE_FLAGS="-j" ./build.sh
```

## Python Build / Install
This uses a pybind11 extension that links against the C++ build outputs.

1) Build the C++ library first (see above).

2) Install the Python package from `python/`:
```bash
cd python
python -m pip install -e .
```

Notes:
- The build expects headers in `./build/include` and libs in `./build/lib`.
- If you are using a non-default toolchain or build directory, update `python/setup.py`
  (or `python/setup.py.in` if using CMake to generate it).
