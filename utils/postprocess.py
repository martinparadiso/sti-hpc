#!/usr/bin/python3.8

from simulation import Wall
import glob
import pandas as pd
import re
import json
from pathlib import Path
import sys
sys.path.append(Path(__file__).parent)


class AgentLocations(object):
    """Load the agents locations from output files"""

    files_glob = 'agents_locations.p*.csv'

    def __init__(self, folderpath):

        files = glob.glob(f"{folderpath}/{self.files_glob}")

        dfs = []
        for path in files:
            df = pd.read_csv(path)
            df['process'] = re.match(
                r'.+agents_locations\.p(\d+)\.csv', path)[1]
            dfs.append(df)

        self.dataframe = pd.concat(dfs)


def rename_columns(df: pd.DataFrame):

    prefixes = ('infection.', 'diagnosis.')
    special_renames = {
        'diagnosis.type': 'diagnosis_type',
        'diagnosis.specialty': 'doctor_specialty',
        'diagnosis.attention_datetime_limit.time': 'attention_datetime_limit',
        'diagnosis.sleep_time.time': 'sleep_time'
    }

    df = df.rename(columns=special_renames)
    for col in df.columns:
        for prefix in prefixes:
            if col.startswith(prefix):
                df = df.rename(columns={col: col[len(prefix):]})
    return df


class AgentsOutput(object):
    """Load the agents output from output files"""

    files_globs = ('agents.p*.json',
                   'exit.p*.json',
                   'icu_beds.p*.json',
                   'chairs.p*.json',
                   'staff.p*.json',
                   'morgue.p*.json')

    human_cols = ['repast_id', 'type', 'entry_time', 'exit_time', 'last_state',
                  'process',
                  'infection_id', 'infection_model', 'infection_mode',
                  'infection_stage', 'infection_time', 'infected_by',
                  'infect_location.x', 'infect_location.y', 'incubation_end']

    patient_cols = [*human_cols,
                    'diagnosis_type', 'doctor_specialty', 'triage_level',
                    'attention_datetime_limit', 'sleep_time', 'survives']

    object_cols = ['process',
                   'infection_id', 'infection_stage', 'infection_model',
                   'infections']

    time_cols = ['infection_time', 'entry_time', 'exit_time',
                 'attention_datetime_limit', 'sleep_time', 'incubation_end']

    def __init__(self, folderpath):

        paths = [path for f in self.files_globs
                 for path in glob.glob(f"{folderpath}/{f}")]

        # Load all the files found
        dfs = []
        for path in paths:
            with open(path) as f:
                data = json.load(f)
            df = pd.json_normalize(data)
            df = rename_columns(df)
            df['process'] = int(re.match(r'.+\.p(\d+)\.json', path)[1])
            dfs.append(df)

        df = pd.concat(dfs)

        for col in self.time_cols:
            df[col] = pd.to_timedelta(df[col], unit='seconds')

        # Add the dataframes to the object
        # Split the objects in humans and objects
        self.dataframe = df
        self.staff = df[(df['infection_model'] == 'human') &
                        (df['type'] != 'patient')][self.human_cols]
        self.objects = df[df['infection_model'] == 'object'][self.object_cols]
        self.patients = df[df['type'] == 'patient'][self.patient_cols]


class AgentsLocations(object):
    """Load the agents locations generated by a simulation"""

    positions_glob = 'agents_locations.p*.csv'

    def __init__(self, folderpath):
        paths = glob.glob(f"{folderpath}/{self.positions_glob}")
        dfs = []
        for path in paths:
            df = pd.read_csv(path)
            df['process'] = int(re.match(r'.+\.p(\d+)\.csv', path)[1])
            dfs.append(df)

        self.df = pd.concat(dfs)

    def plot(self, agent_id):
        """Plot the agent <agent_id> path"""
        from matplotlib import pyplot as plt

        the_agent = self.df[self.df['repast_id'] == agent_id]
        x = the_agent['x'].to_numpy()
        y = the_agent['y'].to_numpy()

        fig, ax = plt.subplots()
        ax.set_ylim(0, 36)
        ax.set_xlim(0, 53)

        return ax.plot(x, y, 'go')

    def plot_animation(self, agent_id, hospital):
        """Plot the path traveled by an agent, animated"""
        import numpy as np
        import matplotlib.pyplot as plt
        import matplotlib.animation as animation

        fig, ax = plt.subplots()
        plt.xlim(0, hospital.dimensions[0])
        plt.ylim(0, hospital.dimensions[1])

        building = dict()
        for elem in hospital.elements:
            color = elem.plot_color
            if color is not None:
                if elem.plot_color not in building:
                    building[color] = np.full((2, 0), 0)
                new_element = np.array([[elem.location.x + 0.5],
                                        [elem.location.y + 0.5]])
                building[color] = np.append(
                    building[color], new_element, axis=1)

        for color in building:
            ax.plot(building[color][0], building[color][1], 's',
                    markersize='4', color=color)
        l, = ax.plot([], [], 'bo')
        plt.title(f"Agent {agent_id} movement")

        df = self.df[self.df['repast_id'] == agent_id]
        oc = df[['datetime', 'x', 'y']]
        data = oc[((oc.shift() != oc)['x'] | (oc.shift() != oc)['y'])]
        data = data[['x', 'y']].T.to_numpy()
        timestamp = pd.DataFrame(pd.to_timedelta(oc['datetime'], unit='s'))

        def feeder(frame, data, points):
            points.set_data(data[..., frame-1:frame])
            points.set_label(str(timestamp.iloc[frame]))
            return points,

        ani = animation.FuncAnimation(fig, feeder,
                                      frames=data.shape[1],
                                      fargs=(data, l),
                                      interval=200,
                                      blit=True)

        ani.save('/home/martin/path.mp4')
