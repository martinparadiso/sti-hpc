#!/usr/bin/python3

from pathlib import Path
import argparse
import secrets
import subprocess
import time

parser = argparse.ArgumentParser(
    description='Simulation runner. Facilitates the execution of the simulation')
parser.add_argument('-n', '--nodes', default=4,
                    help='Number of process to run')
parser.add_argument(
    '--mpi', default='../lib/mpich/bin/mpiexec', help='MPI executable')
parser.add_argument('-e', '--executable', default='./sti-demo',
                    help='Simulation binary file')
parser.add_argument('--config', default='../data/config.props')
parser.add_argument('--model', default='../data/model.props')
parser.add_argument('--hospital', default='../data/hospital.json')
parser.add_argument('--debug', type=int,
                    help='Pass the --debug flag to the N process')

if __name__ == '__main__':
    args = parser.parse_args()

    # Create the output folder
    output_folder = Path('../output')
    if not output_folder.exists():
        output_folder.mkdir()

    # Generate a random id for the run
    run_id = secrets.token_hex(16)

    commands = [args.mpi,
                '-np', str(args.nodes),
                f"{args.executable}",
                f"{args.config}",
                f"{args.model}",
                f"run.id={run_id}",
                f"hospital.file={args.hospital}"]

    if args.debug is not None:
        commands.append(f"--debug={args.debug}")

    print(f"Running: {' '.join(commands)}")

    start_time = time.time()
    subprocess.run(commands)
    print(f"Program finished in {time.time() - start_time:.3f} seconds")
