Wiring:
IR Sensor VCC → ESP32 3.3V
IR Sensor GND → ESP32 GND
IR Sensor OUT → ESP32 GPIO 5

Buzzer (+) → ESP32 GPIO 2
Buzzer (-) → ESP32 GND

This code buzzes whenever any object is detected. After 10 sensing it gives a long beep for 5s and resets the counter and restrats the loop again
