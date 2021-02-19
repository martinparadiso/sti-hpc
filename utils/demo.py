#!/usr/bin/python3

import plan

hospital = plan.BuildingPlan(30,30)
for i in range(30):
    hospital.add( (0, i), plan.Wall() )
    hospital.add( (i, 0), plan.Wall() )
    hospital.add( (29,i), plan.Wall() )
    hospital.add( (i,29), plan.Wall() )

hospital.add( (10,10), plan.Doctor(10) )

hospital.plot()
