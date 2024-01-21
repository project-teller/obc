#!/usr/bin/env python3
"""Python script to estimate the amount of data the OBC is going to store in
the onboard logs and the amount of data the OBC is going to send to the
RXSM.

The script has an initial section with lots of constants. Feel free to adjust
these to test different scenarios. The rest is code. Modify that if you know
what you are doing.
"""

from struct import calcsize

###############################################################################

SCINTILLATOR_BIN_COUNT = 128

LOG_RATES_HZ = {
    "imu": 50,
    "compass": 10,
    "sun": 10,
    "gmm": 50,
    "scm": 50
}

LOG_RECORD_FORMATS = {
    "imu": "<Iffffff",   # timestamp, accx, accy, accz, gyrx, gyry, gyrz
    "compass": "<Ifff",  # timestamp, magx, magy, magz
    "sun": "<Iff",       # timestamp, angle1, angle2
    "gmm": "<IBBBBBBBBBB",  # timestamp, count1-4, coinc1-6
    "scm": "<I" + str(SCINTILLATOR_BIN_COUNT * 3) + "s"
}

TELEM_RATES_HZ = {
    "imu": 50,
    "compass": 10,
    "sun": 0,
    "gmm": 50,
    "scm": 1,
    "heartbeat": 2,
    "time": 0.2
}

TELEM_FRAME_OVERHEAD = 8

TELEM_RECORD_FORMATS = {
    # stuff not mentioned here is the same as for the log records
    "heartbeat": "<IBBBBH",  # timestamp, error code, voltage, temperature, modules present, module status bits (2 per module)
    "time": "<II",   # timestamp, RTC state
}

MB = 1000 * 1000   # not 1024, intentional
GB = MB * 1000     # not 1024, intentional

SD_CARD_SIZES = {
    f"{i} Gb": i*GB for i in (1, 2, 4, 8, 16, 32, 64, 128, 256)
}
FLASH_MEMORY_SIZES = {
    f"{i} Mb": i*MB for i in (1, 2, 4, 8, 16, 32, 64, 128, 256)
}

###############################################################################


def calculate_data_rate(freqs, formats, *, details=False, frame_overhead=0):
    """Calculates the data rate of a log or telemetry stream.

    Args:
        freqs: dict mapping record types to the frequencies with which they are
            logged or sent. Frequencies are given in [Hz].
        formats: format of each record type in Python's struct syntax
        frame_overhead: per-frame overhead if applicable

    Returns:
        the data rate in bytes per second
    """
    total = 0.0
    for record_type, freq in freqs.items():
        record_format = formats.get(record_type)
        if not record_format:
            raise ValueError(f"record type {record_type!r} has no format")

        record_size = calcsize(record_format) + frame_overhead
        rate = freq * record_size

        if details:
            print(f"- {record_type}: {record_size} bytes @ {freq} Hz = {rate} bytes/sec")

        total += rate

    return total


def print_size_report(data_rate, sizes):
    for label, size in sorted(sizes.items(), key=lambda x: x[1]):
        duration = size / data_rate

        if duration < 3600:
            print(f"- {label}: {duration:.2f} sec")
        elif duration < 3 * 3600:
            print(f"- {label}: {duration/60:.2f} min")
        elif duration < 2 * 24 * 3600:
            print(f"- {label}: {duration/3600:.2f} hr")
        else:
            print(f"- {label}: {duration/24/3600:.2f} days")

tests = [
    ("Log records", LOG_RATES_HZ, LOG_RECORD_FORMATS, 0, True),
    ("Telemetry", TELEM_RATES_HZ, {**LOG_RECORD_FORMATS, **TELEM_RECORD_FORMATS}, TELEM_FRAME_OVERHEAD, False)
]


for title, rates, records, frame_overhead, estimate_duration in tests:
    print(f"# {title}")
    print()

    print("## Data rates")
    print()
    data_rate = calculate_data_rate(rates, records, frame_overhead=frame_overhead, details=True)
    print()
    print(f"Total data rate: {data_rate} bytes/sec")
    print(f"Total baud rate: {data_rate*8/1024} kbit/sec")
    print()

    if estimate_duration:
        print("## Estimated log durations")
        print()
        print("### SD card")
        print()
        print_size_report(data_rate, SD_CARD_SIZES)
        print()
        print("### Flash memory")
        print()
        print_size_report(data_rate, FLASH_MEMORY_SIZES)
        print()
        print()
