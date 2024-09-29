This folder contains files hand-picked from a project generated from
STM32CubeMX to support a USB CDC device.

usbd_cdc_if.h and usbd_cdc_if.c were modified based on
https://github.com/philrawlings/bluepill-usb-cdc-test
to support a circular buffer via which we can communicate with the main
application.
