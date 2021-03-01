#!/usr/bin/python3

from typing import Tuple
from collections import namedtuple
import pathlib
import struct

# Config =======================================================================

plot_assets_path = pathlib.Path(__file__).parent.absolute()/"plan_assets"
Dimension = namedtuple('Dimensions', ['width', 'height'])
sprite_size = Dimension(24, 24)

# Tiles ========================================================================


class Tile():
    pass


class Floor(Tile):
    code = 0


class Wall(Tile):
    code = 1
    img_file = 'wall.jpg'


class Chair(Tile):
    code = 16
    img_file = 'chair.jpg'


class Entry(Tile):
    code = 64
    img_file = 'entry.jpg'


class Exit(Tile):
    code = 65
    img_file = 'exit.jpg'


class Triage(Tile):
    code = 66
    img_file = 'triage.jpg'


class ICU(Tile):
    code = 67
    img_file = 'icu.jpg'


class Receptionist(Tile):
    code = 96
    img_file = 'receptionist.jpg'


class Doctor(Tile):

    img_file = 'doctor.jpg'

    code_range = [i for i in range(128, 256)]

    def __init__(self, type = 0):

        # There maximum number of doctors types is 128
        if not 0 <= type <= 127:
            raise Exception(
                "Invalid Doctor type {type}, value must be between 0 and 127")

        self.code = int(type) + 128

    def decode(self, type):
        """Decode the doctor type"""
        if not 128 <= type <= 255:
            raise Exception(
                "Invalid Doctor type {type}, value must be between 128 and 255")

        self.code = type - 128


# Main Classes =================================================================


class Header():
    """Plan file header"""

    magic_numbers = (b'P', b'L', b'A')
    supported_versions = (1,)

    """
    Header format:
        - [char, char, char]  Magic Numbers
        - [unsigned  8 bits]  Version
        - [unsigned 32 bits]  Number of columns
        - [unsigned 32 bits]  Number of rows
        - [         32 bits]  Reserved
    """
    header_format = '<cccBIII'
    size = 16

    def __init__(self, columns=0, rows=0):
        self.columns = columns
        self.rows = rows

    def to_bytes(self):
        return struct.pack(self.header_format,
                           self.magic_numbers[0],
                           self.magic_numbers[1],
                           self.magic_numbers[2],
                           self.supported_versions[0],
                           self.columns,
                           self.rows,
                           0)

    def from_bytes(self, buffer):
        if len(buffer) != self.size:
            raise Exception(
                f"Wrong buffer size, got {len(buffer)} instead of {self.size}")

        data = struct.unpack(self.header_format, buffer)

        # Check magic numbers
        for i in range(3):
            if (data[i] != self.magic_numbers[i]):
                raise Exception('Unknown magic numbers')

        # Check version
        if not data[3] in self.supported_versions:
            raise Exception('Unsupported version')

        # Load dimensions
        self.columns = data[4]
        self.rows = data[5]


class BuildingPlan():
    """Represents a building plan BuildingPlan"""
    plotter_loaded = False

    def __init__(self, columns=0, rows=0):
        self.header = Header(columns, rows)

        # Generate an empty map
        self.data = [[Floor() for x in range(rows)] for x in range(columns)]

    def add(self, coord: Tuple[int, int], type):
        """Insert a tile in a specific coordinate
        """

        x = coord[0]
        y = coord[1]
        if (x < 0 or y < 0):
            raise Exception(f"Invalid negative coordinates: ({x},{y})")

        self.data[x][y] = type

    def to_bytes(self):

        header_bytes = self.header.to_bytes()
        tiles_bytes = bytes(
            [tile.code for column in self.data for tile in column])

        return header_bytes + tiles_bytes

    def from_bytes(self, buffer):
        """Update the object from byte data"""

        def decode(code):
            for ConcreteTile in Tile.__subclasses__():
                try:
                    if code == ConcreteTile.code:
                        return ConcreteTile()
                except:
                    if code in ConcreteTile.code_range:
                        ct = ConcreteTile()
                        ct.decode(code)
                        return ct

            raise Exception(f"Unknown tile {code}")

        # Recreate the header
        self.header = Header()
        self.header.from_bytes(buffer[:16])
        height = self.header.rows
        width = self.header.columns

        if len(buffer[16:]) != width * height:
            raise Exception('Wrong file size')

        matrix = [buffer[16 + height * i: 16 + height * i + height]
                  for i in range(width)]

        # self.data = [[Floor() for x in range(rows)] for x in range(columns)]
        self.data = [
            [decode(code) for code in row] for row in matrix
        ]

    def plot(self):
        """Plot the map as an image
        """

        # if not self.__class__.plotter_loaded:

        # Delayed import
        from PIL import Image

        # Load all images, add the Image object to each class
        for tile_class in Tile.__subclasses__():

            try:
                tile_class.img_file
            except:
                pass
            else:
                try:
                    sprite_path = plot_assets_path/tile_class.img_file
                    tile_class.sprite = Image.open(
                        sprite_path).resize(sprite_size)

                except:
                    print(f"Couldn't open file {sprite_path}, using red image")
                    tile_class.sprite = Image.new(
                        'RGB', sprite_size, (255, 0, 0))

            self.__class__.plotter_loaded = True

        img_size = Dimension(self.header.columns * sprite_size.width,
                             self.header.rows * sprite_size.height)

        self.image = Image.new('RGB', img_size, color=(255, 255, 255))

        for x in range(self.header.columns):
            for y in range(self.header.rows):
                try:
                    # NOTE: PIL Coordinates start at upper left corner, map
                    # starts at lower left corner
                    self.image.paste(self.data[x][y].__class__.sprite,
                                     (x * sprite_size.width,
                                      img_size.height - (y + 1) * sprite_size.height))
                except AttributeError:
                    pass

        self.image.show()

    def write(self, path):
        """Save the map to a file"""

        with open(path, 'wb') as output:
            output.write(self.to_bytes())


def load(path: str):
    """Load a file map"""

    # Load as bytes
    with open(path, 'rb') as f:
        buffer = f.read()

    building = BuildingPlan()
    building.from_bytes(buffer)

    return building

# Run as script ================================================================


if __name__ == '__main__':
    raise Exception('Unimplemented')
