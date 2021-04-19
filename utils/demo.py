#!/usr/bin/python3

import simulation as s
import performance
import numpy as np
import random

# Create a new simple hospital

hospital = s.Hospital(10, 10)

for i in range(10):
    hospital.add_element(s.Wall((0, i)))
    hospital.add_element(s.Wall((9, i)))
    hospital.add_element(s.Wall((i, 0)))
    hospital.add_element(s.Wall((i, 9)))

hospital.add_element(s.DoctorOffice('general_practitioner', (3, 8), (3, 7)))
hospital.add_element(s.DoctorOffice('radiologist', (5, 8), (5, 7)))

hospital.add_element(s.Receptionist((8, 2), (8, 3)))

hospital.add_element(s.Triage((5, 1)))
hospital.add_element(s.ICU((8, 8)))

for i in range(1, 4, 1):
    hospital.add_element(s.Chair((i, 2)))

hospital.add_element(s.Entry((0, 5)))
hospital.add_element(s.Exit((9, 5)))

hospital.parameters = {
    'objects': {
        'chair': {
            'infect_probability': 0.1,
            'cleaning_interval': s.TimePeriod(0, 2, 0, 0),
            'radius': 0.1
        },
        'bed': {
            'infect_probability': 0.0,
            'radius': 0.0,
            'cleaning_interval': s.TimePeriod(0, 2, 0, 0)
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
                'time': s.TimePeriod(1, 0, 0, 0),
                'probability': 0.2
            },
            {
                'time': s.TimePeriod(2, 0, 0, 0),
                'probability': 0.2
            },
            {
                'time': s.TimePeriod(4, 0, 0, 0),
                'probability': 0.2
            },
            {
                'time': s.TimePeriod(8, 0, 0, 0),
                'probability': 0.2
            },
            {
                'time': s.TimePeriod(16, 0, 0, 0),
                'probability': 0.2
            }
        ]
    },
    'reception': {
        'attention_time': s.TimePeriod(0, 0, 15, 0)
    },
    'triage': {
        'icu': {
            'death_probability': 0.2,
            'probability': 0.2
        },
        'doctors_probabilities': [
            {
                'specialty': 'general_practitioner',
                'probability': 0.6
            },
            {
                'specialty': 'radiologist',
                'probability': 0.2
            }
        ],
        'levels': [
            {
                'level': 1,
                'probability': 0.2,
                'wait_time': s.TimePeriod(0, 0, 0, 0)
            },
            {
                'level': 2,
                'probability': 0.2,
                'wait_time': s.TimePeriod(0, 0, 0, 15)
            },
            {
                'level': 3,
                'probability': 0.2,
                'wait_time': s.TimePeriod(0, 0, 0, 30)
            },
            {
                'level': 4,
                'probability': 0.2,
                'wait_time': s.TimePeriod(0, 0, 1, 0)
            },
            {
                'level': 5,
                'probability': 0.2,
                'wait_time': s.TimePeriod(0, 0, 2, 0)
            }
        ],
        'attention_time': s.TimePeriod(0, 0, 15, 0)
    },
    'doctors': [
        {
            'attention_duration': s.TimePeriod(0, 0, 30, 0),
            'specialty': 'general_practitioner'
        },
        {
            'attention_duration': s.TimePeriod(0, 1, 0, 0),
            'specialty': 'radiologist'
        }
    ],
    'patient': {
        'walk_speed': 0.2,
        'influx': np.array([[random.randrange(2, 10) for i in range(12)] for j in range(365)]),
        'infected_probability': 0.1
    },
    'human': {
        'infect_distance': 2.0,
        'contamination_probability': 0.1,
        'incubation_time': s.TimePeriod(0, 2, 0, 0),
        'infect_probability': 0.2
    }
}

# Generate a new run
props = s.SimulationProperties(1, 1)

run = s.Simulation(props, hospital)

run.run()

# Plot the performance 
metrics = performance.Metrics(run.folder)
print(f"Simulation {run.id}")
print(f"\t total_time = {metrics.total_time}")
print(f"\t save_time = {metrics.save_time}")
metrics.plot().pie()

metrics.plot().plot(0, ('rhpc', 'mpi'))
