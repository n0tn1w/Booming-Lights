#include <WiFi.h>
#include "FFT.h"
#include <NeoPixelBus.h>
#include <NeoPixelSegmentBus.h>
#include <NeoPixelBrightnessBus.h>
#include <NeoPixelAnimator.h>
#include <stdlib.h>
#include <map>
#include <cstring>

#define NUM_LEDS 16
#define MIC_PIN 34 // Warning! WiFi uses one of the ADCs, so only some of the pins can be used for analogRead!
#define PIXEL_PIN 25

#define Cred     RgbColor(255, 0, 0)
#define Cblack   RgbColor(0,0,0)


NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod> strip(NUM_LEDS, PIXEL_PIN);
#define FFT_N 128 // Must be a power of 2

float fft_input[FFT_N];
float fft_output[FFT_N];

fft_config_t *real_fft_plan = fft_init(FFT_N, FFT_REAL, FFT_FORWARD, fft_input, fft_output);
float blendIndex;


const char* ssid = "Stanislav & Iva";
const char* password = "jjkofajj";
WiFiServer server(301);

String header;
unsigned long currentTime = millis();
unsigned long previousTime = 0;
const long timeoutTime = 2000;

enum State {
  ReactiveLights,
  StaticLights,
  Off
};

String stateToString(State e) {
  switch (e) {
    case ReactiveLights: return "Reactive Lights";
    case StaticLights: return "Static Lights";
    case Off: return "Off";
    default: return "Bad State";
  }
}

State state = ReactiveLights;

void stopLights() {
  for (int i = 0; i < NUM_LEDS; i ++) {
    strip.SetPixelColor(i, Cblack);
  }
  strip.Show();
}

void waitForWiFiRequests( void * parameter ) {

  for (;;) {
    delay(1);

    WiFiClient client = server.available();   // Listen for incoming clients
    if (client) {                             // If a new client connects,
      currentTime = millis();
      previousTime = currentTime;
      Serial.println("New Client.");          // print a message out in the serial port
      String currentLine = "";                // make a String to hold incoming data from the client
      while (client.connected() && currentTime - previousTime <= timeoutTime) {  // loop while the client's connected
        currentTime = millis();
        if (client.available()) {             // if there's bytes to read from the client,
          char c = client.read();             // read a byte, then
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
              if (header.indexOf("GET /dynamic/on") >= 0) {
                Serial.println("Dynamic Lights on");
                state = ReactiveLights;
              } else if (header.indexOf("GET /dynamic/off") >= 0) {
                Serial.println("Dynamic Lights  off");
                state = Off;
              }

              // Display the HTML web page
              client.println("<!DOCTYPE html><html>");
              client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
              client.println("<link rel=\"icon\" href=\"data:,\">");
              // CSS to style the on/off buttons
              // Feel free to change the background-color and font-size attributes to fit your preferences
              client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
              client.println(".button { background-color: #4CAF50; border: none; color: white; padding: 16px 40px;");
              client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
              client.println(".button2 {background-color: #555555;}</style></head>");

              // Web Page Heading
              client.println("<body><h1>Booming lights</h1>");

              // Display current state, and ON/OFF buttons for GPIO 26
              client.println("<p>Dynamic Lights - State " + stateToString(state) + "</p>");
              // If the output26State is off, it displays the ON button
              if (state == Off) {
                client.println("<p><a href=\"/dynamic/on\"><button class=\"button\">ON</button></a></p>");
              } else {
                client.println("<p><a href=\"/dynamic/off\"><button class=\"button button2\">OFF</button></a></p>");
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
}



void makeLigthsReactToMusic( void * parameter ) {
  for (;;) {
    if ( state != ReactiveLights) {
      stopLights();
      delay(100);
      continue;
    }

    int begin = millis();
    for (int k = 0 ; k < FFT_N ; k++) {
      real_fft_plan->input[k] = analogRead(MIC_PIN);
    }
    int end = millis();
    float total_time = float(end - begin) / 1000.0;

    fft_execute(real_fft_plan);

    for (int k = 0 ; k < real_fft_plan->size / 2 ; k += 2) {
      float len = sqrt(pow(real_fft_plan->output[2 * k], 2) + pow(real_fft_plan->output[2 * k + 1], 2));
      //float freq = float(k) / total_time;
      if (k > 0 && k < 32 && len > 600) {
        blendIndex = len / 50000.0;
      } else {
        blendIndex = 0.0;
      }
      strip.SetPixelColor(k / 2, RgbColor::LinearBlend(Cblack, Cred, blendIndex));
    }
    strip.Show();
  }
}

TaskHandle_t Task1, Task2;

void setup() {
  Serial.begin(115200);
  while (!Serial);

  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  server.begin();
  state = ReactiveLights;

  strip.Begin();
  strip.Show();
  /*Syntax for assigning task to a core:
    xTaskCreatePinnedToCore(
                   coreTask,   // Function to implement the task
                   "coreTask", // Name of the task
                   10000,      // Stack size in words
                   NULL,       // Task input parameter
                   0,          // Priority of the task
                   NULL,       // Task handle.
                   taskCore);  // Core where the task should run
  */
  xTaskCreatePinnedToCore(    waitForWiFiRequests,    "waitForWiFiRequests",    20000,      NULL,    1,    &Task1,    0);
  delay(500);  // needed to start-up task1
  xTaskCreatePinnedToCore(    makeLigthsReactToMusic,    "makeLigthsReactToMusic",    5000,    NULL,    1,    &Task2,    1);
}

void loop () {}
