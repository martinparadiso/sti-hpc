# sti-hpc

Simulador de Transmisiones Intrahospitalarias para HPC

## Compiling

### Ubuntu

Install required packages from Ubuntu repositories:

```sh
$ apt install -y git build-essential cmake clang clang-tidy libomp wget tar bzip2 diffutils zlib1g-dev libgtest-dev
```

Install the rest of the libraries, optionally pass the number of jobs used to
compile the libraries:

```sh
cd lib
./install.py all [-j J]
```
