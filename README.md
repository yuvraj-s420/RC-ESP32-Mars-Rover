# RC-ESP32-Mars-Rover
ESP32 RC rover that is controlled through a webserver with buttons, speed slider, and temperature/ humidity data receiving, updated through the use of Websockets. The rover is automatically stopped from moving (except backwards) when ultrasonic sensor reading is above a threshold, which indicates a cliff. Rover also has a scooper for soil samples.
