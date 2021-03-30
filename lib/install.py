#!/usr/bin/python3

import subprocess
import json
import re
from pathlib import Path
import glob
import argparse
import logging
import os

# Available libraries
libraries = (
    'curl',
    'mpich',
    'netcdf',
    'netcdf-cxx',
    'boost',
    'repast'
)

# Library getters ==============================================================


def check_run(program_return, report):
    if program_return.returncode != 0:
        raise Exception(report)


class Library(object):

    def download(self):
        """Download the given Library"""

        self.file_path = f"{self.download_folder}/{self.filename}"

        # Check if file already exists
        if glob.glob(self.file_path):
            logging.info(f"{self.name} already downloaded, skipping")
            return

        # Download boost with wget
        url = self.download_url()
        logging.info(f"Downloading {self.name} from {url}")

        wget_output = subprocess.run(['wget', url, '-O', self.file_path])

        if wget_output.returncode == 0:
            print(f"{self.name} {'.'.join(self.version)} downloaded")
        else:
            raise Exception(
                f"{self.name} {'.'.join(self.version)} download failed")

    def extract(self):
        """Extact the already downloaded file"""

        logging.info(f"Extracting {self.name}")
        tar_output = subprocess.run(['tar', '-xf', self.file_path,
                                     '--directory', self.download_folder])

        check_run(tar_output, f"Error extracting {self.name}")

    def generic_make_install(self, workdir, cfg_cmd, extra_env={}, extract=True):
        """Install the already downloaded library"""
        if extract:
            self.extract()

        logging.info(f"Installing {self.name}")

        env = os.environ.copy()

        if self.requires:
            # Manually set the curl include and lib folders
            env['LDFLAGS'] = ' '.join(
                [f"-L{self.install_folder}/{l}/lib" for l in self.requires])
            env['CPPFLAGS'] = ' '.join(
                [f"-I{self.install_folder}/{l}/include" for l in self.requires])

        if self.debug:
            if 'CPPFLAGS' in env:
                env['CPPFLAGS'] += (' -g')
            else:
                env['CPPFLAGS'] = '-g'

        env = {**env, **extra_env}

        # Some libraries don't use configure, so check first
        if cfg_cmd:
            configure_output = subprocess.run(cfg_cmd,
                                              cwd=workdir,
                                              env=env)

            check_run(configure_output, f"Error configuring {self.name}")

        compile_output = subprocess.run(['make', '-j', str(self.jobs), 'all'],
                                        cwd=workdir,
                                        env=env)

        check_run(compile_output, f"Error compiling {self.name}")

        install_output = subprocess.run(
            ['make', 'install'], cwd=workdir, env=env)

        check_run(install_output, f"Error installing {self.name}")


class Curl(Library):

    name = 'curl'
    requires = ()

    def __init__(self, version, download_folder, install_folder, args):
        self.version = version.split('.')
        self.filename = f"curl-{'.'.join(self.version)}.tar.bz2"

        self.download_folder = download_folder
        self.install_folder = install_folder
        self.jobs = args.jobs
        self.debug = args.debug

    def download_url(self):
        return f"https://curl.haxx.se/download/{self.filename}"

    def install(self):
        """Install the already downloaded library"""
        # Subscripting to remove the trailing extension '.tar.gz'
        workdir = f"{self.download_folder}/{self.filename[:-8]}"

        super().generic_make_install(workdir,
                                     ['./configure',
                                      f"--prefix={self.install_folder}/{self.name}"])


class MPICH(Library):

    name = 'mpich'
    requires = ('curl')

    def __init__(self, version, download_folder, install_folder, args):
        self.version = version.split('.')
        self.filename = f"mpich-{'.'.join(self.version)}.tar.gz"
        self.download_folder = download_folder
        self.install_folder = install_folder
        self.jobs = args.jobs
        self.debug = args.debug

    def download_url(self):
        return f"https://www.mpich.org/static/downloads/{'.'.join(self.version)}/{self.filename}"

    def install(self):
        workdir = f"{self.download_folder}/{self.filename[:-7]}"

        super().generic_make_install(workdir,
                                     ['./configure',
                                      f"--prefix={self.install_folder}/{self.name}/",
                                      '--disable-fortran'])


class Netcdf(Library):

    name = 'netcdf'
    requires = ('curl')

    def __init__(self, version, download_folder, install_folder, args):
        self.version = version.split('.')
        self.filename = f"netcdf-{'.'.join(self.version)}.tar.gz"
        self.download_folder = download_folder
        self.install_folder = install_folder
        self.jobs = args.jobs
        self.debug = args.debug

    def download_url(self):
        return f"ftp://ftp.unidata.ucar.edu/pub/netcdf/old/{self.filename}"

    def install(self):
        workdir = f"{self.download_folder}/{self.filename[:-7]}"

        super().generic_make_install(workdir, ['./configure',
                                               f"--prefix={self.install_folder}/{self.name}/",
                                               '--disable-netcdf-4'])


class Netcdfcxx(Library):

    name = 'netcdf-cxx'

    requires = ('curl', 'netcdf')

    def __init__(self, version, download_folder, install_folder, args):
        self.version = version.split('.')
        self.filename = f"netcdf-cxx-{'.'.join(self.version)}.tar.gz"
        self.download_folder = download_folder
        self.install_folder = install_folder
        self.jobs = args.jobs
        self.debug = args.debug

    def download_url(self):
        return f"ftp://ftp.unidata.ucar.edu/pub/netcdf/{self.filename}"

    def install(self):
        workdir = f"{self.download_folder}/{self.filename[:-7]}"

        super().generic_make_install(workdir, ['./configure',
                                               f"--prefix={self.install_folder}/{self.name}/"])


class Boost(Library):

    name = 'boost'
    requires = ('mpich')

    def __init__(self, version, download_folder, install_folder, args):
        self.version = version.split('.')
        self.filename = f"boost_{'_'.join(self.version)}.tar.bz2"
        self.download_folder = download_folder
        self.install_folder = install_folder
        self.mpi_compiler = args.mpi_compiler
        self.jobs = args.jobs
        self.debug = args.debug

    def download_url(self):
        return f"https://sourceforge.net/projects/boost/files/boost/{'.'.join(self.version)}/{self.filename}/download"

    def install(self):
        """Install the already downloaded library"""
        super().extract()

        logging.info(f"Installing {self.name}")
        workdir = f"{self.download_folder}/{self.filename[:-8]}"

        bootstrap = subprocess.run(['./bootstrap.sh',
                                    f"--prefix={self.install_folder}/boost/",
                                    '--with-libraries=system,filesystem,mpi,serialization'],
                                   cwd=workdir)

        if bootstrap.returncode != 0:
            raise Exception('Error while running Boost bootstrap')

        # Set the mpi version explicitly
        with open(f"{workdir}/project-config.jam", 'a') as config:
            config.write(f"using mpi : {self.mpi_compiler} ;")

        # Compile and install
        boost_command = ['./b2',
                         f"-j{self.jobs}",
                         '--layout=tagged',
                         'variant=release',
                         'threading=multi',
                         'stage', 'install'
                         ]
        if self.debug:
            boost_command.append('--debug-symbols=on')
        install_output = subprocess.run(boost_command,
                                        cwd=workdir)

        check_run(install_output, f"Error compiling and install Boost")


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
        self.debug = args.debug

        if self.version != ['2', '3', '1']:
            raise Exception('Unsupported repast version')

    def download(self):

        repo_folder = Path(f"{self.download_folder}/repast.hpc")

        if repo_folder.is_dir():
            git_output = subprocess.run(['git', 'pull'], cwd=repo_folder)
        else:
            git_output = subprocess.run(['git',
                                         'clone', 'https://github.com/martinparadiso/repast.hpc.git',
                                         repo_folder.absolute()])
        
        git_switch = subprocess.run(['git',
                                        'switch', 'development'],
                                        cwd=repo_folder.absolute())

        check_run(git_output, 'Error cloning repast repository')
        check_run(git_switch, 'Error switching to development branch')
    def install(self):

        workdir = Path(f"{self.download_folder}/repast.hpc/release")
        try:
            workdir.mkdir()
        except:
            pass

        # Depending of the compilation type, copy the debug or release makefile
        if self.debug:
            cp_output = subprocess.run(
                ['cp', f"{self.download_folder}/../Makefile.debug.repast", './Makefile'], cwd=workdir)
        else: 
            cp_output = subprocess.run(
                            ['cp', f"{self.download_folder}/../Makefile.release.repast", './Makefile'], cwd=workdir)
                            
        check_run(cp_output, 'Error copying repast Makefile')

        env_vars = {
            'CC': self.mpi_compiler,
            'BASE_DIR': self.install_folder,
            'PREFIX': f"{self.install_folder}/{self.name}"
        }

        super().generic_make_install(workdir, [],
                                     extra_env=env_vars, extract=False)


if (__name__ == '__main__'):

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
    parser.add_argument('-g', '--debug', action='store_true',
                        help='Pass the -g flag to the compiler to generate debug info')
    parser.add_argument('--mpi-compiler', default='./mpich/bin/mpicxx',
                        help='MPI compiler needed by Boost')

    logging.getLogger().setLevel(logging.DEBUG)
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

    # Set the mpi path as absolute
    args.mpi_compiler = Path(args.mpi_compiler).absolute()

    # Install all
    if 'all' in args.libs:
        args.libs = libraries

    for lib_str in args.libs:

        for LibClass in Library.__subclasses__():
            if LibClass.name == lib_str:
                lib = LibClass(lib_versions[lib_str],
                               download_folder.absolute(),
                               install_folder.absolute(),
                               args)

        lib.download()
        lib.install()
