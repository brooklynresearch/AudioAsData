Vibrotextiles - js_sound
========================
This folder is for quick testing of the APSK function with the initial tests BKR did for the NRF52832

index
--------
Original javascript app to send data through APSK moduation.  Does not contain any error correction/checksum.

onthefly
----------
This folder contains a simple web based tester for APSK  Each data frame has 5 bytes, 4 of them are data, 1 of them is CRC checksum. At 1200 bps.  The carrier frequency is 1.2kHz to 2.2kHz.