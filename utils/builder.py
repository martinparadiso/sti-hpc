#!/usr/bin/python3

from collections import namedtuple
import json
from random import randrange


class Point(object):

    def __init__(self, x, y):
        self.x = x
        self.y = y


class Tile(object):

    def store(self, out: dict, p: Point):
        try:
            out[self.key].append(p.__dict__)
        except:
            out[self.key] = [p.__dict__]


class Floor(Tile):

    def store(self, out: dict, p: Point):
        pass


class Wall(Tile):

    key = 'walls'


class Chair(Tile):

    key = 'chairs'


class Entry(Tile):

    def store(self, out: dict, p: Point):
        out['entry'] = p.__dict__


class Exit(Tile):

    def store(self, out: dict, p: Point):
        out['exit'] = p.__dict__


class Triage(Tile):

    def store(self, out: dict, p: Point):
        out['triage'] = p.__dict__


class ICU(Tile):

    def store(self, out: dict, p: Point):
        out['ICU'] = p.__dict__


class Receptionist(Tile):

    key = 'receptionists'


class Doctor(Tile):

    key = 'doctors'

    def __init__(self, specialty: str):
        self.specialty = specialty

    def store(self, out: dict, p: Point):
        data = {
            'type': self.specialty,
            **p.__dict__
        }

        try:
            out[self.key].append(data)
        except:
            out[self.key] = [data]


class SimulationParameters(object):
    """Parameters required by the simulation"""

    required_parameters = {
        'human': {
            'infection': {
                'chance': float,
                'distance': float,
            },
            'incubation': {
                'seconds': int
            }
        },

        'object': {
            'infection': {
                'chance': float,
                'distance': float,
            }
        },

        'patient': {
            'walk_speed': float,
            'infected_chance': float,
        }
    }

    def __init__(self):

        # Fill the parameters with None values
        self.parameters = {}

    def __getitem__(self, key):
        return self.parameters[key]

    def __setitem__(self, key, value):
        self.parameters[key] = value

    def add_doctor(self, specialty):
        if not 'doctors' in self.required_parameters:
            self.required_parameters['doctors'] = {}
        self.required_parameters['doctors'][specialty] = {
            'chance': float
        }

    def validate_probabilities(self):
        """"Make sure probabilities are correct"""

        # All "chance" parameters must be 1 or less
        def validate_chances(d):
            for key in d:
                if key == 'chance' and d['chance'] > 1.0:
                    raise Exception('Probability over 1')
                elif key == 'chance' and d['chance'] < 0.0:
                    raise Exception('Probability under 0')
                else:
                    if d[key].__class__ is dict:
                        validate_chances(d[key])

        validate_chances(self.parameters)

        # Check if doctors probabilities sum 1
        acc = 0.0
        for doctor_specialty in self.parameters['doctors']:
            acc += self.parameters['doctors'][doctor_specialty]['chance']
        if acc > 1.0:
            raise Exception(f"Doctor probability over 1: {acc}")
        if acc < 1.0:
            raise Exception(f"Doctor probability under 1: {acc}")

    def validate(self):
        """Check if all necessary values are present and the probabilities make sense"""

        def recurse(data_tree, param_tree, current_path=[]):

            for key in param_tree:
                try:
                    if param_tree[key].__class__ is dict:
                        recurse(data_tree[key], param_tree[key],
                                current_path=current_path + [key])
                    else:
                        if not data_tree[key].__class__ is param_tree[key]:
                            error_msg = ('Invalid type for simulation parameter '
                                         f"""['{"']['".join(current_path + [key])}']. """
                                         f"Expected {param_tree[key].__name__} "
                                         f"but got {data_tree[key].__class__.__name__}")
                            raise Exception(error_msg)
                except KeyError:
                    error_msg = ('Missing simulation parameter '
                                 f"""['{"']['".join(current_path + [key])}']""")
                    raise Exception(error_msg)

        recurse(self.parameters, self.required_parameters)
        self.validate_probabilities()

        return True


class PatientInflux(object):

    def __init__(self, days=0, intervals=0):
        self.histogram = [[0 for i in range(intervals)] for day in range(days)]

    def __getitem__(self, key):
        return self.histogram[key[0]][key[1]]

    def __setitem__(self, key, value):
        if not value.__class__ is int:
            raise Exception('Value be an int')
        self.histogram[key[0]][key[1]] = value

    def new_day(self, data):
        if not data.__class__ is list:
            raise Exception('Data must be a list')

        self.histogram.append(data)


class Hospital(object):

    def __init__(self, width, height):
        """Construct a new hospital, with the given dimensions"""

        self.data = [[Floor() for y in range(height)] for x in range(width)]
        self.parameters = SimulationParameters()
        self.patient_influx = PatientInflux()

    def __getitem__(self, key):
        return self.data[key[0]][key[1]]

    def __setitem__(self, key, value):
        if not value.__class__ in Tile.__subclasses__():
            raise Exception('Unsupported type')

        # If the tile is a doctor, add it to the parameters
        if value.__class__ is Doctor:
            self.parameters.add_doctor(value.specialty)
        self.data[key[0]][key[1]] = value

    def border_walls(self):
        """Add walls to the borders"""
        for x in range(len(self.data)):
            if self[x, 0].__class__ is Floor:
                self[x, 0] = Wall()
            if self[x, -1].__class__ is Floor:
                self[x, -1] = Wall()

        for y in range(len(self.data[0])):
            if self[0, y].__class__ is Floor:
                self[0, y] = Wall()
            if self[-1, y].__class__ is Floor:
                self[-1, y] = Wall()

    def validate(self):
        """Check the map looking for errors"""
        self.parameters.validate()

    def dictionary(self):
        # Check if all the required data is present
        self.validate()

        out = {}

        # Convert the building plan to dictionary
        out['building'] = {}
        out['building']['width'] = len(self.data)
        out['building']['height'] = len(self.data[0])
        for x in range(len(self.data)):
            for y in range(len(self.data[x])):
                self.data[x][y].store(out['building'], Point(x, y))

        out['parameters'] = self.parameters.parameters
        out['parameters']['patient']['influx'] = self.patient_influx.histogram

        return out


# Test zone
hospital = Hospital(10, 10)
hospital.border_walls()

hospital[5, 5] = Doctor('general_practitioner')
hospital[5, 7] = Doctor('radiologist')

hospital[1, 5] = Receptionist()
hospital[1, 3] = Triage()
hospital[9, 9] = ICU()

for i in range(3, 8, 2):
    hospital[2, i] = Chair()

hospital[0, 5] = Entry()
hospital[9, 5] = Exit()

# Set the parameters

hospital.parameters['human'] = {
    'infection': {
        'chance': 0.2,
        'distance': 2.0
    },

    'incubation': {
        'seconds': 120
    }
}
hospital.parameters['object'] = {
    'infection': {
        'chance': 0.1,
        'distance': 0.2
    }
}
hospital.parameters['patient'] = {
    'walk_speed': 1.0,
    'infected_chance': 0.5
}
hospital.parameters['doctors'] = {}
hospital.parameters['doctors']['radiologist'] = {
    'chance': 0.8
}
hospital.parameters['doctors']['general_practitioner'] = {
    'chance': 0.2
}

# Add the patient admission rate
for day in range(14):
    hospital.patient_influx.new_day([randrange(10, 51) for x in range(12)])

d = hospital.dictionary()
print(json.dumps(d, indent=4))
with open('./data/hospital.json', 'w') as f:
    json.dump(d, f, indent=4)
