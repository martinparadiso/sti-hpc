#!/usr/bin/python3

import argparse
import pandas as pd
import numpy as np

pd.options.display.width = 160
pd.options.display.max_columns = 10

parser = argparse.ArgumentParser(description='Plot calibrations stats')
parser.add_argument('file', help='CSV file containing the statistics')
parser.add_argument('--print', help='Print only, no plot', action='store_true')
args = parser.parse_args()

df = pd.read_csv(args.file)

if not args.print:
    import matplotlib.pyplot as plt
    plt.style.use('ggplot')
    axs = df.boxplot(column=['icu_pp', 'nonicu_pp'], by=[
                     'infection_chance', 'icu_chance'], layout=(2, 1))
    for ax in axs:
        ax.set_ylim(0)
    plt.show()

print(df.drop(['run_id', 'icu_infected', 'nonicu_infected'], axis='columns').groupby(by=['infection_chance', 'icu_chance']).agg(
    ['median', 'std']))
