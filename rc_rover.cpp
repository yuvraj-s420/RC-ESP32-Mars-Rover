#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <DHT.h>
#include <ArduinoJson.h>
#include <ESP32Servo.h>


// Replace with your network credentials
#define WIFI_SSID "YOUR_SSID"
#define WIFI_PASSWORD "YOUR_PASSWORD"

// Pins for the DHT sensor, servo, motors, and ultrasonic sensor
const int DHTPIN = 15;

// Left motor pins
const int ENA_PIN = 33;
const int IN1_PIN = 25;
const int IN2_PIN = 26;

// Right motor pins
const int ENB_PIN = 12;
const int IN3_PIN = 27;
const int IN4_PIN = 14;

// Ultrasonic sensor pins
const int TRIG_PIN = 32;
const int ECHO_PIN = 35;    //input only pin, works in this case

// Servo pin
const int SERVO_PIN = 13;

Servo servo;

int interval = 1000;  // Interval (milliseconds) to send updates
unsigned long previousMillies = 0;  // Variable to store the last time the data was sent

String command = "stop"; // Initial command for the rover is to be stationary
float speed = 50;   // Speed of the rover initialized to 50%

const float cliffThreshold = 12.00;  // Threshold for the ultrasonic sensor to detect a cliff (12 cm)
bool isCliff = false;   // Flag to indicate if a cliff is detected

long duration; // Variable to store time taken to the pulse
float distance; // Variable to store distance calculated from the time taken by the ultrasonic pulse

// HTML, CSS, JavaScript code that is send to the webserver
String webpage = R"rawliteral(

<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>RC Rover Control</title>
    <link href="https://fonts.googleapis.com/css2?family=Orbitron&display=swap" rel="stylesheet">
    <style>
        body {
            font-family: 'Orbitron', sans-serif;
            text-align: center;
            background: linear-gradient(to bottom, #000814, #001a33);
            color: white;
            margin: 0;
            padding: 0;
        }
        h1 {
            color: #00aaff;
            text-shadow: 0px 0px 10px #00aaff;
        }
        .dpad-container {
            display: flex;
            justify-content: center;
            align-items: center;
            height: 40vh;
        }
        .dpad {
            display: grid;
            grid-template-areas:
                " .  up  . "
                "left middle right"
                " . down . ";
            gap: 10px;
        }
        .dpad button {
            padding: 30px;
            font-size: 20px;
            border: none;
            background-color: #002244;
            color: #00aaff;
            border-radius: 10px;
            cursor: pointer;
            box-shadow: 0 0 10px rgba(0, 170, 255, 0.5);
            transition: 0.2s;
            user-select: none;
        }
        .dpad button:active {
            background-color: #00aaff;
            color: black;
            transform: scale(0.9);
            box-shadow: 0 0 20px rgba(0, 170, 255, 1);
        }
        .up { grid-area: up; }
        .down { grid-area: down; }
        .left { grid-area: left; }
        .right { grid-area: right; }
        .middle { grid-area: middle; background: #004466; }

        .slider-container {
            width: 90%;
            max-width: 400px;
            margin: 20px auto;
            text-align: center;
            background-color: #002244;
            padding: 15px;
            border-radius: 10px;
            box-shadow: 0 0 15px rgba(0, 170, 255, 0.4);
        }
        .slider-container p {
            font-size: 18px;
            margin-bottom: 10px;
            color: #00aaff;
        }
        .slider {
            width: 100%;
            height: 10px;
            border-radius: 5px;
            background: linear-gradient(to right, #004488, #00aaff);
            transition: background 0.3s;
        }
        .slider::-webkit-slider-thumb {
            width: 30px;
            height: 30px;
            border-radius: 50%;
            background: white;
            cursor: pointer;
            box-shadow: 0 0 10px rgba(0, 170, 255, 0.8);
        }

        .data-container {
            margin-top: 20px;
            background: #002244;
            padding: 15px;
            border-radius: 10px;
            box-shadow: 0 0 15px rgba(0, 170, 255, 0.4);
            width: 90%;
            max-width: 400px;
            margin-left: auto;
            margin-right: auto;
        }
        .data-container h2 {
            color: #00aaff;
        }

        #status {
            color: #00ffff;
            font-size: 18px;
            margin-top: 20px;
            text-shadow: 0px 0px 10px #00ffff;
        }
    </style>
</head>
<body>

    <h1>Rover Control</h1>

    <div class="dpad-container">
        <div class="dpad">
            <button class="up" onmousedown="sendCommand('forward')" onmouseup="sendCommand('stop')" ontouchstart="sendCommand('forward')" ontouchend="sendCommand('stop')">&#9650;</button>
            <button class="down" onmousedown="sendCommand('backward')" onmouseup="sendCommand('stop')" ontouchstart="sendCommand('backward')" ontouchend="sendCommand('stop')">&#9660;</button>
            <button class="left" onmousedown="sendCommand('left')" onmouseup="sendCommand('stop')" ontouchstart="sendCommand('left')" ontouchend="sendCommand('stop')">&#9668;</button>
            <button class="right" onmousedown="sendCommand('right')" onmouseup="sendCommand('stop')" ontouchstart="sendCommand('right')" ontouchend="sendCommand('stop')">&#9658;</button>
            <button class="middle" onclick="sendCommand('scoop')">Scoop</button>
        </div>
    </div>

    <div class="slider-container">
        <p>Speed: <span id="value">50</span>%</p>
        <input type="range" min="1" max="100" value="50" class="slider" id="speedSlider">
    </div>

    <div class="data-container">
        <h2>Rover Data</h2>
        <p>Humidity: <span id="humidity">-</span>%</p>
        <p>Temperature: <span id="temperature">-</span>°C</p>
        <p>Distance: <span id='distance'>-</span> cm</p>

    </div>

    <p id="status">Status: <span id="statusText">Idle</span></p>

    <script>
        // Create a new WebSocket for real time data and changes
        var Socket;

        var slider = document.getElementById('speedSlider');
        var output = document.getElementById('value');
        output.innerHTML = slider.value;

        // Update the speed slider value when it is changed and send to the WebSocket
        slider.oninput = function() {
            output.innerHTML = this.value;
            var obj = {
                command: 'stop',  // Default command to stop when slider is adjusted
                speed: this.value
            };
            Socket.send(JSON.stringify(obj));
        }

        // Sending the direction to the ESP32 while the button is held
        function sendCommand(cmd) {
            console.log("Sending command: " + cmd + " with speed: " + slider.value);
            document.getElementById('statusText').innerHTML = cmd
            var obj = {
                command: cmd,
                speed: slider.value
            };
            Socket.send(JSON.stringify(obj));
        }

        // Function to initialize the WebSocket
        function init() {
            Socket = new WebSocket('ws://' + window.location.hostname + ':81/');    // Connect to the WebSocket server
            
            // Console feedback for debugging
            Socket.onopen = function() {
                console.log('WebSocket Connected!');
            };

            Socket.onerror = function(error) {
                console.log('WebSocket Error:', error);
            };
            
            Socket.onmessage = function(event) {
                console.log('Received Data:', event.data);
                processCommand(event);
            };
        }
        // Function to process the command received from the WebSocket (in this case, changing the inner html of the span elements to the temp and humidity data received)
        function processCommand(event) {
            // Parse the JSON object from the event and update the values on the HTML page
            var jsonobj = JSON.parse(event.data);
            document.getElementById('humidity').innerHTML = jsonobj.humidity;
            document.getElementById('temperature').innerHTML = jsonobj.temperature;
            document.getElementById('distance').innerHTML = jsonobj.distance;
            console.log(jsonobj.humidity);    // Log the data to the console for debugging
            console.log(jsonobj.temperature);
        }   
        // Call the init function when the page is loaded to start websocket
        window.onload = function(event) {
            init();
        }
    </script>

</body>
</html>

)rawliteral";

// Initialize the DHT sensor
DHT dht(DHTPIN, DHT11);

// Initialize the WebServer and WebSocket
WebServer server(80);  
WebSocketsServer webSocket = WebSocketsServer(81);  

// Initialize the JSON documents
JsonDocument doc_tx;    // JSON for transmitting data
JsonDocument doc_rx;    // JSON for receiving data

void executeCommand(String command) {
    // Moves the rover based on the command

    int motorSpeed = speed / 100 * 255;  // Convert speed to PWM value

    if (command == "forward") {
        // Move the rover forward

        // Left wheels move forward
        analogWrite(ENA_PIN, motorSpeed);
        digitalWrite(IN1_PIN, 1);
        digitalWrite(IN2_PIN, 0);

        // Right wheels move forward
        analogWrite(ENB_PIN, motorSpeed);
        digitalWrite(IN3_PIN, 1);
        digitalWrite(IN4_PIN, 0);
    }
    else if (command == "backward") {
        // Move the rover backward

        // Left wheels move backward
        analogWrite(ENA_PIN, motorSpeed);
        digitalWrite(IN1_PIN, 0);
        digitalWrite(IN2_PIN, 1);

        // Right wheels move backward
        analogWrite(ENB_PIN, motorSpeed);
        digitalWrite(IN3_PIN, 0);
        digitalWrite(IN4_PIN, 1);
    }
    else if (command == "left") {
        // Move the rover left

        // Left wheels stop
        analogWrite(ENA_PIN, 0);
        digitalWrite(IN1_PIN, 0);
        digitalWrite(IN2_PIN, 0);

        // Right wheels move forward
        analogWrite(ENB_PIN, motorSpeed);
        digitalWrite(IN3_PIN, 1);
        digitalWrite(IN4_PIN, 0);
    }
    else if (command == "right") {
        // Move the rover right

        // Left wheels move forward
        analogWrite(ENA_PIN, motorSpeed);
        digitalWrite(IN1_PIN, 1);
        digitalWrite(IN2_PIN, 0);

        // Right wheels stop
        analogWrite(ENB_PIN, 0);
        digitalWrite(IN3_PIN, 0);
        digitalWrite(IN4_PIN, 0);
    }
    else if (command == "scoop") {
        // Utilize scooper

        Serial.println("Scooping...");

        for (int i = 0; i < 180; i++){
            servo.write(i);
            delay(10);
        }

        // Stop after scooper has finished
        executeCommand("stop");
    }
    else if (command == "stop") {
        // Stop the rover

        // Stop left wheels
        analogWrite(ENA_PIN, 0);
        digitalWrite(IN1_PIN, 0);
        digitalWrite(IN2_PIN, 0);

        // Stop right wheels
        analogWrite(ENB_PIN, 0);
        digitalWrite(IN3_PIN, 0);
        digitalWrite(IN4_PIN, 0);
    }
    else {
        Serial.println("Invalid command");  // Should not appear
    }

}

void webSocketEvent(byte num, WStype_t type, uint8_t * payload, size_t length) {
    // num is client number, type is type of event (connect, disconnect, text message), payload is data received (array of unsigned int), length is number of bytes

    Serial.println("\nWebSocket Event Triggered");

    // Check the type of event
    if (type == WStype_CONNECTED) {
        Serial.println("Client connected");
    }
    else if (type == WStype_DISCONNECTED) {
        Serial.println("Client disconnected");
        executeCommand("stop");  // Stop the rover when client disconnects
    }
    else if (type == WStype_TEXT) {
        // Deserialize the JSON payload, displaying error message if there is one
        DeserializationError error = deserializeJson(doc_rx, payload);
        if (error) {
            Serial.print("deserializeJson() failed: ");
            return;
        }
        else {
            // Extract the command and speed from the JSON
            const char* temp = doc_rx["command"];
            command = String(temp);
            speed = (float) doc_rx["speed"];

            Serial.println("Received Command: " + command+ " with speed: " + String(speed));   

            // If there is a cliff, only allow backward movement
            if (isCliff) {
                if (command == "backward") {
                    executeCommand(command);
                }
                else {
                    executeCommand("stop");
                    Serial.println("Cliff detected, stopping rover");
                }
            }
            else {  // Otherwise execute the command
                executeCommand(command);
            }
        }
    }
}

void setup() {
    // put your setup code here, to run once:
    Serial.begin(115200);

    // Initialize the pins as outputs
    //pinMode(SERVO_PIN, OUTPUT);
    pinMode(DHTPIN, INPUT);
    pinMode(ENA_PIN, OUTPUT);
    pinMode(IN1_PIN, OUTPUT);
    pinMode(IN2_PIN, OUTPUT);
    pinMode(ENB_PIN, OUTPUT);
    pinMode(IN3_PIN, OUTPUT);
    pinMode(IN4_PIN, OUTPUT);
    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);
    
    servo.attach(SERVO_PIN);

    servo.write(0); // Initial position of the servo

    // Initialize the DHT sensor
    dht.begin();

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    // Wait for connection
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to WiFi with SSID: " + String(WIFI_SSID) + " and Password: " + String(WIFI_PASSWORD));
    }

    Serial.println("Connected to WiFi");
    Serial.println("IP Address: ");
    Serial.println(WiFi.localIP()); // Print the IP address

    // Set up the HTML webpage
    server.on("/", []() { server.send(200, "text/html", webpage); });

    // Start the server and websocket
    server.begin();
    Serial.println("HTTP Server started!");
    webSocket.begin();
    webSocket.onEvent(webSocketEvent);
    Serial.println("WebSocket Server started on port 81!");
}


void loop() {

    server.handleClient();
    webSocket.loop();

    //Read the distance from the ultrasonic sensor
    digitalWrite(TRIG_PIN, LOW);    // Set the trigger to low
    delayMicroseconds(2);           // wait for 2 ms to prevent collision in Serial monitor
    digitalWrite(TRIG_PIN, HIGH);      // turn on the Trigger to generate pulse
    delayMicroseconds(10);          // keep the trigger "ON" for 10 ms to generate
    digitalWrite(TRIG_PIN, LOW);    // Turn off the pulse trigger to stop the pulse

    duration = pulseIn(ECHO_PIN, HIGH);     // Read the time taken for the pulse to return
    distance = round(duration * 0.0344 / 2 * 100) / 100.0;       // Calculate the distance from the time taken and speed of sound rounded to 2 decimals
    
    //Serial.println("Distance: " + String(distance) + " cm");

    delay(50);   // Short delay to stabilize readings

    // Check if the distance is greater than the threshold (indicating a cliff)
    if (distance > cliffThreshold) {
        isCliff = true;
    }
    else {
        isCliff = false;
    }

    // Send the temperature and humidity data at regular intervals

    long currentMillis = millis();  // Keep track of time elapsed

    if (currentMillis - previousMillies >= interval) {

        String jsonString = "";
        JsonObject object = doc_tx.to<JsonObject>();

        // Read the temperature and humidity
        float temperature = dht.readTemperature();
        float humidity = dht.readHumidity();

        //Serial.println("Temperature: " +String(temperature) + " °C"+", Humidity: " +String(humidity) + " %");

        // Construct JSON document with values
        doc_tx["temperature"] = temperature;
        doc_tx["humidity"] = humidity;
        doc_tx["distance"] = distance;

        // Convert JSON object to a string by serializing, and broadcast to web socket
        serializeJson(doc_tx, jsonString);
        webSocket.broadcastTXT(jsonString);

        previousMillies = currentMillis;
    }
}
