#!/usr/bin/python3

import libbenchmark as bench

for x, y in [(1, 1), (1, 2), (1,3), (2,2)]:
    for patients in [p for p in range(15000, 70001, 15000)]:
        print(f"Running {x}x{y} with {patients} patients")
        cfg = bench.LocalConfiguration(tag='normal',
                                    layout=(x, y),
                                    patients=patients,
                                    seconds_per_tick=10,
                                    chair_process=0,
                                    reception_process=0,
                                    triage_process=0,
                                    doctor_process=0)
        batch = bench.RamBatch(f"ram/ram{x}x{y}.{patients}.csv", cfg)
        batch.run()
