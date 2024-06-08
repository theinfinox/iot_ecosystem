#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DHT.h>
#include <time.h> // Include the time library

#define DHTPIN D3           // What digital pin the DHT11 is connected to
#define DHTTYPE DHT11       // Specify using DHT11
#define airquality_PIN A0   // Analog pin for airquality sensor

DHT dht(DHTPIN, DHTTYPE);
ESP8266WebServer server(80); // Set HTTP server on port 80

const char* ssid = "pi";
const char* password = "raspberry";
const char* ntpServer = "pool.ntp.org"; // NTP server

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi. IP address: ");
  Serial.println(WiFi.localIP());

  configTime(0, 0, ntpServer); // Configure the NTP server

  pinMode(airquality_PIN, INPUT); // Set airquality pin as input
  server.on("/", HTTP_GET, handleRoot); // Root endpoint
  server.on("/data", HTTP_GET, handleData); // Data endpoint
  server.begin();
  Serial.println("HTTP server started");

  dht.begin();
}

void loop() {
  server.handleClient();
}

void handleRoot() {
  server.send(200, "text/plain", "Welcome to the ESP8266 Air Quality Monitor");
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
