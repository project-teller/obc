#!/bin/sh

set -e

if [ x"$2" == x ]; then
    echo "Usage: $0 openocd_config firmware_binary"
	exit 1
fi

openocd -f $1 &
OPENOCD_PID="$!"

trap "kill ${OPENOCD_PID}" EXIT

arm-none-eabi-gdb -q \
	-ex "set confirm off" \
	-ex "target extended-remote localhost:3333" \
	$2
