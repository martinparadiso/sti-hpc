#!/usr/bin/python3

from json.encoder import JSONEncoder
import random
import numpy as np
import inspect
import json


class Point(object):
    """
    A two dimension point in space

    Keyword arguments:

    - x -- The x coordinate
    - y -- The y coordinate
    """

    def __init__(self, x, y):
        self.x = x
        self.y = y

    @property
    def x(self):
        return self._x

    @x.setter
    def x(self, value):
        if not isinstance(value, int) or value < 0:
            raise Exception('x must be a positive int')
        self._x = value

    @property
    def y(self):
        return self._y

    @y.setter
    def y(self, value):
        if not isinstance(value, int) or value < 0:
            raise Exception('y must be a positive int')
        self._y = value


class TimePeriod(object):
    """
    An interval of time
    """

    def __init__(self, days, hours, minutes, seconds):
        self.days = days
        self.hours = hours
        self.minutes = minutes
        self.seconds = seconds

    @property
    def days(self):
        return self._days

    @days.setter
    def days(self, value):
        if not isinstance(value, int) or value < 0:
            raise Exception('days must be a positive integer')
        self._days = value

    @property
    def hours(self):
        return self._hours

    @hours.setter
    def hours(self, value):
        if not isinstance(value, int) or value < 0 or value > 23:
            raise Exception('hours must be an integer in the range [0, 23]')
        self._hours = value

    @property
    def minutes(self):
        return self._minutes

    @minutes.setter
    def minutes(self, value):
        if not isinstance(value, int) or value < 0 or value > 59:
            raise Exception('minutes must be an integer in the range [0, 59]')
        self._minutes = value

    @property
    def seconds(self):
        return self._seconds

    @seconds.setter
    def seconds(self, value):
        if not isinstance(value, int) or value < 0 or value > 59:
            raise Exception('seconds must be an integer in the range [0, 59]')
        self._seconds = value

class Encoder(JSONEncoder):
    def default(self, obj):
        if isinstance(obj, Point):
            return {'x' : obj.x, 'y': obj.y}
        if isinstance(obj, TimePeriod):
            return {
                'days': obj.days,
                'hours': obj.hours,
                'minutes': obj.minutes,
                'seconds': obj.seconds
            }
        if isinstance(obj, np.ndarray):
            return obj.tolist()
        return json.JSONEncoder.default(self, obj)

class Parameters(object):
    """
    A collection of parameters accessed by key

    Keyword arguments:

    - key -- The key of this collection
    - parameters -- A list of parameters
    """

    def __init__(self, parameters):
        if not isinstance(parameters, set):
            raise Exception('parameters should be a set')

        self.parameters = parameters

    def validate(self, values: dict):
        """Validate a dictionary of values"""
        accumulators = {}
        for parameter in self.parameters:
            parameter.validate(values, accumulators)

        for acc in accumulators:
            if accumulators[acc] < 1:
                raise Exception((f"Probability group {acc} does not sum 1: "
                                 f"{accumulators[acc]}"))


class Parameter(object):
    """
    A parameter required by the hospital

    Keyword arguments:

    - key -- The *key* of the parameter, used for storage
    - type -- The type of the parameter: can be a set of parameters or a type
    - validate -- A function for complex type validation, receives the value to validate,
                  and returns a tuple with a bool and an explanation. True if the value
                  is ok.
    - help -- Description of the parameter
    - prob -- Indicate if the parameter is a probability and needs to be validated
              accordingly
    - pgroup -- If the parameter is a probability, the group to which it belongs,
                in order to perform validations (i.e. all probabilities in the
                group sum 1). If this variable is set, prob is automatically set
                to True
    - islist -- The value is a list of grouped parameters
    """

    def __init__(self, key, type, validate=None, help=None,
                 prob=False, pgroup=None, islist=False):
        if not isinstance(key, str):
            raise Exception('key should be a str')
        self.key = key
        self.type = type
        self.validator = validate
        self.help = help
        self.probability = prob or pgroup is not None
        self.pgroup = pgroup
        self.parent = None
        self.islist = islist

        # If the type is a set of parameters or a list, set the parent key for each child
        if isinstance(self.type, set):
            for parameter in self.type:
                parameter.parent = self

        if isinstance(self.islist, set):
            for parameter in self.islist:
                parameter.parent = self

    @property
    def full_key(self):
        try:
            return f"{self.parent.full_key}{'[]' if self.parent.islist else ''}.{self.key}"
        except:
            return f"{self.key}"

    def validate(self, values, accumulators):
        """Validate a dictionary against this required parameter"""

        # Values musts be either a list or a dict
        if not isinstance(values, (list, dict)):
            raise Exception((f"Wrong value for {self.full_key}. "
                             "Expected a list or dict"))

        if self.key not in values:
            raise Exception(f"Missing parameter {self.full_key}")

        # If the Parameter is a list of Parameters, iterate over all the elements
        # of the value validating each one
        if self.islist:
            if not isinstance(values[self.key], list):
                raise Exception(f"{self.full_key} must be a list")

            if len(values[self.key]) == 0:
                raise Exception(f"List {self.full_key} is empty")
            for element in values[self.key]:

                for req_param in self.type:
                    req_param.validate(element, accumulators)
        else:

            # If the type is a set of parameters, validate each one
            if isinstance(self.type, set):
                for parameter in self.type:
                    if not isinstance(values[self.key], dict):
                        raise Exception(f"{self.key} should be a dict")
                    parameter.validate(values[self.key], accumulators)

            # Otherwise is a concrete type, perform a proper validation
            else:
                if not isinstance(values[self.key], self.type):
                    raise Exception(
                        f"{self.full_key} should be {self.type.__name__}")

                if self.probability:
                    if not 0 <= values[self.key] < 1:
                        raise Exception(
                            f"{self.full_key} outside range [0, 1)")

                if self.pgroup is not None:
                    try:
                        accumulators[self.pgroup] += values[self.key]
                    except:
                        accumulators[self.pgroup] = values[self.key]

                    if accumulators[self.pgroup] > 1:
                        raise Exception(('Probability accumulator overflow for '
                                         f"group {self.pgroup}"))

                if self.validator is not None:
                    ok, diagnose = self.validator(values[self.key])
                    if not ok:
                        raise Exception(f"{self.full_key} {diagnose}")


class Hospital(object):
    """
    A hospital

    Keyword arguments:

    - width -- The width of the building plan
    - height -- the height of the building plan
    """

    required_parameters = Parameters({
        Parameter('human', {
            Parameter('infect_probability', float, prob=True),
            Parameter('infect_distance', float,
                      validate=lambda v: (v >= 0, 'Must be >= 0')),
            Parameter('contamination_probability', float, prob=True),
            Parameter('incubation_time', TimePeriod),
        }),
        Parameter('objects', {
            Parameter('chair', {
                Parameter('infect_probability', float, prob=True),
                Parameter('radius', float,
                          validate=lambda v: (v >= 0, 'Must be >=  0')),
                Parameter('cleaning_interval', TimePeriod)
            }),
            Parameter('bed', {
                Parameter('infect_probability', float, prob=True),
                Parameter('radius', float,
                          validate=lambda v: (v >= 0, 'Must be at least 0')),
                Parameter('cleaning_interval', TimePeriod)
            })
        }),
        Parameter('patient', {
            Parameter('walk_speed', float,
                      validate=lambda x: (x >= 0, 'Must be >= 0')),
            Parameter('infected_probability', float, prob=True),
            Parameter('influx', np.ndarray,
                      validate=lambda v: (len(v.shape) == 2 and v.dtype == 'int64',
                                          'Must be a matrix of ints'))
        }),
        Parameter('reception', {
            Parameter('attention_time', TimePeriod)
        }),
        Parameter('triage', {
            Parameter('attention_time', TimePeriod),
            Parameter('icu', float, prob=True, pgroup='triage_diagnosis'),
            Parameter('doctors_probabilities', islist=True, type={
                Parameter('specialty', str),
                Parameter('probability', float, pgroup='triage_diagnosis')
            }),
            Parameter('levels', islist=True, type={
                Parameter('level', int,
                          validate=lambda x: (x >= 0, 'Must be a positive int')),
                Parameter('probability', float, pgroup='triage_levels'),
                Parameter('wait_time', TimePeriod),
            })
        }),
        Parameter('icu', type={
            Parameter('beds', type=int,
                      validate=lambda x: (x >= 0, 'Must be >= 0')),
            Parameter('environment', {
                Parameter('infection_probability', float, prob=True)
            }),
            Parameter('dead_probability', float, prob=True),
            Parameter('sleep_times', islist=True, type={
                Parameter('time', TimePeriod),
                Parameter('probability', float, pgroup='icu_sleep_times')
            })
        }),
        Parameter('doctors', islist=True, type={
            Parameter('specialty', str),
            Parameter('attention_duration', TimePeriod)
        })
    })

    def __init__(self, width, height, parameters):

        self.dimensions = (width, height)
        self.elements = set()
        self.parameters = parameters

    @property
    def dimensions(self):
        return self._dimensions

    @dimensions.setter
    def dimensions(self, value):
        if not isinstance(value, tuple) or len(value) != 2:
            raise Exception('dimension should ve a tuple with 2 elements')
        if not isinstance(value[0], int) or not isinstance(value[1], int):
            raise Exception('Hospital dimensions should be an int')
        if value[0] < 1 or value[1] < 1:
            raise Exception('Hospital dimensions should be >1')
        self._dimensions = value

    @property
    def parameters(self):
        return self._parameters

    @parameters.setter
    def parameters(self, value):
        self.required_parameters.validate(value)
        self._parameters = value

    def add_element(self, element):
        """Add a new 'element' to the Hospital"""
        if not issubclass(element.__class__, HospitalElement):
            raise Exception(
                'The element to add must be subclass of HospitalElement')
        self.elements.add(element)

    def validate(self):
        """Check the parameters and the elements"""

        # Make sure doctors in the map have their respective properties
        for doctor in {x for x in self.elements if isinstance(x, DoctorOffice)}:
            if doctor.specialty not in {d['specialty'] for d in self.parameters['doctors']}:
                raise Exception(
                    f"Missing parameters for doctor '{doctor.specialty}'")

    def save(self, folder, filename='hospital.json'):
        """Save the hospital to a file"""

        data = {
            'building': {},
            'parameters': self.parameters
        }
        for element in self.elements:
            element.store(data)

        with open(f"{folder}/{filename}", 'w') as f:
            json.dump(data, f, cls=Encoder)


class HospitalElement(object):
    """
    An element inside the hospital with physical presence
    """

    def store(self, dictionary):
        """Store the element in a dictionary"""
        if self.unique:
            dictionary['building'][self.store_key] = self.location
        else:
            try:
                dictionary['building'][self.store_key].append(
                    self.location)
            except:
                dictionary['building'][self.store_key] = [
                    self.location.to_dict]


class DoctorOffice(HospitalElement):
    """
    A doctor inside the simulation

    Keyword arguments:

    - specialty -- The medical specialty
    - doctor_location -- The location of the doctor inside the office
    - patient_location -- The point where the patient will stand durning attention
    """
    unique = False
    store_key = 'doctors'

    def __init__(self, specialty, doctor_location, patient_location):
        self.specialty = specialty
        self.doctor_location = doctor_location
        self.patient_location = patient_location

    def store(self, dictionary):
        data = {
            'specialty': self.specialty,
            'doctor_location': self.doctor_location,
            'patient_location': self.patient_location
        }
        try:
            dictionary['building'][self.store_key].append(data)
        except:
            dictionary['building'][self.store_key] = [data]


class Wall(HospitalElement):
    """
    A wall

    Keyword arguments:

    - location -- The coordinates of the wall
    """
    unique = False
    store_key = 'walls'

    def __init__(self, location):
        self.location = location


class Chair(HospitalElement):
    """
    A chair, for patients to sit in

    Keyword arguments:

    - location -- The coordinates of the chair
    """
    unique = False
    store_key = 'chairs'

    def __init__(self, location):
        self.location = location


class Entry(HospitalElement):
    """
    The entry door to the hospital

    Keyword arguments:

    - location -- The location of the entry door
    """
    unique = True
    store_key = 'entry'

    def __init__(self, location):
        self.location = location


class Exit(HospitalElement):
    """
    The exit door of the hospital

    Keyword arguments:

    - location -- The location of the exit door
    """
    unique = True
    store_key = 'exit'

    def __init__(self, location):
        self.location = location


class ICU(HospitalElement):
    """
    The Intensive Care Unit of the hospital, which currently is only a door.

    Keyword arguments:

    - location -- The location of the ICU entry
    """
    unique = True
    store_key = 'icu'

    def __init__(self, location):
        self.location = location


class Triage(HospitalElement):
    """
    The triage, place where the patient priority is determined. Note: currently
    the triage has no medical personnel

    Keyword arguments:

    - patient_location -- The location of the patient durning attention
    """
    unique = False
    store_key = 'triages'

    def __init__(self, patient_location):
        self.patient_location = patient_location

    def store(self, dictionary):
        data = {
            'patient_location': self.patient_location
        }
        try:
            dictionary['building'][self.store_key].append(data)
        except:
            dictionary['building'][self.store_key] = [data]


class Receptionist(HospitalElement):
    """
    The hospital reception

    Keyword arguments:

    - receptionist_location -- The physical location of the receptionist
    - patient_location -- The physical location of the patient durning admission
    """
    unique = False
    store_key = 'receptionists'

    def __init__(self, receptionist_location, patient_location):
        self.receptionist_location = receptionist_location
        self.patient_location = patient_location

    def store(self, dictionary):
        data = {
            'doctor_location': self.receptionist_location,
            'patient_location': self.patient_location
        }
        try:
            dictionary['building'][self.store_key].append(data)
        except:
            dictionary['building'][self.store_key] = [data]

hospital = Hospital(50, 20, {
    'objects': {
        'chair': {
            'infect_probability': 0.1,
            'cleaning_interval': TimePeriod(0, 2, 0, 0),
            'radius': 0.1
        },
        'bed': {
            'infect_probability': 0.0,
            'radius': 0.0,
            'cleaning_interval': TimePeriod(0, 2, 0, 0)
        }
    },
    'icu': {
        'environment': {
            'infection_probability': 0.1
        },
        'beds': 5,
        'dead_probability': 0.05,
        'sleep_times': [
            {
                'time': TimePeriod(1, 0, 0, 0),
                'probability': 0.2
            },
            {
                'time': TimePeriod(2, 0, 0, 0),
                'probability': 0.2
            },
            {
                'time': TimePeriod(4, 0, 0, 0),
                'probability': 0.2
            },
            {
                'time': TimePeriod(8, 0, 0, 0),
                'probability': 0.2
            },
            {
                'time': TimePeriod(16, 0, 0, 0),
                'probability': 0.2
            }
        ]
    },
    'reception': {
        'attention_time': TimePeriod(0, 0, 15, 0)
    },
    'triage': {
        'icu': 0.2,
        'doctors_probabilities': [
            {
                'specialty': 'general_practitioner',
                'probability': 0.6
            },
            {
                'specialty': 'radiologist',
                'probability': 0.2
            }
        ],
        'levels': [
            {
                'level': 1,
                'probability': 0.2,
                'wait_time': TimePeriod(0, 0, 0, 0)
            },
            {
                'level': 2,
                'probability': 0.2,
                'wait_time': TimePeriod(0, 0, 0, 15)
            },
            {
                'level': 3,
                'probability': 0.2,
                'wait_time': TimePeriod(0, 0, 0, 30)
            },
            {
                'level': 4,
                'probability': 0.2,
                'wait_time': TimePeriod(0, 0, 1, 0)
            },
            {
                'level': 5,
                'probability': 0.2,
                'wait_time': TimePeriod(0, 0, 2, 0)
            }
        ],
        'attention_time': TimePeriod(0, 0, 15, 0)
    },
    'doctors': [
        {
            'attention_duration': TimePeriod(0, 0, 30, 0),
            'specialty': 'general_practitioner'
        },
        {
            'attention_duration': TimePeriod(0, 1, 0, 0),
            'specialty': 'radiologist'
        }
    ],
    'patient': {
        'walk_speed': 0.2,
        'influx': np.array([[random.randrange(2, 10) for i in range(12)] for j in range(365)]),
        'infected_probability': 0.1
    },
    'human': {
        'infect_distance': 2.0,
        'contamination_probability': 0.1,
        'incubation_time': TimePeriod(0, 2, 0, 0),
        'infect_probability': 0.2
    }
})

hospital.add_element(DoctorOffice(
    'general_practitioner', (2, 2), Point(2, 4)))
hospital.add_element(DoctorOffice('radiologist', Point(12, 2), Point(12, 4)))

hospital.validate()
hospital.save('/tmp/')
