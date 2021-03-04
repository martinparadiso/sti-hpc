#!/usr/bin/python3

import subprocess
import json
import re
from pathlib import Path
import glob
import argparse

# Available libraries
libraries = (
    'boost',
    'curl',
    'mpich',
    'netcdf',
    'netcdf-cxx'
    'repast'
)

parser = argparse.ArgumentParser(description='Library downloader')
parser.add_argument('libs', nargs='+', choices=(*libraries,
                                                'all'), help='Libraries to install')
parser.add_argument('--versions', default='versions.json',
                    help='File containing the library version')
parser.add_argument('--download-dir', default='./tmp',
                    help='Download directory')
parser.add_argument('--install-dir', default='.',
                    help='Installation directory')
parser.add_argument('-j', '--jobs', default=1, type=int,
                    help='Number of workers used for compilation')

parser.add_argument('--mpicxx', default='mpicxx',
                    help='MPI compiler needed by Boost')

# Library getters ==============================================================


class Library(object):
    pass


class Boost(Library):

    name = 'boost'
    requires = ('mpich')
    required_by = ('repast')

    # def __init__(self, version, download_folder, install_folder, mpi_compiler, jobs):
    def __init__(self, version, args):
        self.version = version.split('.')
        self.filename = f"boost_{'_'.join(version)}.tar.bz2"
        self.download_folder = args.download_folder
        self.install_folder = args.install_folder
        self.mpi_compiler = args.mpi_compiler
        self.jobs = args.jobs
        if (len(version) != 3):
            raise Exception(f"Invalid boost version {version}")

    def download(self):
        """Download the library to the given path"""

        self.file_path = f"{self.download_folder}/{self.filename}"

        # Check if file already exists
        if glob.glob(self.file_path):
            print('Boost already downloaded, skipping')
            return

        # Download boost with wget
        url = f"https://sourceforge.net/projects/boost/files/boost/{'.'.join(self.version)}/{self.filename}/download"

        wget_output = subprocess.run(['wget', url, '-O', self.file_path])

        if wget_output.returncode == 0:
            print(f"Boost {'.'.join(self.version)} downloaded")
        else:
            raise Exception(f"Boost {'.'.join(self.version)} download failed")

    def install(self):
        """Install the already downloaded library"""

        # Extract the file
        print('Extracting boost')
        tar_output = subprocess.run(['tar', '-xf', self.file_path,
                                     '--directory', self.download_folder])
        if tar_output.returncode != 0:
            raise Exception('Error extracting Boost')

        # Install boost
        print('Installing boost')
        workdir = f"{self.download_folder}/{self.filename[:-8]}"

        bootstrap = subprocess.run(['./bootstrap.sh',
                                    f"--prefix={self.install_folder}/boost/",
                                    '--with-libraries=system,filesystem,mpi,serialization'],
                                   cwd=workdir)

        if bootstrap.returncode != 0:
            raise Exception('Error while running Boost bootstrap')

        # Set the mpi version
        with open(f"{workdir}/project-config.jam", 'a') as config:
            config.write(f"using mpi : {self.mpi_compiler} ;")

        # Compile and install
        install_output = subprocess.run(['./b2',
                                         f"-j{self.jobs}",
                                         '--layout=tagged',
                                         'varient=release',
                                         'threading=multi',
                                         'stage', 'install'
                                         ])

        if install_output.returncode != 0:
            raise Exception(f"Error compiling and install Boost")

if (__name__ == '__main__'):
    args = parser.parse_args()

    # Load library version
    with open(args.versions) as f:
        lib_versions = json.load(f)

    # Install all
    if 'all' in args.libs:
        raise Exception('Unimplemented')

    for lib_str in args.libs:

        for LibClass in Library.__subclasses__():
            if LibClass.name == lib_str:
                lib = LibClass(lib_versions[lib_str], args)

        lib.download()
        lib.install()
