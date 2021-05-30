#!/usr/bin/python3

import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
plt.style.use('ggplot')

exclude = ['28-triage_manager_rank_1']

df = pd.read_csv('/home/martin/Repositories/sti-hpc/utils/benchmark.csv')

# df = df[(df['label'].isin(('reference', '20200528'))) | (df['label'].str.startswith('29-'))]
df = df[~(df['label'].isin(exclude))]

df['time'] = pd.to_timedelta(df['time'])
res = df[['label', 'time']].groupby('label').apply(lambda x: np.mean(x))
res['std'] = df[['label', 'time']].groupby('label').apply(lambda x: np.std(x))

df['time'] = df['time'].dt.total_seconds()
ax = df.boxplot(by='label', column='time', rot='90')

ax.set_ylim(0)
plt.show()
print(res)
