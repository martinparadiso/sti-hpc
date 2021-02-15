#!/usr/bin/python3
"""Arbitrary hospital for testing only
"""

import pathlib
import sys

# Add the plan module directory to the modules path
plan_dir = pathlib.Path(__file__).absolute().parents[2]/'utils'
sys.path.append(str(plan_dir))
import plan

hospital = plan.BuildingPlan(50, 50)

# Add walls to the borders
for i in range(50):
    hospital.add( (i,  0), plan.Wall() )
    hospital.add( (i, 49), plan.Wall() )
    hospital.add( (0,  i), plan.Wall() )
    hospital.add( (49, i), plan.Wall() )

# Add an entry and exit in the bottom center
hospital.add( (23, 0), plan.Entry() )
hospital.add( (24, 0), plan.Exit() )

# Add the reception
for i in range(6):
    hospital.add( (21 + i,  5), plan.Wall() )
hospital.add( (23, 4), plan.Receptionist() )

# Add the triage
for i in range(5):
    hospital.add( (30 + i, 2), plan.Wall() )
    hospital.add( (30 + i, 6), plan.Wall() )
    hospital.add( (35, 2 + i), plan.Wall() )
hospital.add( (30, 4), plan.Triage() )

# Add the 128 doctor types in batches of 32
for y in range(4):
    for x in range(32):
        # Add the doctors in order in rows 10, 11, 12, 13 starting in x = 10
        hospital.add((x + 10, y + 10), plan.Doctor(y * 32 + x))

# Add a group of chairs
for x in range(0, 5, 2):
    for y in range(0, 5, 2):
        hospital.add( (20 + x, 20 + y), plan.Chair() )

# ICU zone
for i in range(48, 39, -1):
    hospital.add((i, 40), plan.Wall())
    hospital.add((40, i), plan.Wall())
hospital.add((44, 40), plan.ICU())

hospital.write('test.hosp')