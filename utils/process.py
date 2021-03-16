#!/usr/bin/python3

import argparse
import json
import matplotlib
import matplotlib.pyplot as plt
import numpy as np

parser = argparse.ArgumentParser(description='Simulation output processor')
parser.add_argument('--hospital',
                    default='../data/hospital.json',
                    help='The hospital used in the simulation')
parser.add_argument('--exit',
                    default='../output/exit.json',
                    help='The exit file generated')
subparsers = parser.add_subparsers(help='Subcommands')


def plot_path(args):

    with open(args.hospital) as f:
        hospital = json.load(f)

    with open(args.exit) as f:
        exit_data = json.load(f)

    agent_id = args.id

    # fig, ax = plt.subplots(figsize=(50, 20))
    # ax.plot([0, 3, 4], [4, 5, 6])
    # ax.grid()
    # fig.show()

    fig, ax = plt.subplots(figsize=(6, 6))

    plt.xlim([0, hospital['building']['width']])
    plt.ylim([0, hospital['building']['height']])
    ax.grid()

    # Get the path
    prev_point = (hospital['building']['entry']['x'] + 0.5,
                  hospital['building']['entry']['y'] + 0.5)
    path = []
    for point in exit_data[agent_id]['path']:
        new_point = (point['x'], point['y'])
        # print(f"[{prev_point}] -> [{new_point}]")
        path.append((prev_point, new_point))
        prev_point = new_point

    # Add the arrows
    for vector in path:
        sx, sy = vector[0]
        ex, ey = vector[1]
        lx = ex - sx
        ly = ey - sy
        print(f"[{sx:.2f},{sy:.2f}] -> [{ex:.2f}, {ey:.2f}] === {np.linalg.norm([lx, ly]):.2f}")
        ax.arrow(sx, sy, lx, ly, length_includes_head=True,
                 head_width=0.1,
                 head_length=0.2,
                 fc='k',
                 ec='k')

    plt.show()


plot_path_parser = subparsers.add_parser(
    'plot_path', help='Plot the path travelled by a given agent')
plot_path_parser.add_argument('id', help='The id of the Agent to plot')
plot_path_parser.set_defaults(func=plot_path)

if __name__ == '__main__':
    args = parser.parse_args()
    args.func(args)
