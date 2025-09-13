# ESP32 RC Rover with Web Control

This project is a Wi-Fi controlled RC Rover built using an ESP32 microcontroller. The rover can be driven through a web interface that runs in any browser on the same network. It also streams real-time sensor data (temperature, humidity, distance) and features a servo-operated scoop for collecting samples.

## Picture of the Rover
![alt text](https://github.com/yuvraj-s420/RC-ESP32-Mars-Rover/blob/main/Rover_Image.jpg "Logo Title Text 1")

## Picture of website
![alt text](https://github.com/yuvraj-s420/RC-ESP32-Mars-Rover/blob/main/Website_Example.jpg "Logo Title Text 1")


## Features

Web-based control panel (works on phone, tablet, or PC)

D-pad interface for forward, backward, left, right movement

Real-time speed control using a slider

Scoop function using a servo motor

Live data feed from sensors:

Temperature & Humidity (DHT11)

Distance from ultrasonic sensor (for obstacle/cliff detection)

Cliff detection safety: Rover automatically stops if a drop-off is detected

DC motor control with PWM

JSON-based communication via WebSockets for real-time updates

## Technologies Used

ESP32 (Wi-Fi + WebSockets server)

C++ / Arduino framework

Web Interface (HTML/CSS/JS) served from ESP32

WebSockets for two-way communication between browser and rover

DHT11 Sensor for temperature & humidity

HC-SR04 Ultrasonic Sensor for distance/cliff detection

Servo motor for scooping (mg90s)

4x DC Motors (for rover movement)

L298N (or compatible) motor driver

Power supply (battery pack)
