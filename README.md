# American Sign language
 american sign language recognition with esp32-cam:

 I implement a tensorflow lite (tflite) model on the ESP32-cam for American Sign language. The model is a stand-alone model and requires no network connection to run. The projects uses a pcb from pcbway to connect the esp32-can to a tft lcd screen, thus eliminating much of the messy wiring.

 The youtube video giving instructions for building this project is here: <br>
 https://youtu.be/9ikQ7QFDbh4

 [![American Sign Language](https://i9.ytimg.com/vi_webp/9ikQ7QFDbh4/mqdefault.webp?v=64ccb685&sqp=CNzrsqYG&rs=AOn4CLAw7yBEJtLvFue2c2x371T_yBB77g)](https://youtu.be/9ikQ7QFDbh)

# Summary of direcotries in this repo.

### american_sign_language_esp32_v2
This directory contains the Arduino sketch (and supporting files) for doing american sign language recognition on the esp32-cam.

### person_detection_tft_update_v4
This directory contains my updated version of the person detection sketch for ouputing the camera image on a tft, using the TFT_eSPI library.

### pcb_design
This directory contains my pcb design gerber files and fritzing file.

### User_Setup.h is the update file for the TFT_eSPI library.

# Links
Kaggle dataset: <br>
https://www.kaggle.com/datasets/datamunge/sign-language-mnist <br>

pcbway: <br>
https://pcbway.com/g/QkVak7 <br>

person detection training instructions: <br>
https://github.com/tensorflow/tflite-micro/blob/main/tensorflow/lite/micro/examples/person_detection/training_a_model.md <br>

tflite instructions for microcontrollers: <br>
https://www.tensorflow.org/lite/microcontrollers/get_started_low_level <br>

Converting tensorflow to tflite interesting video: <br>
[https://www.youtube.com/watch?v=bKLL0tAj3GE&t](https://youtu.be/bKLL0tAj3GE) <br>

# Notes

I'm using board version 2.02 of the esp32. The higher versions had a bug with the TFT_eSPI library for outputting  jpeg files. Although, I didn't end up using the jpeg files. <br>

The pin connections for the esp32 and tft are below. Note: if you are using the TFT_eSPI library, you need to define the pin connections in the User_Setup.h file. <br>
```c
#define TFT_MISO 13
#define TFT_MOSI 12
#define TFT_SCLK 14
#define TFT_CS   15  // Chip select control pin
#define TFT_DC    2  // Data Command control pin
#define TFT_RST   16  // Reset pin (could connect to RST pin)
\\LED pin connected high
```
 
