/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2016 by Daniel Eichhorn
 * Copyright (c) 2016 by Fabrice Weinberg
 * Copyright (c) 2017 by Andrea Bonomi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

// Include the correct display library
#include <Wire.h>          // I2C Display driver
#include "SSD1306.h"       // Display
#include <WiFi.h>          // WIFI
#include <WiFiUdp.h>       // WIFI UDP
#include "OLEDDisplayUi.h" // UI library
#include "images.h"        // Images
#include <time.h>          // Time
//#include <sys/time.h>

const char* ssid = "WIFI-network-name";
const char* password = "super-secret-password";

// Initialize the OLED display
// D5 -> SDA
// D4 -> SCL
SSD1306  display(0x3c, 5, 4);
OLEDDisplayUi ui ( &display );
#define screenW 128
#define screenH 64
#define centerX (screenW / 2)
#define centerY (((screenH - 16) / 2) + 16)

// Web server
WiFiServer httpServer(80);

// Network Time Protocol
#define NTP_PORT 123
#define NTP_PACKET_SIZE 48
WiFiUDP Udp;
char timeServer[] = "time.nist.gov"; // NTP server
byte packetBuffer[ NTP_PACKET_SIZE];
#define seventyYears 2208988800UL;
#define ntpLocalPort 2390

int remoteStatus = 1;
String ipAddress = "disconnected";

// utility function for digital clock display: prints leading 0
String twoDigits(int digits) {
  if (digits < 10) {
    String i = '0' + String(digits);
    return i;
  }
  else {
    return String(digits);
  }
}


// Convert IPAddress to String
String IpAddress2String(const IPAddress& ipAddress)
{
  return String(ipAddress[0]) + String(".") + \
         String(ipAddress[1]) + String(".") + \
         String(ipAddress[2]) + String(".") + \
         String(ipAddress[3])  ;
}

// Return the UTC time of the day ad HH:MM:SS
String getTimeString() {
  timeval t;
  gettimeofday(&t, NULL);
  int second = t.tv_sec % 60;
  int minute = (t.tv_sec % 3600) / 60;
  int hour = (t.tv_sec % 86400L) / 3600;
  return twoDigits(hour) + ":" + twoDigits(minute) + ":" + twoDigits(second);
}


// Draw the first frame
void displayFrame1(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  String tmp = String(touchRead(T4)); // touch sensor
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_10);
  display->drawString(centerX + x , centerY + y - 16, "Touch sensor");
  display->setFont(ArialMT_Plain_24);
  display->drawString(centerX + x , centerY + y, tmp);
}


// Display the second frame
void displayFrame2(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  // draw an xbm image
  if (remoteStatus) {
    display->drawXbm(x + 34, y + 14, WiFi_Logo_width, WiFi_Logo_height, WiFi_Logo_bits);
  }

  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_10);
  display->drawString(centerX + x , centerY + y + 10, ipAddress);
}


// Draw the third frame
void displayFrame3(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_10);
  display->drawString(centerX + x , centerY + y - 16, "UTC Time");
  display->setFont(ArialMT_Plain_24);
  display->drawString(centerX + x , centerY + y, getTimeString());
}


// Display frames array
FrameCallback frames[] = { displayFrame1, displayFrame2, displayFrame3 };
int frameCount = 3;


// Send an NTP request to the time server
void sendNTPRequest() {
  // Prepare NTP request
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  packetBuffer[0] = 0b11100011;
  packetBuffer[1] = 0;
  packetBuffer[2] = 6;
  packetBuffer[3] = 0xEC;
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;
  // Send UDP package requesting timestamp
  Udp.beginPacket(timeServer, NTP_PORT);
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}


void WiFiEvent(WiFiEvent_t event)
{
  switch (event) {
    case SYSTEM_EVENT_STA_GOT_IP:
      Serial.println("WiFi connected");
      Serial.println("IP address: ");
      Serial.println(WiFi.localIP());
      ipAddress = IpAddress2String(WiFi.localIP());
      Udp.begin(ntpLocalPort);
      sendNTPRequest(); // send an NTP request
      break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
      Serial.println("WiFi lost connection");
      ipAddress = "disconnected";
      break;
  }
}


void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\nConnecting");

  // UI Setup
  ui.setTargetFPS(30); // fps
  ui.setActiveSymbol(activeSymbol);
  ui.setInactiveSymbol(inactiveSymbol);
  ui.setIndicatorPosition(TOP);
  ui.setIndicatorDirection(LEFT_RIGHT);
  ui.setFrameAnimation(SLIDE_LEFT);
  ui.setFrames(frames, frameCount);
  ui.init();
  display.flipScreenVertically();

  // Wifi Setup
  WiFi.disconnect(true);
  delay(1000);
  WiFi.onEvent(WiFiEvent);
  WiFi.begin(ssid, password);
  Serial.println();
  Serial.println();
  Serial.println("Wait for WiFi... ");

  // Start web server
  httpServer.begin();
}


void serveWebPages() {
  WiFiClient client = httpServer.available(); // check incoming clients

  if (client) {
    String currentLine = "";
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        if (c == '\n') {
          if (currentLine.length() == 0) {
            // HTTP headers
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println();

            // Page content
            client.println("<head><meta http-equiv=\"refresh\" content=\"5; url=/\"></head>");
            client.println("<a href=\"/on\">Show logo</a><br/>");
            client.println("<a href=\"/off\">Hide logo</a><br/>");
            client.println("<a href=\"/gettime\">Update time from NTP server</a><br/>");
            client.println("Touch sensor: ");
            client.println(String(touchRead(T4)));
            client.println("<br/>UTC time: ");
            client.println(getTimeString());
            client.println();
            break;
          } else {
            currentLine = "";
          }
        } else if (c != '\r') {
          currentLine += c;
        }

        // Actions
        if (currentLine.endsWith("GET /on")) {
          remoteStatus = 1;
        } else if (currentLine.endsWith("GET /off")) {
          remoteStatus = 0;
        } else if (currentLine.endsWith("GET /gettime")) {
          sendNTPRequest();
        }
      }
    }
    client.stop();
  }
}

// Get the time from the NTP server (parse NTP Packet)
void parseNTP() {
  if (Udp.parsePacket()) {
    Udp.read(packetBuffer, NTP_PACKET_SIZE);
    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
    unsigned long secsSince1900 = highWord << 16 | lowWord; // seconds since 1900-01-01
    unsigned long epoch = secsSince1900 - seventyYears;
    // Set RTC time
    timeval t;
    t.tv_sec = epoch;
    t.tv_usec = NULL;
    settimeofday(&t, NULL);
  }
}

// Main loop
void loop() {
  int remainingTimeBudget = ui.update();
  parseNTP();
  serveWebPages();
  if (remainingTimeBudget > 0) {
    delay(remainingTimeBudget);
  }
}

