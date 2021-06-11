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

ignore_labels = [i for i in range(5)]


reference = {
    'total_patients': 65713,
    'infected_patients': 817.72,
    'punctual_prevalence': 0.012443809,
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
    'percentage_of_out_of_time_patients': 0.018641973
}

groupby = [
    'label'
]

plot = [
    'total_patients',
    'infected_patients',
    'punctual_prevalence',
    'icu_total_patients',
    'icu_infected_patients',
    'icu_punctual_prevalence',
    'infected_patients_deaths_at_icu',
    'icu_mortality_of_infected_patients',
    'total_infected_patients_by_personal',
    'percentage_infected_patients_by_personal',
    'total_infected_objects',
    'total_infected_patients_by_objects',
    'percentage_infected_patients_by_objects',
    'total_infected_patients_by_patients',
    'percentage_infected_patients_by_patients',
    'icu_rejected_patients',
    'percentage_of_rejections_at_icu',
    'waiting_room_rejected_patients',
    'percentage_of_rejections_at_waiting_room',
    'out_of_time_patients',
    'percentage_of_out_of_time_patients'
]

percentage_cols = [
    'punctual_prevalence',
    'icu_punctual_prevalence',
    'icu_mortality_of_infected_patients',
    'percentage_infected_patients_by_personal',
    'percentage_infected_patients_by_objects',
    'percentage_infected_patients_by_patients',
    'percentage_of_rejections_at_icu',
    'percentage_of_rejections_at_waiting_room',
    'percentage_of_out_of_time_patients'
]

df = pd.read_csv(args.file)
df = df[~df['label'].isin(ignore_labels)]

if not args.print:
    import matplotlib.pyplot as plt
    plt.style.use('ggplot')
    plt.rcParams.update({
        'font.size': 9,
        'figure.titlesize': 10,
        'axes.titlesize': 10
    })

    fig1, axs_1 = plt.subplots(3, 4)
    fig2, axs_2 = plt.subplots(3, 4)
    axs = list(axs_1.flat) + list(axs_2.flat)
    for ax in axs[21:]: ax.remove()
    axs = axs[:21]
    
    axs = df.boxplot(column=plot, by=groupby, ax=axs)

    for ax in axs:
        ax.set_xlabel('')
        if ax.get_title() in percentage_cols:
            ax.set_ylim(0, 1)
        try:
            ax.axhline(reference[ax.get_title()])
        except:
            pass
        
            
    plt.show()

print(df.drop(['run_id'],axis='columns').groupby(by=groupby).agg(['median', 'std']))
