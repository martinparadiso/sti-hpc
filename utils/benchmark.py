#!/usr/bin/python3

import libbenchmark as bench

batch = bench.Batch('/mnt/ssd/data/Martin/Nextcloud/Facultad/PPS/benchmark.csv')

layouts = [(x, y) for x in range(1,5) for y in range(1,5)]
done = layouts[:7]
layouts = [l for l in layouts if l[0] * l[1] <= 6 and l not in done]

for layout in layouts[0:]:
    for patients in [0, 5000, *range(10000, 70001, 10000)]:

            batch.add_configurations(5, 
                bench.LocalConfiguration(tag='normal',
                    layout=layout,
                    patients=patients,
                    seconds_per_tick=10,
                    chair_process=0,
                    reception_process=0,
                    triage_process=0,
                    doctor_process=0))

# print(batch.report())
batch.run(continue_from=127)
