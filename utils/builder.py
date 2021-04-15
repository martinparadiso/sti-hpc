#!/usr/bin/python3

from collections import namedtuple
import json
from random import randrange


class Point(object):

    def __init__(self, x, y):
        self.x = x
        self.y = y


class TimePeriod(object):

    def __init__(self, days, hours, minutes, seconds):
        self.days = days
        self.hours = hours
        self.minutes = minutes
        self.seconds = seconds

        # Value validation
        for attr in self.__dict__:
            if not isinstance(self.__dict__[attr], int):
                raise Exception(f"{__class__.__name__}: {attr} must be an int")

        valid_values = (
            ('days', 0, 364),
            ('hours', 0, 23),
            ('minutes', 0, 59),
            ('seconds', 0, 59)
        )

        for key, lower, upper in valid_values:
            if self.__dict__[key] < lower:
                raise Exception(f"{__class__.__name__}: {key} < {lower}")
            if self.__dict__[key] > upper:
                raise Exception(f"{__class__.__name__}: {key} > {upper}")

# Custom JSON serializer


class CustomEncoder(json.JSONEncoder):
    def default(self, obj):
        if isinstance(obj, TimePeriod) or isinstance(obj, Point):
            return obj.__dict__
        return json.JSONEncoder.default(self, obj)


class Tile(object):

    def __init__(self):
        self.location = None

    def store(self, out: dict):
        try:
            out[self.key].append(self.location.__dict__)
        except:
            out[self.key] = [self.location.__dict__]


class Floor(Tile):

    def store(self, out: dict):
        pass


class Wall(Tile):

    key = 'walls'


class Chair(Tile):

    key = 'chairs'


class Entry(Tile):

    def store(self, out: dict):
        out['entry'] = self.location.__dict__


class Exit(Tile):

    def store(self, out: dict):
        out['exit'] = self.location.__dict__


class Triage(Tile):

    key = 'triages'

    def store(self, out: dict):
        data = {
            'location': self.location.__dict__
        }

        try:
            out[self.key].append(data)
        except:
            out[self.key] = [data]


class ICU(Tile):

    def store(self, out: dict):
        out['ICU'] = self.location.__dict__

class Receptionist(Tile):

    key = 'receptionists'

    def __init__(self):
        """Creates a new reception, the patient location stablish the location
           the patient will stand when assigned to this receptionist"""
        self.patient_location = None

    def store(self, out: dict):
        data = {
            'location': self.location.__dict__,
            'patient_chair': self.patient_location.location.__dict__
        }

        try:
            out[self.key].append(data)
        except:
            out[self.key] = [data]


class ReceptionistChair(Tile):

    key = 'receptionists'

    def __init__(self, associated_receptionist: Receptionist):
        self.associated_receptionist = associated_receptionist
        self.associated_receptionist.patient_location = self

    def store(self, out: dict):
        # The receptionist is in charge of storing
        pass


class Doctor(Tile):

    key = 'doctors'

    def __init__(self, specialty: str):
        self.specialty = specialty
        self.patient_location = None

    def store(self, out: dict):
        data = {
            'type': self.specialty,
            **self.location.__dict__,
            'patient_chair': self.patient_location.location.__dict__
        }

        try:
            out[self.key].append(data)
        except:
            out[self.key] = [data]


class DoctorChair(Tile):

    key = 'receptionists'

    def __init__(self, associated_doctor: Doctor):
        self.associated_doctor = associated_doctor
        self.associated_doctor.patient_location = self

    def store(self, out: dict):
        # The receptionist is in charge of storing
        pass


def char_art(tile: Tile, hospital_dims: tuple) -> str:
    """Get the "ASCII" art of a given tile. The extra information is needed for
       the door arrows"""

    lut = {
        Floor: ' ',
        Wall: '#',
        Chair: 'h',
        Triage: 'T',
        ICU: 'I',
        Receptionist: 'R',
        ReceptionistChair: 'h',
        Doctor: 'D',
        DoctorChair: 'h',
    }

    if tile.__class__ in lut:
        return lut[tile.__class__]

    if isinstance(tile, Entry):
        if tile.location.x == 0:
            return '→'
        if tile.location.x == hospital_dims[0]:
            return '←'
        if tile.location.y == 0:
            return '↑'
        if tile.location.y == hospital_dims[1]:
            return '↓'

    if isinstance(tile, Exit):
        if tile.location.x == 0:
            return '←'
        if tile.location.x == hospital_dims[0]:
            return '→'
        if tile.location.y == 0:
            return '↓'
        if tile.location.y == hospital_dims[1]:
            return '↑'

    raise Exception(f"Unsupported type {tile.__class__}")


class SimulationParameters(object):
    """Parameters required by the simulation"""

    required_parameters = {
        'human': {
            'infect_probability': float,
            'infect_distance': float,
            'contamination_probability': float,
            'incubation_time': TimePeriod
        },

        'objects': {
            'chair': {
                'infect_chance': float,
                'radius': float,
                'cleaning_interval': TimePeriod
            }
        },

        'doctors': {},

        'icu': {
            'beds': int,
            'environment': {
                'infection_chance': float
            },
            'death_probability': float,
            'sleep_time': [
                {
                    'time': TimePeriod,
                    'probability': float
                }
            ]
        },

        'patient': {
            'walk_speed': float,
            'infected_chance': float,
        },

        'reception': {
            'attention_time': TimePeriod
        },
        'triage': {
            'attention_time': TimePeriod,
            'icu': {
                'chance': float
            },
            'doctors_probabilities': {

            }
        },

    }

    def __init__(self):

        # Fill the parameters with None values
        self.parameters = {}

    def __getitem__(self, key):
        return self.parameters[key]

    def __setitem__(self, key, value):
        self.parameters[key] = value

    def add_doctor(self, specialty):

        # Add the required parameters: attention time and probability
        self.required_parameters['doctors'][specialty] = {
            'attention_duration': TimePeriod
        }
        self.required_parameters['triage']['doctors_probabilities'][specialty] = {
            'chance': float
        }

    def add_triage_level(self, level, probability, wait_time):
        """Set the triage levels and the probabilities of each"""
        if not isinstance(level, int):
            raise Exception('Triage level must be an int')

        # Validate the data
        if not isinstance(probability, float):
            raise Exception('Triage level probability must be a float')

        if probability < 0.0 or probability > 1.0:
            raise Exception(f"Wrong probability {probability}")

        if not isinstance(wait_time, TimePeriod):
            raise Exception('wait_time must be of type TimePeriod')

        if not 'triage' in self.parameters:
            self.parameters['triage'] = {}

        if not 'levels' in self.parameters['triage']:
            self.parameters['triage']['levels'] = {}

        self.parameters['triage']['levels'][str(level)] = {
            'chance': probability,
            'wait_time': wait_time
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
                    if isinstance(d[key], dict):
                        validate_chances(d[key])

        validate_chances(self.parameters)

        # Check if probabilities sum 1
        doctriage_acc = self.parameters['triage']['icu']['chance']
        for specialty in self.parameters['triage']['doctors_probabilities']:
            doctriage_acc += self.parameters['triage']['doctors_probabilities'][specialty]['chance']
        if doctriage_acc < 0.0 or doctriage_acc > 1.0:
            raise Exception(f"Doctor/ICU probabilities sum {doctriage_acc}")

        triage_level_acc = 0.0
        for triage_level in self.parameters['triage']['levels']:
            triage_level_acc += self.parameters['triage']['levels'][triage_level]['chance']
        if triage_level_acc < 0.0 or triage_level_acc > 1.0:
            raise Exception(
                f"Triage probabilities don't add 1: {doctriage_acc}")

    def validate(self):
        """Check if all necessary values are present and the probabilities make sense"""

        def recurse(data_tree, param_tree, current_path=[]):

            for key in param_tree:
                try:
                    if isinstance(param_tree[key], dict):
                        recurse(data_tree[key], param_tree[key],
                                current_path=current_path + [key])
                    else:
                        if not isinstance(data_tree[key], list) and not isinstance(data_tree[key], param_tree[key]):
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
        if not isinstance(value, int):
            raise Exception('Value be an int')
        self.histogram[key[0]][key[1]] = value

    def new_day(self, data):
        if not isinstance(data, list):
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
        if isinstance(value, Doctor):
            self.parameters.add_doctor(value.specialty)

        self.data[key[0]][key[1]] = value
        point = Point(key[0], key[1])
        if point.x > len(self.data) or point.y > len(self.data[0]):
            raise IndexError()
        point.x %= len(self.data)
        point.y %= len(self.data[0])
        self.data[key[0]][key[1]].location = point

    def border_walls(self):
        """Add walls to the borders"""
        for x in range(len(self.data)):
            if isinstance(self[x, 0], Floor):
                self[x, 0] = Wall()
            if isinstance(self[x, -1], Floor):
                self[x, -1] = Wall()

        for y in range(len(self.data[0])):
            if isinstance(self[0, y], Floor):
                self[0, y] = Wall()
            if isinstance(self[-1, y], Floor):
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
                self.data[x][y].store(out['building'])

        out['parameters'] = self.parameters.parameters
        out['parameters']['patient']['influx'] = self.patient_influx.histogram

        return out

    def char_art(self) -> str:
        """Get the map formatted in ASCII string"""
        string = ""

        for y in range(len(self.data[0]) - 1, -1, -1):
            string += f"{y:2d} │"
            for x in range(len(self.data)):
                string += char_art(self[x, y], (x, y))
            string += '\n'

        string += f"───┼{''.join(['─' for x in range(len(self.data))])}\n"
        string += f"   │{''.join([str(x % 10) for x in range(len(self.data))])}"

        return string


# Test zone
hospital = Hospital(10, 10)
hospital.border_walls()

hospital[3, 8] = Doctor('general_practitioner')
hospital[3, 7] = DoctorChair(hospital[3, 8])
hospital[5, 8] = Doctor('radiologist')
hospital[5, 7] = DoctorChair(hospital[5, 8])

hospital[8, 2] = Receptionist()
hospital[8, 3] = ReceptionistChair(hospital[8, 2])
hospital[5, 1] = Triage()
hospital[8, 8] = ICU()

for i in range(1, 4, 1):
    hospital[i, 2] = Chair()

hospital[0, 5] = Entry()
hospital[9, 5] = Exit()

# Set the parameters
hospital.parameters['human'] = {
    'infect_probability': 0.8,
    'infect_distance': 2.0,
    'contamination_probability': 0.1,
    'incubation_time': TimePeriod(0, 2, 0, 0)
}

hospital.parameters['objects'] = {
    'chair': {
        'infect_chance': 0.1,
        'radius': 0.2,
        'cleaning_interval': TimePeriod(0, 2, 0, 0)
    },

    'bed': {
        'infect_chance': 0.1,
        'radius': 0.5,
        'cleaning_interval': TimePeriod(0, 2, 0, 0)
    }
}

hospital.parameters['patient'] = {
    'walk_speed': 0.5,
    'infected_chance': 0.5
}

hospital.parameters['reception'] = {
    'attention_time': TimePeriod(0, 0, 15, 0)
}

hospital.parameters['triage'] = {
    'attention_time': TimePeriod(0, 0, 15, 0),
    'icu': {
        'chance': 0.1
    },
    'doctors_probabilities': {
        'radiologist': {
            'chance': 0.2
        },
        'general_practitioner': {
            'chance': 0.7
        }
    }
}

hospital.parameters['icu'] = {
    'beds': 5,
    'environment': {
        'infection_chance': 0.01
    },
    'death_probability': 0.1,
    'sleep_time': [
        {
            'time': TimePeriod(1, 0, 0, 0),
            'probability': 0.2,
        },
        {
            'time': TimePeriod(2, 0, 0, 0),
            'probability': 0.2,
        },
        {
            'time': TimePeriod(3, 0, 0, 0),
            'probability': 0.2,
        },
        {
            'time': TimePeriod(4, 0, 0, 0),
            'probability': 0.2,
        },
        {
            'time': TimePeriod(5, 0, 0, 0),
            'probability': 0.2,
        },
    ]
}

hospital.parameters['doctors'] = {}
hospital.parameters['doctors']['radiologist'] = {
    'attention_duration': TimePeriod(0, 0, 30, 0)
}
hospital.parameters['doctors']['general_practitioner'] = {
    'attention_duration': TimePeriod(0, 0, 30, 0)
}
hospital.parameters.add_triage_level(1, 0.2, TimePeriod(0, 0, 0, 0))
hospital.parameters.add_triage_level(2, 0.2, TimePeriod(0, 0, 10, 0))
hospital.parameters.add_triage_level(3, 0.2, TimePeriod(0, 1, 0, 0))
hospital.parameters.add_triage_level(4, 0.2, TimePeriod(0, 2, 0, 0))
hospital.parameters.add_triage_level(5, 0.2, TimePeriod(0, 4, 0, 0))

# Only one patient
# hospital.patient_influx.new_day([1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0])
# for day in range(1, 14):
#     hospital.patient_influx.new_day([0 for x in range(12)])

# Random patients
for day in range(365):
    hospital.patient_influx.new_day([randrange(5, 10) for x in range(12)])

d = hospital.dictionary()
with open('./data/hospital.json', 'w') as f:
    json.dump(d, f, indent=4, cls=CustomEncoder)

print(hospital.char_art())
