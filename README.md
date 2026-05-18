# Tiwut-ENV

Tiwut-ENV is a pure, independent Wayland environment base.
It initializes a bare `wayland-server` socket with zero third-party dependencies, licenses, or bloat. 

It is completely yours to build upon. Since it drops `wlroots`, you are now required to write the rendering (DRM/KMS/EGL) and input (libinput) modules yourself if you want an end-to-end compositor.

## Building

To build Tiwut-ENV, you need only:
- `wayland-server` (usually found in `libwayland-dev` or `wayland-devel`)
- `cmake`
- `g++`

Run the following commands:
```sh
mkdir build && cd build
cmake ..
make
```

## Running
After building, run the executable:
```sh
./tiwut-env
```
---

## Install
```sh
./setup.sh
```
### Run
```sh
./build/tiwut-env
```
