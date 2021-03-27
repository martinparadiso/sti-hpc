#!/usr/bin/python3

import matplotlib.pyplot as plt
import pandas as pd
import json
import numpy as np
import argparse
from collections import namedtuple

parser = argparse.ArgumentParser(
    description='Print and plot the performance metrics')
parser.add_argument('--folder', default='./output/',
                    help='The folder containing the CSV with the metrics')
parser.add_argument('-n', default=1, type=int,
                    help='Number of the process to analyze, each process must have an independent CSV file')
parser.add_argument('-p', '--process', default=0,
                    help='If the number of process to plot is 1, choose which process to draw')
parser.add_argument('-f', '--file',
                    help='Load an specific file, ignoring the rest of the arguments')
args = parser.parse_args()

Metrics = namedtuple('Metrics', ('average_tick_time',
                                 'average_mpi_sync_time',
                                 'average_rhpc_sync_time',
                                 'average_logic_time',
                                 'average_limbo_time'))


def extract_metrics(filepath) -> Metrics:
    """Extract the relevant metrics from a CSV file passed by parameter"""
    df = pd.read_csv(f"{filepath}")

    tick_start_time = df['tick_start_time'].to_numpy()
    tick_ns = tick_start_time[1:] - tick_start_time[:-1]
    avg_tick_ns = np.average(tick_ns)
    avg_mpi_sync_ns = np.average(df['mpi_sync_ns'].to_numpy())
    avg_rhpc_sync_ns = np.average(df['rhpc_sync_ns'].to_numpy())
    avg_logic_ns = np.average(df['logic_ns'].to_numpy())
    avg_limbo_time = avg_tick_ns - avg_logic_ns - avg_mpi_sync_ns - avg_rhpc_sync_ns

    return Metrics(avg_tick_ns, avg_mpi_sync_ns, avg_rhpc_sync_ns, avg_logic_ns, avg_limbo_time)

if args.n == 4:

    fig, axes = plt.subplots(2, 2)
    fig.suptitle('Performance of the 4 process')

    p = 0
    for ax in axes.flatten():
        metrics = extract_metrics(f"{args.folder}/process_{p}.csv")
        
        labels = ('Unknown', 'Logic', 'Non-RHPC Sync',
                'RHPC Sync')
        sizes = (metrics.average_limbo_time,
                metrics.average_logic_time,
                metrics.average_mpi_sync_time,
                metrics.average_rhpc_sync_time)

        ax.pie(sizes, labels=labels, autopct='%1.1f%%')
        ax.axis('equal')

        ax.set_title(f"Process {p}")

        p += 1
    
    plt.show()

if args.n == 1 or args.file is not None:
    # Process and plot the process 0
    if args.file is not None:
        metrics = extract_metrics(f"{args.file}")
    else:
        metrics = extract_metrics(f"{args.folder}/process_{args.process}.csv")

    print(f"Average tick time:      {metrics.average_tick_time:8.2f}ns")
    print(f"Average MPI sync time:  {metrics.average_mpi_sync_time:8.2f}ns")
    print(f"Average RHPC sync time: {metrics.average_rhpc_sync_time:8.2f}ns")
    print(f"Average logic time:     {metrics.average_logic_time:8.2f}ns")
    print(f"Limbo time:             {metrics.average_limbo_time:8.2f}ns")

    # Pyplot zone

    labels = ('Unknown', 'Logic', 'Non-RHPC Sync',
              'RHPC Sync')
    sizes = (metrics.average_limbo_time,
             metrics.average_logic_time,
             metrics.average_mpi_sync_time,
             metrics.average_rhpc_sync_time)

    fig, ax = plt.subplots()
    ax.pie(sizes, labels=labels, autopct='%1.1f%%')
    ax.axis('equal')

    if args.process is not None:
        ax.set_title(f"Process {args.process}")
    fig.suptitle('Distribution of time spent in the simulation')

    plt.show()
