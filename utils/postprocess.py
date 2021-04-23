#!/usr/bin/python3.8

import glob
import json
import pandas as pd
import re


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


class AgentsOutput(object):
    """Load the agents output from output files"""

    files_globs = ('agents.p*.json',
                   'exit.p*.json',
                   'icu_beds.p*.json',
                   'chairs.p*.json')

    human_cols = ['repast_id', 'type', 'entry_time', 'exit_time', 'last_state',
                  'process',
                  'infection_id', 'infection_model',  'infection_stage',
                  'infection_time', 'infected_by']
    object_cols = ['process',
                   'infection_id', 'infection_stage', 'infection_model',
                   'infections']

    def __init__(self, folderpath):

        paths = [path for f in self.files_globs
                 for path in glob.glob(f"{folderpath}/{f}")]

        # Load all the files found
        dfs = []
        for path in paths:
            with open(path) as f:
                data = json.load(f)
            df = pd.json_normalize(data)

            # Remove the 'infection' prefix
            for c in df.columns:
                if c.startswith('infection.'):
                    df = df.rename(columns={c: c[10:]})

            df['process'] = int(re.match(r'.+\.p(\d+).json', path)[1])
            dfs.append(df)

        df = pd.concat(dfs)

        # Fix the dtypes
        for col in ('infection_time', 'entry_time', 'exit_time'):
            df[col] = pd.to_timedelta(df[col], unit='seconds')

        # Add the dataframes to the object
        # Split the objects in humans and objects
        self.dataframe = df
        self.humans = df[df['infection_model'] == 'human'][self.human_cols]
        self.objects = df[df['infection_model'] == 'object'][self.object_cols]
