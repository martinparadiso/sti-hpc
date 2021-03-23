#!/usr/bin/python3

import argparse
import subprocess
import time
from pathlib import Path

parser = argparse.ArgumentParser(
    description='Simulation runner. Facilitates the execution of the simulation')
parser.add_argument('-n', '--nodes', default=4,
                    help='Number of process to run')
parser.add_argument(
    '--mpi', default='../lib/mpich/bin/mpiexec', help='MPI executable')
parser.add_argument('-e', '--executable', default='./sti-demo',
                    help='Simulation binary file')
parser.add_argument('--config', default='./config.props')
parser.add_argument('--model', default='./model.props')
parser.add_argument('--debug', type=int,
                    help='Pass the --debug flag to the N process')

if __name__ == '__main__':
    args = parser.parse_args()

    # Create the output folder
    output_folder = Path('../output')
    if not output_folder.exists():
        output_folder.mkdir()

    commands = [args.mpi,
                '-np', str(args.nodes),
                f"./{args.executable}",
                f"./{args.config}",
                f"./{args.model}"]

    if args.debug is not None:
        commands.append(f"--debug={args.debug}")

    print(f"Running: {' '.join(commands)}")

    start_time = time.time()
    subprocess.run(commands)
    print(f"Program finished in {time.time() - start_time:.2} seconds")
