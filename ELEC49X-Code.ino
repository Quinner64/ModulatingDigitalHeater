#include <ESP8266WiFi.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// Wi-Fi settings
const char* ssid = "yourSSID";
const char* password = "yourPASSWORD";

// Temperature sensor settings
#define ONE_WIRE_BUS 2 // Data wire for the sensors connected to pin 2
#define NUM_SENSORS 3  // Number of temperature sensors

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
float temperatures[NUM_SENSORS];

WiFiServer server(80);

// Desired temperature (setpoint) and hysteresis
float setpoint = 25.0;
float hysteresis = 0.5;
bool heatingElementOn = false;

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);

  // Start the Wi-Fi and server
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  server.begin();
  Serial.println("Server started");
  Serial.println(WiFi.localIP());

  // Start the temperature sensors
  sensors.begin();
}

void loop() {
  readAndAverageTemperatures();
  controlHeatingElement();

  // Check for a client and serve the web page
  WiFiClient client = server.available();
  if (client) {
    serveWebPage(client);
  }
}

void readAndAverageTemperatures() {
  sensors.requestTemperatures();
  float totalTemp = 0;
  for (int i = 0; i < NUM_SENSORS; i++) {
    temperatures[i] = sensors.getTempCByIndex(i);
    totalTemp += temperatures[i];
  }
  float averageTemp = totalTemp / NUM_SENSORS;

  // Control logic
  if (averageTemp < setpoint - hysteresis && !heatingElementOn) {
    heatingElementOn = true;
  } else if (averageTemp > setpoint + hysteresis && heatingElementOn) {
    heatingElementOn = false;
  }
}

void controlHeatingElement() {
  digitalWrite(LED_BUILTIN, heatingElementOn ? HIGH : LOW);
}

void serveWebPage(WiFiClient client) {
  // Read the HTTP request
  String request = client.readStringUntil('\r');
  client.flush();

  // Handle setpoint change
  if (request.indexOf("/SET=") != -1) {
    int startPos = request.indexOf("/SET=") + 5;
    int endPos = request.indexOf(" ", startPos);
    setpoint = request.substring(startPos, endPos).toFloat();
  }

  // Serve the HTML web page
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println("");
  client.println("<!DOCTYPE HTML><html><head><title>Temperature Control</title></head><body>");
  client.println("<h1>Room Temperature: " + String(getAverageTemperature(), 2) + " &#8451;</h1>");
  client.println("<h2>Setpoint: " + String(setpoint, 2) + " &#8451;</h2>");
  client.println("<form action=\"/SET\" method=\"get\">Set Temperature: <input type=\"number\" name=\"value\"><input type=\"submit\" value=\"Set\"></form>");
  client.println("</body></html>");

  delay(100);
  client.stop();
}

float getAverageTemperature() {
  float totalTemp = 0;
  for (int i = 0; i < NUM_SENSORS; i++) {
    totalTemp += temperatures[i];
  }
  return totalTemp / NUM_SENSORS;
}
