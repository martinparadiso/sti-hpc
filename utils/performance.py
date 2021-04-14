#!/usr/bin/python3

from collections import namedtuple
import argparse
import glob
import matplotlib.pyplot as plt
import pandas as pd
import re
import math


class Metrics(object):
    """Collect metrics from a execution"""

    mpi_sync_stages = ('chairs_sync_ns',
                       'reception_sync_ns',
                       'triage_sync_ns',
                       'doctors_sync_ns',
                       'icu_sync_ns')

    def __init__(self, folderpath):

        # Search for performance files, the name should be 'profiling.pX.csv'
        files_found = glob.glob(f"{folderpath}/profiling.p*.csv")

        dfs = []
        for path in files_found:
            # Read the CSV, append the process number
            df = pd.read_csv(path)
            process = int(re.match(r'.+profiling\.p(\d+)\.csv', path)[1])
            df['process'] = process
            dfs.append(df)

        # Combine all dataframes
        self.dataframe = pd.concat(dfs)

        # Add extra information
        self.dataframe['total_mpi_sync_ns'] = sum(
            [self.dataframe[column] for column in self.mpi_sync_stages])

        print(self.dataframe)
        print(self.dataframe.dtypes)

    def summary(self):
        return Summary(self)

    def plot(self):
        return Plots(self)


class Summary(object):
    """Generate a summary of the performance"""

    def __init__(self, metrics: Metrics):
        self.number_of_processes = metrics.dataframe.process.max() + 1
        self.combined = metrics.dataframe.groupby('process').mean()

    def __str__(self):
        output = ''
        output += f"Number of processes: {self.number_of_processes}\n"
        output += f"{self.combined}"
        return output


class Plots(object):
    """Collection of different plots based on performance metrics"""

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

    @property
    def __plot_cols_mapper__(self):
        return {
            'logic': ['logic_ns'],
            'rhpc': ['rhpc_sync_ns'],
            'mpi': [*self.metrics.mpi_sync_stages]
        }

    def __init__(self, metrics: Metrics):
        self.metrics = metrics

    def plot(self, process_number, plot=supported_plots):
        """Simple plot that shows the execution times in each tick"""

        # Verify all the plots exists
        for entry in plot:
            if entry not in self.supported_plots:
                raise Exception((f"Unsupported plot <{entry}>. "
                                 f"Available: {', '.join(self.supported_plots)}"))

        df = self.metrics.dataframe
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

        # Group by process
        df = self.metrics.dataframe.groupby('process').sum()
        # Keep only times
        df = df[[*self.metrics.mpi_sync_stages,
                 'logic_ns',
                 'rhpc_sync_ns']]
        df.T.plot.pie(
            subplots=True, layout=self.layout_map[int(df.index.max() + 1)])
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
