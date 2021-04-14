#!/usr/bin/python3.8

import pandas as pd
import glob


class AgentData(object):

    def init(self, folderpath):
        exit_path = glob.glob(f"{folderpath}/exit_in_process_*.json")
