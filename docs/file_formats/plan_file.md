# Plan File

## File Description

The building plan is stored in a binary file, where each tile of the discrete
plan is encoded as an 8 bit integer. The file also contains a header with
information about the plan.

```
Byte
0   | 0x50  | 0x4C  | 0x41  | VER   | # file format VERsion
4   |       NUMBER OF COLUMNS       | # Stored as little-endian unsigned 32 bit integer
8   |         NUMBER OF ROWS        | # Stored as little-endian unsigned 32 bit integer
12  |            RESERVED           |
16  | Tiles ...                     | # Map tiles, as unsigned 8 bit integer
...
```

The tiles are stored as unsigned 8 bit integer, `uint8_t` in C. The plan is
stored column wise, starting with row 0, column 0.

The tile's codes are grouped by class, the grouping is specified in the next
table:

| Code (dec)    | Code (hex)    | Group meaning         | Max. number of types  |
|---------------|---------------|-----------------------|-----------------------|
| `[00:15]`     | `[0x00:0x0F]` | Structures            | 16                    |
| `[16:31]`     | `[0x10:0x1F]` | Interactive Elements  | 16                    |
| `[32:63]`     | `[0x20:0x3F]` | Unused                | -                     |
| `[64:95]`     | `[0x40:0x5F]` | Entry points/zones    | 32                    |
| `[96:127]`    | `[0x60:0x7F]` | Personnel             | 32                    |
| `[128:255]`   | `[0x80:0xFF]` | Physician of type *n* | 128                   |

With the following specific values already in use:

| Code (dec)    | Code (hex)    | Meaning               |
|---------------|---------------|-----------------------|
| `00`          | `0x00`        | Floor/empty space     |
| `01`          | `0x01`        | Wall                  |
| `16`          | `0x10`        | Chair                 |
| `64`          | `0x40`        | Entry                 |
| `65`          | `0x41`        | Exit                  |
| `66`          | `0x42`        | Triage                |
| `67`          | `0x43`        | ICU entry             |
| `96`          | `0x60`        | Receptionist          |

To allow for an arbitrary number of medical specialties, the format does not
define any kind of physician. It is up to the user to define them.

*Note*: the value `128` is normally used to define the general practitioner.
