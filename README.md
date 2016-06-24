# XPD-MPIIO-Driver (XMD)
This is a PMPI library that allows MPI-IO to utilize in-memory storage of the Kove XPD.

## Current Status

### Fully supported (i.e., should be MPI compliant, otherwise report this as an error):

-  MPI\_File\_open
-  MPI\_File\_close
-  MPI\_File\_write\_at
-  MPI\_File\_read\_at
-  MPI\_File\_write
-  MPI\_File\_read
-  MPI\_File\_sync
-  MPI\_File\_seek
-  MPI\_File\_get\_position
-  MPI\_File\_set\_size
-  MPI\_File\_get\_size
-  MPI\_File\_preallocate

#### Collective operations are treaded as independent operations:
-  MPI\_File\_read\_at\_all
-  MPI\_File\_write\_at\_all

### Partly supported:

- MPI\_File\_set\_atomicity (Pretends to support atomic mode)
- MPI\_File\_get\_atomicity (Pretends to support atomic mode)
- MPI\_File\_delete (Would be easy to fix.)
- MPI\_File\_set\_view (Only Byte arrays are supported)
-- Derived datatypes are NOT supported, yet

## Requirements

### For the dummy version

- There are no particular relevant requirements to run the dummy version.

### For the live version

- KOVE XPD with volumes

## Running code

You have to prefix the application with the library
- LD_PRELOAD=./libmpi-xpd.so mpiexec <WHATEVER_YOU USUALLY RUN>

To store data on the XPD you have to prefix file names with xpd: then add your connection string:
e.g., xpd:mlx5_0.1/260535867639876.9:e58c053d-ac6b-4973-9722-cf932843fe4e[+mlx...]
Otherwise the native MPI version is used to access the file.

Always create an empty file by formatting the volume (initializes the file size to 0):
- mpio-xpd-format.exe <CONNECTION_STRING>

## Development

### Directory structure

- dev contains stuff for development purpose

- src contains the source code
  To build the project call:

  		./configure
		cd build
		make -j
