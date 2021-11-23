# Neighbour Awareness Networking

OpenNAN is an open source implementation of a Neighbor Awareness Networking stack for Linux written in C.


## Requirements

To use NAN, you will need a Wi-Fi card supporting active monitor mode with frame injection.


## Build

The project is build using CMake. To build the project, simply clone this repositry in `<NANDIR>` and run
```sh
cd <NANDIR>
mkdir build
cd build
cmake ..
make
```

NAN requires `libpcap` for frame injection and reception and `libev` for event handling which have to be installed on the target system.
On Debian Linux, install all dependencies via
```sh
apt install libpcap-dev libev-dev libnl-3-dev libnl-genl-3-dev libnl-route-3-dev
```


## Use

Simply run

```sh
<NANDIR>/build/daemon/nan -i <WLAN_IFACE>
```

You may increase the log level with `-v` and `-vv`. For other options, have a look at `daemon/nan.c`.

## Architecture



## Code Structure

We provide a coarse structure of the most important components and files to facilitate navigating the code base.
For more detailed info on functions consult the corresponding header file.

* `daemon/` Contains the active components that interact with the system.
  * `core.{c,h}` Schedules all relevant functions on the event loop.
  * `io.{c,h}` Platform-specific functions to send and receive frames.
  * `netutils.{c,h}`  Platform-specific functions to interact with the system's networking stack.
  * `nan.c` Contains `main()` and sets up the `core` based on user arguments.
* `googletest/` The runtime for running the tests.
* `src/` Contains platform-independent NAN code.
  * `frame.{h}` The corresponding header file contains the definitions of all NAN frame types
  * `rx.{c,h}` Functions for handling received data and action frames including parsing.
  * `schedule.{c,h}` Functions to determine *when* and *which* frames should be sent.
  * `state.{c,h}` Consolidates the NAN state.
  * `sync.{c,h}` Synchronization: mangaging discovery windows and adjusting the TSF.
  * `tx.{c,h}` Crafting valid data and action frames ready for transmission.
  * `wire.{c,h}` Mini-library for safely reading and writing primitives types from and to a frame buffer.
  * `peers.{c,h}` No NAN functionality. Used for displaying peer devices. 
* `tests/` The actual test cases for this repository.
* `README.md` This file.


## Current Limitations/TODOs


Thanks to
**Milan Stute** ([email](mailto:mstute@seemoo.tu-darmstadt.de), [web](https://seemoo.de/mstute))
for providing an AWDL implementation that was of great use during the development of this project.
