#!/usr/bin/python3

from pathlib import Path
import argparse
import secrets
import subprocess

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
            return {'x': obj.x, 'y': obj.y}
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

    def __init__(self, width, height, parameters=None):

        self.dimensions = (width, height)
        self.elements = []
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
        if value is not None:
            self.required_parameters.validate(value)
        self._parameters = value

    def add_element(self, element):
        """Add a new 'element' to the Hospital"""
        if not issubclass(element.__class__, HospitalElement):
            raise Exception(
                'The element to add must be subclass of HospitalElement')
        self.elements.append(element)

    def validate(self):
        """Check the parameters and the elements"""

        # Make sure doctors in the map have their respective properties
        for doctor in [x for x in self.elements if isinstance(x, DoctorOffice)]:
            if doctor.specialty not in [d['specialty'] for d in self.parameters['doctors']]:
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

    def plot(self):
        return HospitalPlotter(self)


class HospitalPlotter(object):
    """Render a hospital plan"""

    def __init__(self, hospital: Hospital):
        
        self.plan = [[' ' for y in range(hospital.dimensions[1])]
                    for x in range(hospital.dimensions[0])]
        self.elements = hospital.elements
        
    def to_console(self):
        """Print the hospital to console"""
        for element in self.elements:
            element.put_char_art(self.plan)
        
        for y in range(len(self.plan[0]) - 1, -1, -1):
            for x in range(len(self.plan)):
                print(self.plan[x][y], end='')
            print()


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

    def put_char_art(self, plan):
        """Store this element in a matrix of chars according to its location"""
        plan[self.location.x][self.location.y] = self.char_art


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
        self.doctor_location = Point(*doctor_location)
        self.patient_location = Point(*patient_location)

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

    def put_char_art(self, plan):
        plan[self.doctor_location.x][self.doctor_location.y] = 'D'
        plan[self.patient_location.x][self.patient_location.y] = 'P'

class Wall(HospitalElement):
    """
    A wall

    Keyword arguments:

    - location -- The coordinates of the wall
    """
    unique = False
    store_key = 'walls'
    char_art = '#'

    def __init__(self, location):
        self.location = Point(*location)


class Chair(HospitalElement):
    """
    A chair, for patients to sit in

    Keyword arguments:

    - location -- The coordinates of the chair
    """
    unique = False
    store_key = 'chairs'
    char_art = 'h'

    def __init__(self, location):
        self.location = Point(*location)


class Entry(HospitalElement):
    """
    The entry door to the hospital

    Keyword arguments:

    - location -- The location of the entry door
    """
    unique = True
    store_key = 'entry'
    char_art = 'E'

    def __init__(self, location):
        self.location = Point(*location)


class Exit(HospitalElement):
    """
    The exit door of the hospital

    Keyword arguments:

    - location -- The location of the exit door
    """
    unique = True
    store_key = 'exit'
    char_art = 'X'

    def __init__(self, location):
        self.location = Point(*location)


class ICU(HospitalElement):
    """
    The Intensive Care Unit of the hospital, which currently is only a door.

    Keyword arguments:

    - location -- The location of the ICU entry
    """
    unique = True
    store_key = 'icu'
    char_art = 'I'

    def __init__(self, location):
        self.location = Point(*location)


class Triage(HospitalElement):
    """
    The triage, place where the patient priority is determined. Note: currently
    the triage has no medical personnel

    Keyword arguments:

    - patient_location -- The location of the patient durning attention
    """
    unique = False
    store_key = 'triages'
    char_art = 'T'

    def __init__(self, patient_location):
        self.patient_location = Point(*patient_location)

    def store(self, dictionary):
        data = {
            'patient_location': self.patient_location
        }
        try:
            dictionary['building'][self.store_key].append(data)
        except:
            dictionary['building'][self.store_key] = [data]
    
    def put_char_art(self, plan):
        plan[self.patient_location.x][self.patient_location.y] = self.char_art


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
        self.receptionist_location = Point(*receptionist_location)
        self.patient_location = Point(*patient_location)

    def store(self, dictionary):
        data = {
            'doctor_location': self.receptionist_location,
            'patient_location': self.patient_location
        }
        try:
            dictionary['building'][self.store_key].append(data)
        except:
            dictionary['building'][self.store_key] = [data]

    def put_char_art(self, plan):
        plan[self.receptionist_location.x][self.receptionist_location.y] = 'R'
        plan[self.patient_location.x][self.patient_location.y] = 'P'

class SimulationProperties(object):
    """
    Stores all the properties required by the simulation

    Keyword arguments:
        - x -- Number of processes in which to split the map along the x axis
        - y -- Number of processes in which to split the map along the y axis
        - seconds_per_tick -- Period of a tick in the simulation
        - chair_manager_process -- Rank of the process containing the chair pool
        - reception_manager_process -- Rank of the process containing the reception queue
        - triage_manager_process -- Rank of the process containing the triage queue
        - doctors_manager_process -- Rank of the process containing the doctors queues
        - simulation_seed -- Int used as a random seed for the simulation
        - debug_performance -- Collect performance statistics inside the simulation 
    """

    def __init__(self, x=1, y=1, seconds_per_tick=60, chair_manager_process=0,
                 reception_manager_process=0, triage_manager_process=0,
                 doctors_manager_process=0, simulation_seed=1574454,
                 debug_performance=False):

        self.process_layout = (x, y)
        self.number_of_processes = x * y
        self.seconds_per_tick = seconds_per_tick
        self.chair_manager_process = chair_manager_process
        self.reception_manager_process = reception_manager_process
        self.triage_manager_process = triage_manager_process
        self.doctors_manager_process = doctors_manager_process
        self.simulation_seed = simulation_seed
        self.debug_performance = debug_performance

    @property
    def process_layout(self):
        return self._process_layout

    @process_layout.setter
    def process_layout(self, pl):
        if not isinstance(pl, tuple) or len(pl) != 2:
            raise Exception('Processes layout should be a tuple of size 2')

        if not isinstance(pl[0], int) or not isinstance(pl[1], int):
            raise Exception('Number of process should be of type int')

        if pl[0] < 1 or pl[1] < 1:
            raise Exception('Number of processes should be >=1')

        self._process_layout = pl

    @property
    def seconds_per_tick(self):
        return self._seconds_per_tick

    @seconds_per_tick.setter
    def seconds_per_tick(self, value):
        if not isinstance(value, int) or value < 0:
            raise Exception('seconds_per_tick should be a int > 0')

        self._seconds_per_tick = value

    @property
    def chair_manager_process(self):
        return self._chair_manager_process

    @chair_manager_process.setter
    def chair_manager_process(self, value):
        if not isinstance(value, int):
            raise Exception('chair_manager_process should be an int')
        if not 0 <= value < self.number_of_processes:
            raise Exception(('chair_manager_process should be in the range '
                             f"[0, {self.number_of_processes}"))
        self._chair_manager_process = value

    @property
    def reception_manager_process(self):
        return self._reception_manager_process

    @reception_manager_process.setter
    def reception_manager_process(self, value):
        if not isinstance(value, int):
            raise Exception('reception_manager_process should be an int')
        if not 0 <= value < self.number_of_processes:
            raise Exception(('reception_manager_process should be in the range '
                             f"[0, {self.number_of_processes}"))
        self._reception_manager_process = value

    @property
    def triage_manager_process(self):
        return self._triage_manager_process

    @triage_manager_process.setter
    def triage_manager_process(self, value):
        if not isinstance(value, int):
            raise Exception('triage_manager_process should be an int')
        if not 0 <= value < self.number_of_processes:
            raise Exception(('triage_manager_process should be in the range '
                             f"[0, {self.number_of_processes}"))
        self._triage_manager_process = value

    @property
    def doctors_manager_process(self):
        return self._doctors_manager_process

    @doctors_manager_process.setter
    def doctors_manager_process(self, value):
        if not isinstance(value, int):
            raise Exception('doctors_manager_process should be an int')
        if not 0 <= value < self.number_of_processes:
            raise Exception(('doctors_manager_process should be in the range '
                             f"[0, {self.number_of_processes}"))
        self._doctors_manager_process = value

    @property
    def simulation_seed(self):
        return self._simulation_seed

    @simulation_seed.setter
    def simulation_seed(self, value):
        if not isinstance(value, int):
            raise Exception('simulation_seed should be an int')
        self._simulation_seed = value

    @property
    def debug_performance(self):
        return self._debug_performance

    @debug_performance.setter
    def debug_performance(self, value):
        if not isinstance(value, bool):
            raise Exception('debug_performance should be a bool')
        self._debug_performance = value

    def save(self, folder, run_id):
        """Save the properties to a file"""

        with open(folder/'model.props', 'w') as f:
            f.write('# Run id\n')
            f.write(f"run.id = {run_id}")

            f.write('# Process distribution\n')
            f.write(f"x.process = {self.process_layout[0]}\n")
            f.write(f"y.process = {self.process_layout[1]}\n")
            f.write(f"chair.manager.rank = {self.chair_manager_process}\n")
            f.write(
                f"reception.manager.rank = {self.reception_manager_process}\n")
            f.write(f"triage.manager.rank = {self.triage_manager_process}\n")
            f.write(f"doctors.manager.rank = {self.doctors_manager_process}\n")

            f.write('# Output\n')
            f.write(f"output.folder = {folder.absolute()}\n")

            f.write('# Debug\n')
            f.write(f"debug.performance.metrics = {self.debug_performance}\n")

            f.write('# Hospital file\n')
            f.write(f"hospital.file = {folder.absolute()/'hospital.json'}\n")

            f.write('# Simulation\n')
            f.write(f"seconds.per.tick = {self.seconds_per_tick}\n")

            f.write('# Randomness\n')
            f.write(f"random.seed = {self.simulation_seed}\n")

        with open(folder/'config.props', 'w') as f:
            pass


class Simulation(object):
    """
    Object for executing a new unique simulation. Every new object has a unique
    16 char id

    Keyword arguments:

        - props -- A SimulationProperties object
        - hospital -- An Hospital object
        - folder -- Folder to save the simulation props and output
        - mpiexec -- MPI executable
        - wait_for_debugger -- Pass the debug flag to the given process
    """

    def __init__(self, props=None, hospital=None, output_folder=None, mpiexec=None, wait_for_debugger=None):

        self.id = secrets.token_hex(16)

        self.props = props
        self.hospital = hospital

        if output_folder is not None:
            self.folder = output_folder/self.id
        else:
            self.folder = Path(__file__).parent/'run/'/self.id

        if mpiexec is not None:
            self.mpiexec = mpiexec
        else:
            self.default_mpiexec = Path(
                __file__).parent/'lib/mpich/bin/mpiexec'
        self.wait_for_debugger = wait_for_debugger

    @property
    def id(self):
        return id

    @id.setter
    def id(self, value):
        if len(value) != 16 or not isinstance(value, str):
            raise Exception('id should be 16 char long strings')
        self._id = value

    @property
    def props(self):
        return self._props

    @props.setter
    def props(self, value):
        if value is not None and not isinstance(value, SimulationProperties):
            raise Exception('props must be of type SimulationProperties')
        self._props = value

    @property
    def hospital(self):
        return self._hospital

    @hospital.setter
    def hospital(self, value):
        if value is not None and not isinstance(value, Hospital):
            raise Exception('hospital must be of type Hospital')
        self._hospital = value

    @property
    def folder(self):
        return self._folder

    @folder.setter
    def folder(self, f):
        if not isinstance(f, Path):
            raise Exception('folder should be a path')
        self._folder = f

    @property
    def mpiexec(self):
        return self._mpiexec

    @mpiexec.setter
    def mpiexec(self, value: Path):
        if not isinstance(value, Path):
            raise Exception('mpiexec should be a path')
        if not value.exists() or not value.is_file():
            raise Exception('mpiexec not found')

        self._mpiexec = value

    @property
    def wait_for_debugger(self):
        return self._wait_for_debugger

    @wait_for_debugger.setter
    def wait_for_debugger(self, value):
        if value is not None and isinstance(value, int):
            raise Exception('wait_for_debugger should be an int or None')
        if 0 <= value < self.props.number_of_processes:
            raise Exception(('wait_for_debugger should be in the range '
                             f"[0, {self.props.number_of_processes})"))
        self._wait_for_debugger = value

    def run(self):
        """Execute the simulation, return the subprocess result"""

        self.folder.mkdir()
        self.props.save(self.folder, self.id)
        self.hospital.save(self.folder)

        command = [
            str(self.mpiexec),
            '-np', str(self.props.number_of_process),
            str(Path(__file__).parent/'sti-demo'),
            str(self.folder/'config.props'),
            str(self.folder/'model.props'),
        ]

        if self.wait_for_debugger is not None:
            command.append(f"--debug={self.wait_for_debugger}")

        result = subprocess.run(command)

        return result
