#!/usr/bin/python3

#!/usr/bin/python3
"""
Run the simulation N times with fixed parameters in order to obtain a
reliable metric
"""
import postprocess as pp
import pandas as pd
from multiprocessing import Pool
import simulation as sim
import random
import argparse
import numpy as np

ps_layout = (1, 2)

parser = argparse.ArgumentParser()
parser.add_argument('--simultaneous', type=int, default=3)
parser.add_argument('output_file', help='Output CSV file')
parser.add_argument('--iterations', required=True, type=int,
                    help='Number of time each diff will be run')
parser.add_argument('--human-infection', type=float,
                    help='Human infection probability test')
parser.add_argument('--human-contamination', type=float,
                    help='Human -> object contamination probability')
parser.add_argument('--chair-infection', type=float,
                    help='Chair infection probability')
parser.add_argument('--bed-infection', type=float,
                    help='Bed infection probability')
parser.add_argument('--icu-chance', type=float,
                    help='ICU environment infection probability')
args = parser.parse_args()


def worker(human_infection, human_contamination, chair_infection, bed_infection, icu_chance):
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

    total = 65713
    year = pd.read_csv('admission_reference.csv')
    day = pd.read_csv('day_distribution_reference.csv').to_numpy()

    influx = pd.DataFrame()
    influx['day'] = year['admission_distribution'] * total
    influx['day'] = influx['day'].round()

    for interval in range(12):
        influx[f"{interval}"] = influx['day'] * day[interval]
    influx = influx.drop('day', axis='columns').round().astype('int64').to_numpy()
    infected_percentage = year['pneumonia_probability'].to_numpy()

    hospital.parameters = {
        'human': {
            'infect_distance': 2.0,
            'contamination_probability': human_contamination,
            'incubation_time': {
                'min': sim.TimePeriod(0, 14, 0, 0),
                'max': sim.TimePeriod(6, 0,  0, 0)
            },
            'infect_probability': human_infection
        },
        'objects': {
            'chair': {
                'infect_probability': chair_infection,
                'cleaning_interval': sim.TimePeriod(1, 0, 0, 0)
            },
            'bed': {
                'infect_probability': bed_infection,
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
            'walk_speed': 2.0,
            'infected_probability': infected_percentage,
            'influx': influx
        },
        'personnel': {
            'immunity': 0.81
        },
        'environments': {
            'icu': {
                'infection_probability': icu_chance
            }
        }
    }

    hospital.validate()

    props = sim.SimulationProperties(1, 2, seconds_per_tick=10,
                                     chair_manager_process=1,
                                     reception_manager_process=1,
                                     simulation_seed=random.randint(10000, 10000000))
    simulation = sim.Simulation(props, hospital)

    print(f"Starting {simulation.id}")

    simulation.run()

    # Extract the relevant data
    agents = pp.AgentsOutput(str(simulation.folder))
    df = pd.concat((agents.staff, agents.patients))

    total_patients = len(df[df['type'] == 'patient'])
    infected_patients = len(df[(df['type'] == 'patient') & (df['infected_by'] != '')])
    punctual_prevalence = (infected_patients / len(df[df['type'] == 'patient']))
    icu_total_patients = len(df[df['diagnosis_type'] == 'icu'])
    icu_infected_patients =  len(df[(df['diagnosis_type'] == 'icu') & (df['infected_by'] != '')])
    icu_punctual_prevalence = (icu_infected_patients / icu_total_patients)
    infected_patients_deaths_at_icu = len(df[(df['diagnosis_type'] == 'icu') & (df['infected_by'] != '') & (df['last_state'] == 'MORGUE')])
    icu_mortality_of_infected_patients = (infected_patients_deaths_at_icu / icu_infected_patients)
    total_infected_patients_by_personal = len(df[(df['type'] == 'patient') & (df['infected_by'].isin(df[df['type'] != 'patient']['infection_id']))])
    percentage_infected_patients_by_personal = (total_infected_patients_by_personal / infected_patients)
    total_infected_objects = agents.objects['infections'].apply(lambda o: len(o)).sum()
    total_infected_patients_by_objects = len(df[(df['type'] == 'patient') & (df['infected_by'].isin(agents.objects['infection_id']))])
    percentage_infected_patients_by_objects = (total_infected_patients_by_objects / infected_patients)
    total_infected_patients_by_patients = len(df[(df['type'] == 'patient') & (df['infected_by'].isin(df[df['type'] == 'patient']['infection_id']))])
    percentage_infected_patients_by_patients = (total_infected_patients_by_patients / infected_patients)
    icu_rejected_patients = len(df[(df['type'] == 'patient') & (df['last_state'] == 'WAIT_ICU')])
    percentage_of_rejections_at_icu = (icu_rejected_patients / icu_total_patients)
    waiting_room_rejected_patients = len(df[(df['type'] == 'patient') & (df['last_state'].str.startswith('WAIT_CHAIR'))])
    percentage_of_rejections_at_waiting_room = (waiting_room_rejected_patients / total_patients)
    out_of_time_patients = len(df[(df['type'] == 'patient') & (df['last_state'] == 'NO_ATTENTION')])
    percentage_of_out_of_time_patients = (out_of_time_patients / len(df[(df['type'] == 'patient') & (df['diagnosis_type'] == 'doctor')]))

    return {
        'run_id': simulation.id,
        'human_infection': human_infection,
        'human_contamination': human_contamination,
        'chair_infection': chair_infection,
        'bed_infection': bed_infection,
        'icu_chance': icu_chance,

        'total_patients': total_patients,
        'infected_patients': infected_patients,
        'punctual_prevalence': punctual_prevalence,
        'icu_total_patients': icu_total_patients,
        'icu_infected_patients': icu_infected_patients,
        'icu_punctual_prevalence': icu_punctual_prevalence,
        'infected_patients_deaths_at_icu': infected_patients_deaths_at_icu,
        'icu_mortality_of_infected_patients': icu_mortality_of_infected_patients,
        'total_infected_patients_by_personal': total_infected_patients_by_personal,
        'percentage_infected_patients_by_personal': percentage_infected_patients_by_personal,
        'total_infected_objects': total_infected_objects,
        'total_infected_patients_by_objects': total_infected_patients_by_objects,
        'percentage_infected_patients_by_objects': percentage_infected_patients_by_objects,
        'total_infected_patients_by_patients': total_infected_patients_by_patients,
        'percentage_infected_patients_by_patients': percentage_infected_patients_by_patients,
        'icu_rejected_patients': icu_rejected_patients,
        'percentage_of_rejections_at_icu': percentage_of_rejections_at_icu,
        'waiting_room_rejected_patients': waiting_room_rejected_patients,
        'percentage_of_rejections_at_waiting_room': percentage_of_rejections_at_waiting_room,
        'out_of_time_patients': out_of_time_patients,
        'percentage_of_out_of_time_patients': percentage_of_out_of_time_patients
    }


try:
    df = pd.read_csv(args.output_file)
except:
    df = pd.DataFrame()

try:
    label = df['label'].max() + 1
except:
    label = 0
with Pool(args.simultaneous) as pool:
    res = pool.starmap(worker, args.iterations * [(args.human_infection,
                                                   args.human_contamination,
                                                   args.chair_infection,
                                                   args.bed_infection,
                                                   args.icu_chance)],
                                                   chunksize=1)

for result in res:
    result['label'] = label
df = pd.concat([df, pd.DataFrame(data=res)])
df.to_csv(str(args.output_file), index=False)
