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
import traceback
import re

ps_layout = [(1, 1), (1, 2), (2, 1), (2, 2)]

parser = argparse.ArgumentParser(
    description=('Run the simulation several times, '
                 'storing the times in a CSV file. '
                 'The benchmark goes through the predefined process layout '
                 f"{ps_layout} I times."
                 "Or the specified layout if the --layout flag is used"))
parser.add_argument('file', help='Output CSV file')
parser.add_argument('label', help='Label/tag for the benchmark')
parser.add_argument('-i', '--iterations', type=int, default=16,
                    help='Number of iterations in each layout')
parser.add_argument('--max-cores', type=int, default=os.cpu_count(),
                    help='Maximum number of CPU cores used for the benchmark')
parser.add_argument('--log', type=str,
                    default=Path(__file__).parent/'benchmark.log',
                    help='Log file')
parser.add_argument('-k', '--keep-runs', action='store_true',
                    help='Don not remove run folders after upon finish')
parser.add_argument('-c', '--continue', action='store_true',
                    dest='continue_',
                    help=('Continue an existing benchmark.'
                          'Note: the number of iterations will be the one '
                          'indicated in this invocation'))
parser.add_argument('-l', '--layout', type=str, action='append',
                    help='An NxM layout to run instead of the predefined layouts')
parser.add_argument('-s', '--skip', type=int, default=0,
                    help=('Skip the first N process layouts. Ignored if '
                          '-c/--continue is specified'))
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
hospital.add_element(sim.ICU((20, 5)))

for y in (25, 31):
    hospital.add_element(sim.DoctorOffice(
        'general_practitioner', (3, y), (4, y)))
hospital.add_element(sim.DoctorOffice('psychiatrist', (3, 5), (4, 5)))
hospital.add_element(sim.DoctorOffice('surgeon', (18, 13), (18, 14)))
hospital.add_element(sim.DoctorOffice('pediatry', (27, 13), (27, 14)))
hospital.add_element(sim.DoctorOffice('gynecologist', (36, 13), (36, 14)))
hospital.add_element(sim.DoctorOffice('geriatrics', (45, 13), (45, 14)))

hospital.add_element(sim.Receptionist((45, 25), (45, 27)))


hospital.add_element(sim.Triage((32, 25)))
hospital.add_element(sim.Triage((34, 25)))
hospital.add_element(sim.Triage((36, 25)))

for x in range(14, 28, 2):
    for y in range(22, 31, 2):
        hospital.add_element(sim.Chair((x, y)))

original_probability = 0.00099225
objects_probability = original_probability / 10
patient_sum = 67000

# Load the reference admission distribution
adm_ref = pd.read_csv('admission_reference.csv')
day_distribution_reference = pd.read_csv('day_distribution_reference.csv')
day_influx = (adm_ref['general_admissions'] * (patient_sum) /
              adm_ref['general_admissions'].sum()).round().to_numpy()
influx = np.array([[0 for i in range(12)] for d in range(365)])
for d in range(365):
    influx[d] = day_distribution_reference['percentage'].to_numpy() * \
        day_influx[i]
infected_percentage = adm_ref['percentage'].to_numpy()


hospital.parameters = {
    'objects': {
        'chair': {
            'infect_probability': objects_probability,
            'cleaning_interval': sim.TimePeriod(1, 0, 0, 0)
        },
        'bed': {
            'infect_probability': original_probability,
            'cleaning_interval': sim.TimePeriod(1, 0, 0, 0)
        }
    },
    'icu': {
        'beds': 90,
        'sleep_times': [
            {
                'time': sim.TimePeriod(2, 14, 24, 0),
                'probability': 0.004748328
            },
            {
                'time': sim.TimePeriod(3, 0, 0, 0),
                'probability': 0.088623115
            },
            {
                'time': sim.TimePeriod(3, 7, 12, 0),
                'probability': 0.017333166
            },
            {
                'time': sim.TimePeriod(3, 16, 48, 0),
                'probability': 0.032968386
            },
            {
                'time': sim.TimePeriod(4, 4, 48, 0),
                'probability': 0.013086353
            },
            {
                'time': sim.TimePeriod(4, 9, 36, 0),
                'probability': 0.100335789
            },
            {
                'time': sim.TimePeriod(4, 16, 48, 0),
                'probability': 0.066380434
            },
            {
                'time': sim.TimePeriod(4, 21, 36, 0),
                'probability': 0.000017555
            },
            {
                'time': sim.TimePeriod(6, 7, 12, 0),
                'probability': 0.007899849
            },
            {
                'time': sim.TimePeriod(6, 9, 36, 0),
                'probability': 0.100224175
            },
            {
                'time': sim.TimePeriod(6, 12, 0, 0),
                'probability': 0.084432757
            },
            {
                'time': sim.TimePeriod(6, 16, 48, 0),
                'probability': 0.117953925
            },
            {
                'time': sim.TimePeriod(7, 9, 36, 0),
                'probability': 0.053206605
            },
            {
                'time': sim.TimePeriod(8, 0, 0, 0),
                'probability': 0.026187069
            },
            {
                'time': sim.TimePeriod(9, 4, 48, 0),
                'probability': 0.122177398
            },
            {
                'time': sim.TimePeriod(9, 7, 12, 0),
                'probability': 0.033379033
            },
            {
                'time': sim.TimePeriod(10, 4, 48, 0),
                'probability': 0.037753818
            },
            {
                'time': sim.TimePeriod(29, 16, 48, 0),
                'probability': 0.094000000
            }
        ]
    },
    'reception': {
        'attention_time': sim.TimePeriod(0, 0, 1, 0)
    },
    'triage': {
        'icu': {
            'death_probability': 0.255,
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
                'probability': 0.0418719,
                'wait_time': sim.TimePeriod(0, 0, 0, 0)
            },
            {
                'level': 2,
                'probability': 0.0862069,
                'wait_time': sim.TimePeriod(0, 0, 15, 0)
            },
            {
                'level': 3,
                'probability': 0.6305419,
                'wait_time': sim.TimePeriod(0, 1, 0, 0)
            },
            {
                'level': 4,
                'probability': 0.2266010,
                'wait_time': sim.TimePeriod(0, 2, 0, 0)
            },
            {
                'level': 5,
                'probability': 0.0147783,
                'wait_time': sim.TimePeriod(0, 4, 0, 0)
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
        'walk_speed': 0.5,
        'infected_probability': infected_percentage,
        'influx': influx
    },
    'human': {
        'infect_distance': 2.0,
        'contamination_probability': original_probability,
        'incubation_time': {
            'min': sim.TimePeriod(0, 14, 0, 0),
            'max': sim.TimePeriod(6, 0,  0, 0)
        },
        'infect_probability': original_probability
    },
    'personnel': {
        'immunity': 0.81
    },
    'environments': {
        'icu': {
            'infection_probability': 0.00135
        }
    }
}

hospital.validate()


def worker(simulation):
    print(f"Starting {simulation.id}")
    return simulation.run()


git_clean = subprocess.run(['git', 'status', '--porcelain'],
                           capture_output=True, text=True).stdout

logging.info('\n---\n')
if git_clean == '':
    commit_id = subprocess.run(['git', 'rev-parse', 'HEAD'],
                               capture_output=True, text=True).stdout.strip()
    logging.info(f"Commit: {commit_id}")
else:
    commit_id = None
    logging.info(f"Dirty tree, cannot determine commit")

if args.layout is not None:
    ps_layout = []
    for l in args.layout:
        print(args.layout)
        r = re.match(r'(?P<x>\d+)x(?P<y>\d+)', l)
        if r is None:
            raise Exception('--layout must be NxM')
        if int(r['x']) < 1 or int(r['y']) < 1:
            raise Exception('--layout N and M must be >=1')
        ps_layout.append((int(r['x']), int(r['y'])))

for f in (print, logging.info):
    f(f"Iterations: {args.iterations}")
    f(f"Max cores: {args.max_cores}")
    f(f"Layouts: {ps_layout}")

if args.continue_:
    df = pd.read_csv(args.file)
    t = (df.iloc[-1].x_processes, df.iloc[-1].y_processes)
    ps_layout = ps_layout[ps_layout.index(t)+1:]
else:
    try:
        df = pd.read_csv(args.file)
    except:
        df = pd.DataFrame(columns=['run_id', 'label', 'commit',
                                   'x_processes', 'y_processes', 'time'])
    if args.layout is not None:
        ps_layout = ps_layout[args.skip:]

try:
    i = 1

    for layout in ps_layout:
        print(f"Layout {i}/{len(ps_layout)}")

        props = sim.SimulationProperties(*layout, seconds_per_tick=10,
                                         chair_manager_process=1,
                                         reception_manager_process=1,
                                        #  triage_manager_process=1,
                                        #  doctors_manager_process=1
                                         )
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
            res = pool.map(worker, simulations)

        for ret, cmd in res:
            if ret.returncode != 0:
                raise Exception(f"{cmd} finished with exit code {ret.returncode}")

        logging.info(f"Runs IDs: {' '.join([s.id for s in simulations])}")

        new_data = pd.DataFrame([{
            'run_id': run.id,
            'label': args.label,
            'commit': commit_id,
            'x_processes': layout[0],
            'y_processes': layout[1],
            'time': perf.Metrics(run.folder).total_time
        }
            for run in simulations])
        df = pd.concat([df, new_data])
        df.to_csv(str(args.file), index=False)

        if not args.keep_runs:
            for run in simulations:
                subprocess.run(['rm', '-r', str(run.folder)],
                               capture_output=True)
except Exception as e:
    logging.info(traceback.format_exc())
    print(traceback.format_exc())
