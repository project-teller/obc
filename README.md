# TELLER OBC

This repository contains the code running on the OBC of the TELLER REXUS experiment module.
The code is equipped with a hardware abstraction layer (HAL) so it can be compiled for three
targets:

- The actual TELLER OBC hardware, based on an STM32F415RG MCU
- The STM32 Nucleo H743ZI development board, which was used during the prototyping phase until
  the real OBC hardware became available
- POSIX-compliant operating systems for SITL (software-in-a-loop) tests

More details about the code and the OBC itself can be found in the submitted final version of
the SED, including the documentation of the telemetry protocol used between the experiment
module and the ground station and the format used for onboard experiment data logs.

## Compilation

Install [CMake][1], then run this from the root of the repo:

```bash
mkdir build
cd build
cmake --preset default -B . -S ..
```

CMake will take care of fetching all required dependencies (including the STM32 SDKs) and
invoking the compiler with the right arguments.

There are three build presets available:

- `default` - builds the code for the real OBC hardware
- `devboard` - builds the code for the STM32c Nucleo H743ZI development board
- `host` - builds the code for the host machine (Linux and macOS tested, Windows probably not supported)

[1]: https://cmake.org
