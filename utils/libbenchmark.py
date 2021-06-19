#!/usr/bin/python3

import argparse
import pandas as pd
import random
import datetime
import tabulate
import simulation as sim
import performance as perf

parser = argparse.ArgumentParser(
    description='Run simulation several times, store results in file')
parser.add_argument('--file', help='Output CSV file')
parser.add_argument('-i',  '--iterations', help='Number of runs')
parser.add_argument('-l', '--layout', action='append',
                    help='layout[s] to execute')
parser.add_argument('-p', '--patients', type=int, action='append',
                    help='Number of patients entering the hospital')


def make_simulation(layout: "tuple[int]",
                    patients: int,
                    seconds_per_tick: int,
                    chair_process: int,
                    reception_process: int,
                    triage_process: int,
                    doctor_process: int) -> sim.Simulation:
    """Create a simulation with the correct configuration"""

    human_infection = 1.500000e-01
    human_contamination = 3.000000e-05
    chair_infection = 8.000000e-06
    bed_infection = 7.000000e-08
    icu_chance = 4.800000e-09

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

    year = pd.read_csv('admission_reference.csv')
    day = pd.read_csv('day_distribution_reference.csv').to_numpy()

    influx = pd.DataFrame()
    influx['day'] = year['admission_distribution'] * patients
    influx['day'] = influx['day'].round()

    for interval in range(12):
        influx[f"{interval}"] = influx['day'] * day[interval]
    influx = influx.drop('day', axis='columns').round().astype(
        'int64').to_numpy()
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

    props = sim.SimulationProperties(layout[0], layout[1],
                                     seconds_per_tick=seconds_per_tick,
                                     chair_manager_process=chair_process,
                                     reception_manager_process=reception_process,
                                     triage_manager_process=triage_process,
                                     doctors_manager_process=doctor_process,
                                     simulation_seed=random.randint(10000, 10000000))

    return sim.Simulation(props, hospital)


class LocalConfiguration(object):
    """Local benchmark configuration"""

    def __init__(self,
                 tag: str,
                 layout: "tuple[int, int]",
                 patients: int,
                 seconds_per_tick: int,
                 chair_process: int,
                 reception_process: int,
                 triage_process: int,
                 doctor_process: int):

        self.tag = tag
        self.layout = layout
        self.patients = patients
        self.seconds_per_tick = seconds_per_tick
        self.chair_process = chair_process
        self.reception_process = reception_process
        self.triage_process = triage_process
        self.doctor_process = doctor_process

    def run(self) -> dict:
        """Run the simulation, return the metrics"""

        simulation = make_simulation(self.layout, self.patients,
                                     self.seconds_per_tick, self.chair_process,
                                     self.reception_process,
                                     self.triage_process, self.doctor_process)
        result, command = simulation.run()
        metrics = perf.Metrics(simulation.folder)

        data= {
            'run_id': simulation.id,
            'tag': self.tag,
            'configuration_type': 'local',
            'x_processes': self.layout[0],
            'y_processes': self.layout[1],
            'patients': self.patients,
            'seconds_per_tick': self.seconds_per_tick,
            'chair_process': self.chair_process,
            'reception_process': self.reception_process,
            'triage_process': self.triage_process,
            'doctor_process': self.doctor_process,
            'total_time': metrics.total_time
        }

        try:
            data = {
                **data,
                **metrics.section_time()
            }
        except:
            pass
        return data


class Batch(object):
    """A collection of configurations, waiting to be executed

    Keyword Arguments:
    - file -- Output CSV file
    """

    def __init__(self, file):
        self.time = datetime.datetime.now().isoformat()
        self.file = file
        self.configurations = []

    def add_configurations(self, count: int, configuration):
        """Run count times the give configuration"""
        self.configurations += [configuration for _ in range(count)]

    def report(self) -> str:
        """Return a string with a print-friendly report about the batch"""

        s = f"Batch {self.time} \n"

        header = ['tag',
                  'layout',
                  'patients',
                  'seconds_per_tick',
                  'chair_process',
                  'reception_process',
                  'triage_process',
                  'doctor_process']
        table = []
        for run in self.configurations:
            table.append([f"{run.tag}",
                          f"{run.layout}",
                          f"{run.patients}",
                          f"{run.seconds_per_tick}",
                          f"{run.chair_process}",
                          f"{run.reception_process}",
                          f"{run.triage_process}",
                          f"{run.doctor_process}"])

        s += tabulate.tabulate(table, headers=header, tablefmt='github')
        return s

    def run(self, print_to_console=True):
        """Run the batch, save the results"""

        results = []
        for configuration, i in zip(self.configurations, range(1, len(self.configurations) + 1)):
            if print_to_console:
                print(f"Running {i}/{len(self.configurations)}")
            results.append(configuration.run())

        try:
            df = pd.read_csv(self.file)
        except:
            df = pd.DataFrame()

        df = df.append(results)
        df.to_csv(self.file, index=False)


if __name__ == '__main__':
    batch = Batch('test.csv')
    batch.add_configurations(5, LocalConfiguration(tag='test-batch', layout=(1, 1), patients=0,
                                                   seconds_per_tick=10, chair_process=0, reception_process=0, triage_process=0, doctor_process=0))
    print(batch.report())

    batch.run()

    # args = parser.parse_args()
    # raise Exception('Unimplemented')
