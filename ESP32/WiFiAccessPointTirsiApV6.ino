#include <dummy.h>
/*
  WifiAccessPointTirsiAppVx.ino creates a WiFi access point and provides a web server on it.
  Steps:
  1. Connect to the access point "TirsiRotorAp"
  2. Point your web browser to http://192.168.4.1
  Created for esp32 on 30/08, 2024
  --pin servo : gpio 1717
  --led red on  gpio21  (and optional Relais "on")
  --led green on gpio2  (and optional Relais "on")
  Tirsi board V1.1 provides place for either ESP32 devcit C 2x19 pin or ESP32-S3-devkit C 2x22 pin 
  for ESP32-S3 devkit with RGB led, you need in arduino ide /tools/usb cdc on boot/enabled
*/

#include <NetworkClient.h>
#include <WiFi.h>
#include <ESP32Servo.h>

Servo servo;

//ssid and password (no password)
const char* ssid = "TirsiRotorAp";
const char* password = "";

//NetworkServer server;
WiFiServer server(80);

#define RGB_BRIGHTNESS 32 //Changse RBB led white brightness on new devkit (max 255)
//variables
String header;
String tirsiCode;
int tirsiCodeStartIndex;
String outputRedState = "off";
String outputGreenState = "off";
String servoState="45";

//GPIO pins
const int servoPin = 17;
const int outputRed = 21;
const int outputGreen = 2;

void setup() {
  Serial.begin(115200);
  // Initialize the output variables as outputs
  pinMode(outputRed, OUTPUT);
  pinMode(outputGreen, OUTPUT);
  // Set outputs to LOW
  digitalWrite(outputRed, LOW);
  digitalWrite(outputGreen, LOW);
  // Connect to Wi-Fi network with SSID and password
  Serial.print("Setting AP (Access Point)…");
  WiFi.softAP(ssid, password);

  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);
  server.begin(); //start wifi server

  //setup servo
  ESP32PWM::allocateTimer(0);
	ESP32PWM::allocateTimer(18);
	servo.setPeriodHertz(50);    // standard 50 hz servo
  servo.attach(servoPin, 500, 2400);
	//if sweep is not accurate, try different min/max settings
  //for Futaba S3010, I use 500,2400 at this moment
}

void loop() {
  WiFiClient client = server.available();   // Listen for incoming clients
  if (client) {                             // If a new client connects,
    Serial.println("New Client.");          // print message out the serial monitor
    String currentLine = "";                // currentline holds incoming data from the client
    while (client.connected()) {            // loop as long as connected
      if (client.available()) {             // if there is something to read
        char c = client.read();             // read it
        Serial.write(c);                    // print it out the serial monitor
        header += c;
        if (c == '\n') {                    // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();

            // turns the GPIOs on and off
            if (header.indexOf("GET /code") >= 0) {
              Serial.println("got Tirsi code");
              int rbright;
              int gbright;
              int bbring;
              tirsiCodeStartIndex=header.indexOf("code");
              tirsiCode=header.substring(tirsiCodeStartIndex+5,tirsiCodeStartIndex+11);
              Serial.println("got a codesubstring:"+tirsiCode);
              char redString=tirsiCode.charAt(0);
              
              if (redString=='0'){
                outputRedState = "off";
                digitalWrite(outputRed, LOW);
                rbright=0;
              }else{
                outputRedState = "on";
                digitalWrite(outputRed, HIGH);
                rbright=RGB_BRIGHTNESS;
              }
              if (tirsiCode.charAt(1)=='0'){
                outputGreenState = "off";
                digitalWrite(outputGreen, LOW);
                gbright=0;
              }else{
                outputGreenState = "on";
                digitalWrite(outputGreen, HIGH);
                gbright=RGB_BRIGHTNESS;
              }
              
              //neopixel
              #ifdef RGB_BUILTIN
              neopixelWrite(RGB_BUILTIN, rbright, gbright, 0);  // Red on
              #endif

              String servoString=tirsiCode.substring(2,5);
              Serial.println("servovaluz:"+servoString);
              int servoValue=servoString.toInt();
              servo.write(servoValue);
              
            }else if (header.indexOf("GET /red/on") >= 0) {
              Serial.println("RED on");
              outputRedState = "on";
              digitalWrite(outputRed, HIGH);
              
              #ifdef RGB_BUILTIN
              neopixelWrite(RGB_BUILTIN, RGB_BRIGHTNESS, 0, 0);  // Red on
              #endif
            }else if (header.indexOf("GET /red/off") >= 0) {
              Serial.println("RED off");
              outputRedState = "off";
              digitalWrite(outputRed, LOW);

              #ifdef RGB_BUILTIN
              neopixelWrite(RGB_BUILTIN, 0, 0, 0);  // Red off
              #endif
            }else if (header.indexOf("GET /green/on") >= 0) {
              Serial.println("Green on");
              outputGreenState = "on";
              digitalWrite(outputGreen, HIGH);

              #ifdef RGB_BUILTIN
              neopixelWrite(RGB_BUILTIN, 0, RGB_BRIGHTNESS, 0);  // Red on
              #endif
            }else if (header.indexOf("GET /green/off") >= 0) {
              Serial.println("Green off");
              outputGreenState = "off";
              digitalWrite(outputGreen, LOW);

              #ifdef RGB_BUILTIN
              neopixelWrite(RGB_BUILTIN, 0, 0, 0);  // Red
              #endif
            }else if (header.indexOf("GET /servo/45") >= 0) {
              servoState = "45";
              Serial.println("Servo 45");
              servo.write(45);
            }else if (header.indexOf("GET /servo/135") >= 0) {
              servoState = "135";
              Serial.println("Servo 135");
              servo.write(135);
            }
            
            // Display the HTML web page
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            // CSS to style the on/off buttons 
            // Feel free to change the background-color and font-size attributes to fit your preferences
            client.println("<style>html { font-family: Helvetica; font-size: 16px; display: inline-block; margin: 0px auto; text-align: center;}");
            client.println(".button { background-color: #4CAF50; border: none; color: white; padding: 16px 30px;");
            client.println("text-decoration: none; font-size: 12px; margin: 2px; cursor: pointer;}");
            client.println(".button2 {background-color: #555555;}");
            client.println(".button3 {background-color: #FF0000;}</style></head>");
            
            // Web Page Heading
            client.println("<body><h1>Tirsi Rotor V6</h1>");
            //client.println("<body><h2>IO:Red=21,Green=2,Servo=17</h2>");

            // Display current RED state, and ON/OFF buttons for GPIO red  
            client.println("<p>GPIO Red(21) - State " + outputGreenState + "</p>");
            // If the outputRedState is off, it displays the ON button       
            if (outputRedState=="off") {
              client.println("<p><a href=\"/red/on\"><button class=\"button\">ON</button></a></p>");
            } else {
              client.println("<p><a href=\"/red/off\"><button class=\"button button2\">OFF</button></a></p>");
            } 
               
            // Display current GREEN state, and ON/OFF buttons for GPIO green  
            client.println("<p>GPIO Green(02) - State " + outputGreenState + "</p>");
            // If the output27State is off, it displays the ON button       
            if (outputGreenState=="off") {
              client.println("<p><a href=\"/green/on\"><button class=\"button\">ON</button></a></p>");
            } else {
              client.println("<p><a href=\"/green/off\"><button class=\"button button2\">OFF</button></a></p>");
            }

            // Display servo 45 old:current GREEN state, and ON/OFF buttons for GPIO green  
            client.println("<p>GPIO Servo (17) - State " + servoState + "</p>");
            //       
            if (servoState=="45") {
              client.println("<p><a href=\"/servo/135\"><button class=\"button\">135</button></a></p>");
            } else {
              client.println("<p><a href=\"/servo/45\"><button class=\"button button3\">45</button></a></p>");
            }

            client.println("</body></html>");
            
            // The HTTP response ends with another blank line
            client.println();
            // Break out of the while loop
            break;
          } else { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    // Clear the header variable
    header = "";
    // Close the connection
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }
}
