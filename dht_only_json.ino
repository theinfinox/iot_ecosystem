#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DHT.h>
#include <time.h> // Include the time library

#define DHTPIN D3          // What digital pin the DHT11 is connected to
#define DHTTYPE DHT11      // Specify using DHT11
DHT dht(DHTPIN, DHTTYPE);

const char* ssid = "pi";
const char* password = "raspberry";
const char* ntpServer = "pool.ntp.org"; // NTP server

ESP8266WebServer server(81); // Set HTTP server on port 81

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

  server.on("/", HTTP_GET, handleRoot);
  server.begin();
  Serial.println("HTTP server started");

  dht.begin();
}

void loop() {
  server.handleClient();
}

void handleRoot() {
  float temp = dht.readTemperature(); // Read temperature as Celsius
  float hum = dht.readHumidity(); // Read humidity (percentage)

  time_t now = time(nullptr);
  struct tm *tm_struct = gmtime(&now);

  char utcString[25];
  strftime(utcString, sizeof(utcString), "%Y-%m-%d %H:%M:%S", tm_struct);
  
  if (isnan(temp) || isnan(hum)) {
    server.send(500, "application/json", "{\"error\":\"Failed to read from DHT sensor\"}");
  } else {
    char json[200];
    snprintf(json, sizeof(json), "{\"temperature\": %.2f, \"humidity\": %.2f, \"utc\": \"%s\"}", temp, hum, utcString);
    server.send(200, "application/json", json);
  }
}
