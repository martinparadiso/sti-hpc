#!/usr/bin/python3

import subprocess
import json
import re
from pathlib import Path
import glob
import argparse
import os

# Available libraries
libraries = (
    'boost',
    'curl',
    'mpich',
    'netcdf',
    'netcdf-cxx',
    'repast'
)

parser = argparse.ArgumentParser(description='Library downloader. '
                                 'To compile with an specific compiler, pass CC and CXX env variables as usual.')
parser.add_argument('libs', nargs='+', choices=(*libraries,
                                                'all'), help='Libraries to install')
parser.add_argument('--versions', default='versions.json',
                    help='File containing the library version')
parser.add_argument('--download-folder', default='./tmp',
                    help='Download directory')
parser.add_argument('--install-folder', default='.',
                    help='Installation directory')
parser.add_argument('-j', '--jobs', default=1, type=int,
                    help='Number of workers used for compilation')

parser.add_argument('--mpi-compiler', default='mpicxx',
                    help='MPI compiler needed by Boost')

# Library getters ==============================================================


class Library(object):
    pass


class Boost(Library):

    name = 'boost'
    requires = ('mpich')
    required_by = ('repast')

    # def __init__(self, version, download_folder, install_folder, mpi_compiler, jobs):
    def __init__(self, version, download_folder, install_folder, args):
        self.version = version.split('.')
        self.filename = f"boost_{'_'.join(self.version)}.tar.bz2"
        self.download_folder = download_folder
        self.install_folder = install_folder
        self.mpi_compiler = args.mpi_compiler
        self.jobs = args.jobs

        if (len(self.version) != 3):
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
                                         'variant=release',
                                         'threading=multi',
                                         'stage', 'install'
                                         ],
                                        cwd=workdir)

        if install_output.returncode != 0:
            raise Exception(f"Error compiling and install Boost")


class Repast(Library):

    name = 'repast'
    requires = ('boost', 'curl', 'mpich', 'netcdf', 'netcdf-cxx')
    required_by = ()

    def __init__(self, version, download_folder, install_folder, args):
        self.version = version.split('.')
        self.download_folder = download_folder
        self.install_folder = install_folder
        self.jobs = args.jobs
        self.mpi_compiler = args.mpi_compiler

        if version != '2.3.1':
            raise Exception('Unsupported repast version')

    def download(self):

        repo_folder = Path(f"{self.download_folder}/repast.hpc")

        if repo_folder.is_dir():
            git_output = subprocess.run(['git', 'pull'], cwd=repo_folder)
        else:
            git_output = subprocess.run(['git',
                                         'clone', 'https://github.com/martinparadiso/repast.hpc.git',
                                         repo_folder.absolute()])

        if git_output.returncode != 0:
            raise Exception('Error cloning repast repository')

    def install(self):

        workdir = Path(f"{self.download_folder}/repast.hpc/release")
        try:
            workdir.mkdir()
        except:
            pass

        # TODO: Temporary solution for the makefile
        cp_output = subprocess.run(['cp', f"{self.download_folder}/../Makefile.repast", './Makefile'], cwd=workdir)

        if cp_output.returncode != 0:
            raise Exception('Error copying repast Makefile')

        
        # TODO: Temporary soluciton for the makefile
        env = os.environ.copy()
        env['CC'] = self.mpi_compiler
        env['PREFIX'] = f"{self.install_folder}/repast/"

        compile_output = subprocess.run(['make',
                                         f"-j{self.jobs}",
                                         'all'],
                                        cwd=workdir,
                                        env=env)

        if compile_output.returncode != 0:
            raise Exception('Error compiling repast')

        install_output = subprocess.run(['make',
                                         'install'],
                                        cwd=workdir,
                                        env=env)

        if install_output.returncode != 0:
            raise Exception('Error installing repast')


if (__name__ == '__main__'):
    args = parser.parse_args()

    # Load libraries versions
    with open(args.versions) as f:
        lib_versions = json.load(f)

    # Create download folder
    download_folder = Path(args.download_folder)
    if not download_folder.is_dir():
        download_folder.mkdir()
    print(f"Download folder: {download_folder.absolute()}")

    # Create install folder
    install_folder = Path(args.install_folder)
    if not install_folder.is_dir():
        install_folder.mkdir()
    print(f"Install folder: {install_folder.absolute()}")

    # Install all
    if 'all' in args.libs:
        raise Exception('Unimplemented')

    for lib_str in args.libs:

        for LibClass in Library.__subclasses__():
            if LibClass.name == lib_str:
                lib = LibClass(lib_versions[lib_str],
                               download_folder.absolute(),
                               install_folder.absolute(),
                               args)

        lib.download()
        lib.install()
