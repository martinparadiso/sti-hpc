#!/usr/bin/python3

import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
plt.style.use('ggplot')
import argparse

parser = argparse.ArgumentParser(description='Plot performance')
parser.add_argument('-i', '--include', action='append',
                    help='Regular expression of the labels to plot')
args = parser.parse_args()

df = pd.read_csv('/home/martin/Repositories/sti-hpc/utils/benchmark.csv')

plot_df = pd.DataFrame()

for regex in args.include:
    plot_df = plot_df.append(df[df['label'].str.match(regex)])

plot_df['time'] = pd.to_timedelta(plot_df['time'])
res = plot_df[['label', 'time']].groupby('label').apply(lambda x: np.mean(x))
res['std'] = plot_df[['label', 'time']].groupby('label').apply(lambda x: np.std(x))

plot_df['time'] = plot_df['time'].dt.total_seconds()
ax = plot_df.boxplot(by='label', column='time', rot='90')

ax.set_ylim(0)
plt.show()
print(res)
