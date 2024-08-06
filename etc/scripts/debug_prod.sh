#!/bin/sh
#
# Invokes gdb with the Nucleo-144 dev board

set -e

if [ x"$1" == x ]; then
    echo "Usage: $0 firmware_binary"
	exit 1
fi

openocd -f interface/stlink-v2.cfg -f target/stm32f4x.cfg &
OPENOCD_PID="$!"

trap "kill ${OPENOCD_PID}" EXIT

arm-none-eabi-gdb -q \
	-ex "set confirm off" \
	-ex "target extended-remote localhost:3333" \
	$1
