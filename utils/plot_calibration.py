#!/usr/bin/python3

import argparse
import pandas as pd
import numpy as np

pd.options.display.width = 160
pd.options.display.max_columns = 10

parser = argparse.ArgumentParser(description='Plot calibrations stats')
parser.add_argument('file', help='CSV file containing the statistics')
parser.add_argument('-l', '--label', help='Label to plot, default latest')
parser.add_argument('--print', help='Print only, no plot', action='store_true')
args = parser.parse_args()

max_plot = 2

reference = {
    'total_patients': 65713,
    'infected_patients': 817.72,
    'punctual_prevalence': 0.0119,
    'icu_total_patients': 3706.78,
    'icu_infected_patients': 125.8,
    'icu_punctual_prevalence': 0.033981458,
    'infected_patients_deaths_at_icu': 20.58,
    'icu_mortality_of_infected_patients': 0.249156973,
    'total_infected_patients_by_personal': 157.46,
    'percentage_infected_patients_by_personal': 0.168195397,
    'total_infected_objects': 320.74,
    'total_infected_patients_by_objects': 109.3,
    'percentage_infected_patients_by_objects': 0.135113751,
    'total_infected_patients_by_patients': 425.2,
    'percentage_infected_patients_by_patients': 0.538341956,
    'icu_rejected_patients': 935.2,
    'percentage_of_rejections_at_icu': 0.201270937,
    'waiting_room_rejected_patients': 0.28,
    'percentage_of_rejections_at_waiting_room': 4.26095e-06,
    'out_of_time_patients': 1225.02,
    'percentage_of_out_of_time_patients': 0.018641973,
    'percentage_infected_patients_by_icu': 0.158348896
}

groupby = [
    'label'
]

plot = [
    #'total_patients',
    'infected_patients',
    'punctual_prevalence',
    #'icu_total_patients',
    'icu_infected_patients',
    'icu_punctual_prevalence',
    #'infected_patients_deaths_at_icu',
    #'icu_mortality_of_infected_patients',
    'total_infected_patients_by_personal',
    'percentage_infected_patients_by_personal',
    'total_infected_objects',
    'total_infected_patients_by_objects',
    'percentage_infected_patients_by_objects',
    'total_infected_patients_by_patients',
    'percentage_infected_patients_by_patients',
    #'icu_rejected_patients',
    #'percentage_of_rejections_at_icu',
    #'waiting_room_rejected_patients',
    #'percentage_of_rejections_at_waiting_room',
    #'out_of_time_patients',
    #'percentage_of_out_of_time_patients',
    'total_infected_patients_by_icu',
    'percentage_infected_patients_by_icu'
]

percentage_cols = [
    'punctual_prevalence',
    'icu_punctual_prevalence',
    'icu_mortality_of_infected_patients',
    'percentage_infected_patients_by_personal',
    'percentage_infected_patients_by_objects',
    'percentage_infected_patients_by_patients',
    'percentage_infected_patients_by_icu',
    'percentage_of_rejections_at_icu',
    'percentage_of_rejections_at_waiting_room',
    'percentage_of_out_of_time_patients'
]

parameters = [
    'human_infection',
    'human_contamination',
    'chair_infection',
    'bed_infection',
    'icu_chance'
]

df = pd.read_csv(args.file)

if args.label is not None:
    label = args.label
else:
    label = df['label'].tail(1).item()
df = df[df['label'] == label]

print('Calibración: ')
print(df.drop(['run_id'],axis='columns').groupby(by=groupby).agg(['mean', 'var', 'median', 'std']).T.to_markdown())

print()
print(f"Parameters of run {label}")
print(df[df['label'] == label][parameters].drop_duplicates())

if not args.print:
    import matplotlib.pyplot as plt
    plt.style.use('ggplot')
    plt.rcParams.update({
        'font.size': 9,
        'figure.titlesize': 6,
        'axes.titlesize': 9
    })

    means = df.groupby(by=groupby).mean()[plot]
    errors = df.groupby(by=groupby).var()[plot]

    # Plot mean
    fig, axs = plt.subplots(3, 5, figsize=(16, 9))
    axs = axs.flat
    for ax in axs[len(plot):]: ax.remove()
    axs = axs[:len(plot)]
    
    for ax, value in zip(axs, plot):
        ax = df[value].plot.box(by='label', ax=ax)
        ax = means[value].plot.bar(ax=ax, yerr=errors[value], rot=0)
        ax.set_title(value)
        ax.set_xlabel('')
        if value in reference: ax.axhline(reference[value], color = 'lightblue')
            
    # Plot boxplot
    fig_boxplot, axs_boxplot = plt.subplots(3, 5, figsize=(16, 9))
    axs_boxplot = axs_boxplot.flat
    for ax in axs_boxplot[len(plot):]: ax.remove()
    axs_boxplot = axs_boxplot[:len(plot)]

    for ax, value in zip(axs_boxplot, plot):
        ax = df[['label', value]].boxplot(by='label', ax=ax)
        ax.set_title(value)
        ax.set_xlabel('')
        if value in reference: ax.axhline(reference[value], color = 'lightblue')

    fig.tight_layout()
    fig_boxplot.tight_layout()
    plt.show()
