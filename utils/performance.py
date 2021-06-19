#!/usr/bin/python3

from collections import namedtuple
import argparse
import glob
import matplotlib.pyplot as plt
import pandas as pd
import re


class Metrics(object):
    """
    Collect metrics from a execution

    Keyword arguments:

    - folderpath -- The folder of the simulation run, containing the metric data
    """

    mpi_stages = ('chairs_sync',
                  'reception_sync',
                  'triage_sync',
                  'doctors_sync',
                  'icu_sync')

    def __init__(self, folderpath):

        tick_files = glob.glob(f"{folderpath}/tick_metrics.p*.csv")

        # Per tick metrics can be disable, so the the file may no exist
        if tick_files:
            tick_dfs = []
            for tick_path in tick_files:
                tick_df = pd.read_csv(tick_path)
                process = int(
                    re.match(r'.+tick_metrics\.p(\d+)\.csv', tick_path)[1])
                tick_df['process'] = process
                tick_dfs.append(tick_df)
            tmp_df = pd.concat(tick_dfs)

            # Convert to a more usable format
            self.ticks = pd.DataFrame()
            self.ticks['tick'] = tmp_df['tick']
            self.ticks['process'] = tmp_df['process']
            self.ticks['agents'] = tmp_df['agents']
            self.ticks['duration'] = tmp_df['end_time'] - tmp_df['start_time']
            self.ticks[self.mpi_stages[0]
                    ] = tmp_df[self.mpi_stages[0]] - tmp_df['start_time']
            for prev, current in zip(self.mpi_stages[0:], self.mpi_stages[1:]):
                self.ticks[current] = tmp_df[current] - tmp_df[prev]
            self.ticks['rhpc_sync'] = tmp_df['rhpc_sync'] - \
                tmp_df[self.mpi_stages[-1]]
            self.ticks['logic'] = tmp_df['logic'] - tmp_df['rhpc_sync']

            # Add extra information
            self.ticks['total_mpi_sync'] = self.ticks[[
                *self.mpi_stages]].sum(axis='columns')

        global_files = glob.glob(f"{folderpath}/global_metrics.p*.csv")
        global_dfs = []
        for global_path in global_files:
            global_df = pd.read_csv(global_path)
            process = int(
                re.match(r'.+global_metrics\.p(\d+)\.csv', global_path)[1])
            global_df['process'] = process
            global_dfs.append(global_df)
        self.global_df = pd.concat(global_dfs)

    @property
    def total_time(self):
        """The total time it took to run the simulation"""
        min_start_time = pd.to_timedelta(self.global_df['epoch'].min(),
                                         unit='nanoseconds')
        max_end_time = pd.to_timedelta(self.global_df['end_time'].max())

        return max_end_time - min_start_time

    @property
    def save_time(self):
        min_start_time = pd.to_timedelta(self.global_df['presave_time'].min())
        max_end_time = pd.to_timedelta(self.global_df['end_time'].max())
        return max_end_time - min_start_time

    def section_time(self) -> dict:
        """Return a dictionary containing per section time"""
        try:
            df = self.ticks.groupby('process').sum() / 1_000_000_000
            df = df[[*self.mpi_stages,
                    'logic',
                    'rhpc_sync']]
            return df.to_dict()
        except AttributeError:
            raise Exception("Run doesn't have section metrics")

    def summary(self):
        return Summary(self)

    def plot(self):
        return Plots(self)


class Summary(object):
    """
    Generate a summary of the performance

    Keyword arguments:

    - metrics -- The performance metrics to extract the data from
    """

    def __init__(self, metrics: Metrics):
        self.number_of_processes = metrics.ticks.process.max() + 1
        self.combined = metrics.ticks.groupby('process').mean()

    def __str__(self):
        output = ''
        output += f"Number of processes: {self.number_of_processes}\n"
        output += f"{self.combined}"
        return output


class Plots(object):
    """
    Collection of different plots based on performance metrics

    Keyword arguments:

    - metrics -- The performance metrics to extract the data from
    """

    # Plot layouts, only for relevant number of process
    layout_map = {
        1: (1, 1),
        2: (2, 1),
        3: (3, 1),
        4: (2, 2),
        5: (3, 2),
        6: (3, 2),
        8: (4, 2),
        9: (3, 3),
        12: (4, 3),
        16: (4, 4)
    }

    supported_plots = ('logic', 'rhpc', 'mpi')

    def __init__(self, metrics: Metrics):
        self.metrics = metrics

    @property
    def __plot_cols_mapper__(self):
        return {
            'logic': ['logic'],
            'rhpc': ['rhpc_sync'],
            'mpi': [*self.metrics.mpi_stages]
        }

    def plot(self, process_number, plot=supported_plots):
        """Simple plot that shows the execution times in each tick"""

        # Verify all the plots exists
        for entry in plot:
            if entry not in self.supported_plots:
                raise Exception((f"Unsupported plot <{entry}>. "
                                 f"Available: {', '.join(self.supported_plots)}"))

        df = self.metrics.ticks
        df = df[df['process'] == process_number]  # Keep only the relevent ps

        # Keep only the interested metrics
        cols = []
        for p in plot:
            cols.extend(self.__plot_cols_mapper__[p])

        df = df[cols]

        df.plot()
        plt.show()

    def pie(self):
        """Plot a pie using the times stored in the metrics"""

        df = self.metrics.ticks.groupby('process').sum()
        df = df[[*self.metrics.mpi_stages,
                 'logic',
                 'rhpc_sync']]

        # Timedeltas must be converted to integers so plt doesn't complain

        df.T.plot.pie(
            subplots=True, layout=self.layout_map[int(df.index.max() + 1)], legend=False)
        plt.show()


# If the file is used as script, process arguments and such
if __name__ == '__main__':

    # Parent parser
    parser = argparse.ArgumentParser(
        description='Simulation performance metrics analyzer')
    parser.add_argument('folder',
                        help='The folder containing the performance metrics')
    parser.add_argument('--no-report',
                        action='store_true',
                        help="Don't print a summary")

    # Subparsers
    subparsers = parser.add_subparsers(dest='subcommand')

    # Simple plot
    plotter = subparsers.add_parser('plot', help='Plot the times of process P')
    plotter.add_argument('process',
                         type=int,
                         help='The process to plot')
    plotter.add_argument('times',
                         choices=('all', *Plots.supported_plots),
                         nargs='+',
                         help='The times to plot')

    # Pie plot
    pieplot = subparsers.add_parser('pie-plot',
                                    help='Plot a pie with the performance times')

    args = parser.parse_args()

    metrics = Metrics(args.folder)

    if not args.no_report:
        print(metrics.summary())

    if args.subcommand == 'pie-plot':
        metrics.plot().pie()

    if args.subcommand == 'plot':
        if args.times[0] == 'all':
            metrics.plot().plot(args.process, plot=Plots.supported_plots)
        else:
            metrics.plot().plot(args.process, args.times)
