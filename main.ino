#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>
#include <time.h> // Include the time library

const char* ssid = "pi";  // Enter SSID here
const char* password = "raspberry";  //Enter Password here
const char* ntpServer = "pool.ntp.org"; // NTP server

ESP8266WebServer server(80);

uint8_t LEDpin = 2;
uint8_t D5pin = 14; // GPIO14 (D5)
const uint8_t D6pin = 12; // GPIO12 (D6)
bool LEDstatus = LOW;
bool D5status = LOW;
bool D6status = LOW;

#define DHTPIN D3           // What digital pin the DHT11 is connected to
#define DHTTYPE DHT11       // Specify using DHT11
#define airquality_PIN A0   // Analog pin for airquality sensor
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for SSD1306 display connected using I2C
#define OLED_RESET     -1 // Reset pin
#define SCREEN_ADDRESS 0x3C
DHT dht(DHTPIN, DHTTYPE);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
String message; // Global variable to store the received message

void setup() {
  Serial.begin(9600);
  delay(100);
  pinMode(LEDpin, OUTPUT);
  pinMode(D5pin, OUTPUT);  // Initialize D5 as an output
  pinMode(D6pin, OUTPUT);  // Initialize D6 as an output
  // Initialize the OLED display
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  display.display();
  delay(2000);  // Pause for 2 seconds
  
  // Clear the display buffer
  display.clearDisplay();
  display.setTextSize(1);      
  display.setTextColor(SSD1306_WHITE);  
  display.setCursor(0,0);     
  display.println("Connecting to WiFi...");
  display.display();

  // Connect to WiFi
  Serial.println("Connecting to " + String(ssid));
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  
  Serial.println("");
  Serial.println("WiFi connected..!");
  Serial.print("Got IP: ");  
  Serial.println(WiFi.localIP());

  configTime(0, 0, ntpServer); // Configure the NTP server

  pinMode(airquality_PIN, INPUT); // Set airquality pin as input

  // Setup HTTP server endpoints
  server.on("/", handle_OnConnect);
  server.on("/data", HTTP_GET, handleData); // Data endpoint
  server.on("/ledon", handle_ledon);
  server.on("/ledoff", handle_ledoff);
  server.on("/handle_msg",handle_msg);
  server.on("/D5on", handle_D5On);
  server.on("/D5off", handle_D5Off);
  server.on("/D6on", handle_D6On);
  server.on("/D6off", handle_D6Off);
  server.onNotFound(handle_NotFound);

  server.begin();
  Serial.println("HTTP server started");
  dht.begin();
}

void loop() {
  server.handleClient();
  
  // Control LED based on LEDstatus
digitalWrite(LEDpin, LEDstatus ? HIGH : LOW);
digitalWrite(D5pin, D5status ? HIGH : LOW);
digitalWrite(D6pin, D6status ? HIGH : LOW);



  // Update OLED display with serial output
  updateDisplay();
}

void handle_OnConnect() {
  LEDstatus = HIGH;
  server.send(200, "text/html", SendHTML(false)); 
}

void handle_ledon() {
  LEDstatus = HIGH;
  server.send(200, "text/html", SendHTML(true)); 
}

void handle_ledoff() {
  LEDstatus = LOW;
  server.send(200, "text/html", SendHTML(false)); 
}





void handle_D5On() {
  D5status = LOW;
  server.send(200, "text/html", SendHTML(true)); 
}

void handle_D5Off() {
  D5status = HIGH;
  server.send(200, "text/html", SendHTML(false)); 
}

void handle_D6On() {
  D6status = LOW;
  server.send(200, "text/html", SendHTML(true)); 
}

void handle_D6Off() {
 D6status = HIGH;
  server.send(200, "text/html", SendHTML(false)); 
}





void handle_msg() {
  if (server.args() > 0) { // Check if there are any arguments in the request
    for (uint8_t i = 0; i < server.args(); i++) { // Iterate through each argument
      if (server.argName(i) == "message") { // Check if the argument is named "message"
        message = server.arg(i); // Store the value of the "message" argument
        break; // Exit the loop after finding the "message" argument
      }
    }
  }
  
  // Process the received message
  // Here you can perform any actions you want with the received message
  // For this example, let's just print the message to the Serial monitor
  Serial.print("Received message: ");
  Serial.println(message);
  
  // Update the OLED display with the latest message
  updateDisplay();
  
  // You can add further processing logic here based on the received message
  
  // Send a response back to the client
  server.send(200, "text/plain", "Message received: " + message);
}

void handleData() {
  float temp = dht.readTemperature(); // Read temperature as Celsius
  float hum = dht.readHumidity(); // Read humidity (percentage)
  int airqualityValue = analogRead(airquality_PIN); // Read air quality level from the analog pin
  float airqualityPercentage = ((1023 - airqualityValue) / 1023.0) * 100.0; // Calculate air quality percentage inversely

  time_t now = time(nullptr);
  struct tm *tm_struct = gmtime(&now);
  char utcString[25];
  strftime(utcString, sizeof(utcString), "%Y-%m-%d %H:%M:%S", tm_struct);
  
  if (isnan(temp) || isnan(hum)) {
    server.send(500, "application/json", "{\"error\":\"Failed to read from sensors\"}");
  } else {
    char json[300];
    snprintf(json, sizeof(json), "{\"temperature\": %.2f, \"humidity\": %.2f, \"airquality\": %.2f, \"utc\": \"%s\"}", temp, hum, airqualityPercentage, utcString);
    server.send(200, "application/json", json);
  }
}


void handle_NotFound(){
  server.send(404, "text/plain", "Not found");
}

String SendHTML(uint8_t led) {
  String ptr = "<!DOCTYPE html>\n";
  ptr +="<html>\n";
  ptr +="<head>\n";
  ptr +="<title>LED Control</title>\n";
  ptr +="</head>\n";
  ptr +="<body>\n";
  ptr +="<h1>LED</h1>\n";
  ptr +="<p>Click to switch LED on and off.</p>\n";

  // Control for the main LED
  ptr +="<form method=\"get\">\n";
  if(led)
    ptr +="<input type=\"button\" value=\"LED ON\" onclick=\"window.location.href='/ledon'\">\n";
  else
    ptr +="<input type=\"button\" value=\"LED OFF\" onclick=\"window.location.href='/ledoff'\">\n";
  ptr +="</form>\n";

  // Message form
  ptr += "<form method=\"get\" action=\"/handle_msg\">\n";
  ptr += "<input type=\"text\" name=\"message\">\n";
  ptr += "<input type=\"submit\" value=\"Send Message\">\n";
  ptr += "</form>\n";

  // Control for D5
  ptr +="<form method=\"get\">\n";
  if(led)
    ptr +="<input type=\"button\" value=\"LED ON\" onclick=\"window.location.href='/D5off'\">\n";
  else
    ptr +="<input type=\"button\" value=\"LED OFF\" onclick=\"window.location.href='/D5on'\">\n";
  ptr +="</form>\n";

  // Control for D6
  ptr +="<form method=\"get\">\n";
  if(led)
    ptr +="<input type=\"button\" value=\"LED ON\" onclick=\"window.location.href='/D6off'\">\n";
  else
    ptr +="<input type=\"button\" value=\"LED OFF\" onclick=\"window.location.href='/D6on'\">\n";
  ptr +="</form>\n";

  ptr +="<p><a href=\"/temp\">Get Temperature</a></p>\n";
  ptr +="<p><a href=\"/humidity\">Get Humidity</a></p>\n";
  ptr +="</body>\n";
  ptr +="</html>\n";
  return ptr;
}


void updateDisplay() {
  // Clear the display buffer
  display.clearDisplay();
  display.setTextSize(1);      
  display.setTextColor(SSD1306_WHITE);  
  display.setCursor(0,0);     

  // Print serial outputs
  display.println("InBuiltLED : " + String(LEDstatus ? "OFF" : "ON"));
   display.println("Switch one : " + String(D5status ? "OFF" : "ON"));
  display.println("Switch two : " + String(D6status ? "OFF" : "ON"));
  display.println("IP Address: " + WiFi.localIP().toString());
  display.println("Received message:"); // Display a label
  display.println(message); 
  // Display buffer on OLED
  display.display();
}
