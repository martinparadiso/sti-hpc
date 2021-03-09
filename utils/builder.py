#!/usr/bin/python3

from collections import namedtuple
import json


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


class Hospital(object):

    def __init__(self, width, height):
        """Construct a new hospital, with the given dimensions"""

        self.data = [[Floor() for y in range(height)] for x in range(width)]
        self.doc_probability = {}

    def add(self, tile: Tile, location: Point):
        """Specify the tile in a given location"""
        self.data[location.x][location.y] = tile

    def border_walls(self):
        """Add walls to the borders"""
        for x in range(len(self.data)):
            if self.data[x][0].__class__ is Floor:
                self.data[x][0] = Wall()
            if self.data[x][-1].__class__ is Floor:
                self.data[x][-1] = Wall()

        for y in range(len(self.data[0])):
            if self.data[0][y].__class__ is Floor:
                self.data[0][y] = Wall()
            if self.data[-1][y].__class__ is Floor:
                self.data[-1][y] = Wall()

    def doctor_probability(self, specialty: str, chance: float):
        """Set the probability of being assigned to a doctor type"""
        self.doc_probability[specialty] = {
            "probability": chance
        }

    def validate_doctors(self):
        """Check the map looking for errors"""

        # Check if every doctor specialty has a probability assigned
        for col in self.data:
            for tile in col:
                if tile.__class__ is Doctor:
                    if not tile.specialty in self.doc_probability:
                        raise Exception(
                            f"Unspecified doctor probability for {tile.specialty}")

        # Make sure the probability of all doctors is 1
        total_doc_p = 0.0
        for doc in self.doc_probability:
            total_doc_p += self.doc_probability[doc]['probability']
        if total_doc_p > 1.0:
            raise Exception(f"Doctor probability over 1: {total_doc_p}")
        if total_doc_p < 1.0:
            raise Exception(f"Doctor probability under 1: {total_doc_p}")

    def validate(self):
        """Check the map looking for errors"""
        self.validate_doctors()

    def dictionary(self):
        out = dict()

        # Jsonify the building plan
        out['building'] = {}
        out['building']['width'] = len(self.data)
        out['building']['height'] = len(self.data[0])
        for x in range(len(self.data)):
            for y in range(len(self.data[x])):
                self.data[x][y].store(out['building'], Point(x, y))

        # Check if all the required data is present
        self.validate()

        out['doctors'] = self.doc_probability

        return out


# Test zone
hospital = Hospital(10, 10)
hospital.border_walls()

hospital.add(Doctor('general_practitioner'), Point(5, 5))
hospital.add(Doctor('radiologist'), Point(5, 7))

hospital.doctor_probability('general_practitioner', 0.3)
hospital.doctor_probability('radiologist', 0.7)

hospital.add(Receptionist(), Point(1, 5))
hospital.add(Triage(), Point(1, 3))
hospital.add(ICU(), Point(9, 9))

hospital.add(Chair(), Point(2, 3))
hospital.add(Chair(), Point(2, 5))
hospital.add(Chair(), Point(2, 7))

hospital.add(Entry(), Point(0, 5))
hospital.add(Exit(), Point(9, 5))

d = hospital.dictionary()

with open('./data/hospital.json', 'w') as f:
    json.dump(d, f, indent=4)

with open('./build/hospital.json', 'w') as f:
    json.dump(d, f, indent=4)
