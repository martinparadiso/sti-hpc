#!/usr/bin/python3
"""
Run the simulation N times with fixed parameters in order to obtain a
reliable metric
"""
import performance as perf
import pandas as pd
from multiprocessing import Pool
import os
import simulation as sim
import random
import argparse
import numpy as np
from pathlib import Path
import subprocess
import logging

parser = argparse.ArgumentParser(
    description='Run the simulation N times, print the execution time')
parser.add_argument('-i', '--iterations', type=int, default=16,
                    help='Number of times the simulation will run')
parser.add_argument('-c', '--max-cores', type=int, default=os.cpu_count(),
                    help='Maximum number of CPU cores used for the benchmark')
parser.add_argument('-l', '--log', type=str, default=Path(__file__).parent/'benchmark.log',
                    help='Log file')
parser.add_argument('-r', '--remove-runs', action='store_true',
                    help='Cleanup run folder')
args = parser.parse_args()

logging.basicConfig(filename=args.log,
                    level=logging.DEBUG,
                    format='%(message)s')

width = 53
height = 36
hospital = sim.Hospital(width, height)

for i in range(width):
    hospital.add_element(sim.Wall((i, 0)))
    hospital.add_element(sim.Wall((i, height - 1)))
for i in range(height):
    hospital.add_element(sim.Wall((0, i)))
    hospital.add_element(sim.Wall((width-1, i)))

for y in range(36):
    l = (4, 5, 13, 19, 25, 31)
    if y not in l:
        hospital.add_element(sim.Wall((9, y)))

for x in range(9):
    for y in (10, 16, 22, 28):
        hospital.add_element(sim.Wall((x, y)))

for y in range(9):
    if y not in (5, 6):
        hospital.add_element(sim.Wall((14, y)))

for y in range(9, 19):
    for x in (14, 23, 32, 41):
        hospital.add_element(sim.Wall((x, y)))

for x in range(14, 52):
    hospital.add_element(sim.Wall((x, 9)))

    if x not in (19, 28, 38, 46, 18, 27, 36, 45):
        hospital.add_element(sim.Wall((x, 18)))

for x in range(29, 52):
    hospital.add_element(sim.Wall((x, 23)))

for x in range(29, 40):
    if x not in (34, 35):
        hospital.add_element(sim.Wall((x, 28)))

for y in range(23, 28):
    hospital.add_element(sim.Wall((29, y)))
    hospital.add_element(sim.Wall((39, y)))

hospital.add_element(sim.Entry((width-9, height-1)))
hospital.add_element(sim.Exit((width-8, height-1)))
hospital.add_element(sim.ICU((14, 5)))

for y in (13, 19, 25, 31):
    hospital.add_element(sim.DoctorOffice(
        'general_practitioner', (3, y), (5, y)))
hospital.add_element(sim.DoctorOffice('psychiatrist', (3, 5), (5, 5)))
hospital.add_element(sim.DoctorOffice('surgeon', (18, 13), (18, 15)))
hospital.add_element(sim.DoctorOffice('pediatry', (27, 13), (27, 15)))
hospital.add_element(sim.DoctorOffice('gynecologist', (36, 13), (36, 15)))
hospital.add_element(sim.DoctorOffice('geriatrics', (45, 13), (45, 15)))

hospital.add_element(sim.Receptionist((45, 25), (45, 27)))
hospital.add_element(sim.Receptionist((48, 25), (48, 27)))

hospital.add_element(sim.Triage((35, 26)))

for x in range(14, 28, 2):
    for y in range(22, 31, 2):
        hospital.add_element(sim.Chair((x, y)))

hospital.parameters = {
    'objects': {
        'chair': {
            'infect_probability': 0.1,
            'cleaning_interval': sim.TimePeriod(0, 2, 0, 0),
            'radius': 0.1
        },
        'bed': {
            'infect_probability': 0.0,
            'radius': 0.0,
            'cleaning_interval': sim.TimePeriod(0, 2, 0, 0)
        }
    },
    'icu': {
        'environment': {
            'infection_probability': 0.1
        },
        'beds': 5,
        'death_probability': 0.05,
        'sleep_times': [
            {
                'time': sim.TimePeriod(1, 0, 0, 0),
                'probability': 0.2
            },
            {
                'time': sim.TimePeriod(2, 0, 0, 0),
                'probability': 0.2
            },
            {
                'time': sim.TimePeriod(4, 0, 0, 0),
                'probability': 0.2
            },
            {
                'time': sim.TimePeriod(8, 0, 0, 0),
                'probability': 0.2
            },
            {
                'time': sim.TimePeriod(16, 0, 0, 0),
                'probability': 0.2
            }
        ]
    },
    'reception': {
        'attention_time': sim.TimePeriod(0, 0, 15, 0)
    },
    'triage': {
        'icu': {
            'death_probability': 0.2,
            'probability': 0.070724557
        },
        'doctors_probabilities': [
            {
                'specialty': 'general_practitioner',
                'probability': 0.444567551
            },
            {
                'specialty': 'psychiatrist',
                'probability': 0.045610876
            },
            {
                'specialty': 'surgeon',
                'probability': 0.292939085
            },
            {
                'specialty': 'pediatry',
                'probability': 0.051335318
            },
            {
                'specialty': 'gynecologist',
                'probability': 0.075895021
            },
            {
                'specialty': 'geriatrics',
                'probability': 0.018927592
            },
        ],
        'levels': [
            {
                'level': 1,
                'probability': 0.2,
                'wait_time': sim.TimePeriod(0, 0, 0, 0)
            },
            {
                'level': 2,
                'probability': 0.2,
                'wait_time': sim.TimePeriod(0, 0, 0, 15)
            },
            {
                'level': 3,
                'probability': 0.2,
                'wait_time': sim.TimePeriod(0, 0, 0, 30)
            },
            {
                'level': 4,
                'probability': 0.2,
                'wait_time': sim.TimePeriod(0, 0, 1, 0)
            },
            {
                'level': 5,
                'probability': 0.2,
                'wait_time': sim.TimePeriod(0, 0, 2, 0)
            }
        ],
        'attention_time': sim.TimePeriod(0, 0, 15, 0)
    },
    'doctors': [
        {
            'attention_duration': sim.TimePeriod(0, 0, 15, 0),
            'specialty': 'general_practitioner'
        },
        {
            'attention_duration': sim.TimePeriod(0, 0, 15, 0),
            'specialty': 'psychiatrist'
        },
        {
            'attention_duration': sim.TimePeriod(0, 0, 15, 0),
            'specialty': 'surgeon'
        },
        {
            'attention_duration': sim.TimePeriod(0, 0, 15, 0),
            'specialty': 'gynecologist'
        },
        {
            'attention_duration': sim.TimePeriod(0, 0, 15, 0),
            'specialty': 'geriatrics'
        },
        {
            'attention_duration': sim.TimePeriod(0, 0, 15, 0),
            'specialty': 'pediatry'
        },
    ],
    'patient': {
        'walk_speed': 0.2,
        'influx': np.array([[random.randrange(1, 15) for i in range(12)] for j in range(365)]),
        'infected_probability': 0.3
    },
    'human': {
        'infect_distance': 2.0,
        'contamination_probability': 0.1,
        'incubation_time': sim.TimePeriod(0, 2, 0, 0),
        'infect_probability': 0.5
    },
    'personnel': {
        'immunity': 0.8
    },
    'environments': {
        'icu': {
            'infection_probability': 0.8
        }
    }
}

hospital.validate()


def worker(simulation):
    simulation.run()


ps_layout = ((1, 1),
             (1, 2),
             (2, 1),
             (2, 2))

git_clean = subprocess.run(['git', 'status', '--porcelain'],
                           capture_output=True, text=True).stdout

logging.info('\n---\n')
if git_clean == '':
    commit_id = subprocess.run(['git', 'rev-parse', 'HEAD'],
                               capture_output=True, text=True).stdout.strip()
    logging.info(f"Commit: {commit_id}")
else:
    logging.info(f"Dirty tree, cannot determine commit")

logging.info(f"Iterations: {args.iterations}")
logging.info(f"Max cores: {args.max_cores}")
logging.info(f"Layouts: {ps_layout}")

for layout in ps_layout:

    props = sim.SimulationProperties(*layout)
    simulations = [sim.Simulation(props, hospital)
                   for i in range(args.iterations)]

    cores_per_run = props.number_of_processes
    simultaneous = min(args.iterations,
                       int(args.max_cores/props.number_of_processes))
    cores_used = cores_per_run * simultaneous

    logging.info((f"Benchmarking {layout} process layout, running "
                  f"{simultaneous} instances at a time "
                  f"across {cores_used} cores"))

    with Pool(simultaneous) as pool:
        pool.map(worker, simulations)

    logging.info(f"Runs IDs: {' '.join([s.id for s in simulations])}")

    times = pd.Series(
        [perf.Metrics(run.folder).total_time for run in simulations])
    logging.info(times.describe())
    logging.info('')

    if args.remove_runs:
        for run in simulations:
            subprocess.run(['rm', '-r', str(run.folder)], capture_output=True)
