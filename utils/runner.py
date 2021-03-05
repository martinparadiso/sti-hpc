#!/usr/bin/python3

import argparse
import subprocess

parser = argparse.ArgumentParser(description='Simulation runner')
parser.add_argument('-n', '--nodes', default=4)
parser.add_argument('--mpi', default='/home/martin/repast/mpich/bin/mpiexec')
parser.add_argument('executable', default='./sti-demo')
parser.add_argument('--config', default='./config.props')
parser.add_argument('--model', default='./model.props')
parser.add_argument('--debug', action='store_true')

if __name__ == '__main__':
    args = parser.parse_args()

    commands = [args.mpi,
                    '-np', str(args.nodes),
                    f"./{args.executable}",
                    f"./{args.config}",
                    f"./{args.model}"]

    if args.debug:
        commands.append('--debug')

    subprocess.run(commands)

