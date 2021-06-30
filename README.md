# sti-hpc

Simulador de Transmisiones Intrahospitalarias para HPC

## Compiling

### Ubuntu

Note: The following commands use clang as the compiler.

```sh
# Optional
export CC=clang
export CXX=clang++

# Install dependencies
sudo apt install -y python3-pip git build-essential cmake clang clang-tidy libomp-dev wget tar bzip2 diffutils zlib1g-dev libgtest-dev
sudo pip install -r requirements.txt

# Install the rest of the libraries
cd lib
python3 install.py all -j4

# Compile
cd .. && mkdir build && cd build/
export CC=../lib/mpich/bin/mpicc
export CXX=../lib/mpich/bin/mpicxx
cmake .. -DCMAKE_BUILD_TYPE=Release -DENABLE_LTO=On
cmake --build . -j4
```

