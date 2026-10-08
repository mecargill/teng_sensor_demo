STRUCTURE
-------------------------------
The top level folder contains python scripts and the requirements.txt containing the libraries they depend on. This is also where the folder adc_data should be. Udp reader is all that will be needed if doing it wirelessly - uart was for my initial testing before I got wifi to work.

env_det_demo (envelope detector demo) contains the ESP32 project, made using ESP IDF. Within that, main contains the source of the project. env_det_demo.c contains the main function. adc.c handles the adc initialization and sampling, adc_output handles the streaming of those samples to the laptop, either by uart (for initial wired prototyping) or udp (for the actual wireless demo). 

HOW TO USE
---------------------------------
*Note: this is how to build and use with esp-idf, using Arduino ide may be different.
You will need to download esp idf in order to build

To Flash (do once) from env_det_demo Run:
export.ps1
idf.py flash

To Use (do every time) (from top level folder):
Power on ESP32
connect to access point ESP32_ADC.
run udp_reader.py
press r to record, e to end recording.
