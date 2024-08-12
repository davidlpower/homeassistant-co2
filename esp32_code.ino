#include <WiFi.h>
#include <Wire.h>
#include "HT_SSD1306Wire.h"
#include <MHZ19.h>
#include <ESPAsyncWebServer.h>

// Initialise the display
static SSD1306Wire display(0x3c, 500000, SDA_OLED, SCL_OLED, GEOMETRY_128_64, RST_OLED);

// UART settings for MH-Z19E (CO2 Sensor)
HardwareSerial mySerial(1); // UART1 for MH-Z19E
MHZ19 myMHZ19;
unsigned long getDataTimer = 0;

// WiFi settings
const char* ssid = "";
const char* password = "";

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

int co2 = 0;
bool homeAssistantRequest = false;

void waitForWiFiConnectOrReboot() {
  uint32_t notConnectedCounter = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    notConnectedCounter++;
    if (notConnectedCounter > 50) { // Reset board if not connected after 5s
      ESP.restart();
    }
  }

  // Display WiFi info on OLED (initially during setup)
  display.clear();
  display.setFont(ArialMT_Plain_10);
  display.drawString(0, 0, "WiFi Connected");
  display.drawString(0, 20, WiFi.localIP().toString());
  display.display();
  delay(2000); // Show the info for a bit
}

void setup() {
  VextON();
  delay(100);

  // Initialise the display
  display.init();
  display.setFont(ArialMT_Plain_10);

  // Initialise UART for the MH-Z19E sensor
  mySerial.begin(9600, SERIAL_8N1, 3, 1); // RX -> GPIO 3, TX -> GPIO 1
  myMHZ19.begin(mySerial);
  myMHZ19.autoCalibration();

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  waitForWiFiConnectOrReboot();

  // Serve CO2 data when requested
  server.on("/co2", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", String(co2));
    homeAssistantRequest = true;  // Indicate that HA requested data
  });

  server.begin();
}

void loop() {
  // Read CO2 sensor data every 2 seconds
  if (millis() - getDataTimer >= 2000) {
    getDataTimer = millis();
    co2 = myMHZ19.getCO2();

    // Display CO2 value and IP address on the screen
    display.clear();
    display.setFont(ArialMT_Plain_10);
    display.drawString(0, 0, "IP: " + WiFi.localIP().toString());  // Display IP address
    display.setFont(ArialMT_Plain_16);
    if (co2 > 0) {
      display.drawString(0, 20, "CO2: " + String(co2) + " ppm");
    } else {
      display.drawString(0, 20, "No Data");
    }

    // Display a small circle if HA requested data
    if (homeAssistantRequest) {
      display.fillCircle(120, 10, 5); // Draw a circle at position (120,10) with radius 5
      homeAssistantRequest = false;   // Reset the request flag
    }

    display.display();
  }
}

// Function to control the power of the external display
void VextON(void) {
  pinMode(Vext, OUTPUT);
  digitalWrite(Vext, LOW);
}

void VextOFF(void) {
  pinMode(Vext, OUTPUT);
  digitalWrite(Vext, HIGH);
}
