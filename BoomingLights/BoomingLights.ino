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

#define Cblack   RgbColor(0,0,0)

#define Cred     RgbColor(255, 0, 0)
#define Corange  RgbColor(255, 96, 0)
#define Cwhite   RgbColor(128, 128, 128)
#define Cblue    RgbColor(0, 64, 255)
#define Cgreen   RgbColor(0, 192, 64)

RgbColor currentColor = Cred;

NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod> strip(NUM_LEDS, PIXEL_PIN);
#define FFT_N 128 // Must be a power of 2

float fft_input[FFT_N];
float fft_output[FFT_N];

fft_config_t *real_fft_plan = fft_init(FFT_N, FFT_REAL, FFT_FORWARD, fft_input, fft_output);
float blendIndex;

unsigned long timerCount = 0;

const char* ssid = "Stanislav & Iva";
const char* password = "jjkofajj";

//const char* ssid = "Plamen_2";
//const char* password = "0898630664";
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

String colorToString(RgbColor e) {
  if (e == Cred) {
    return "Red";
  } else if (e == Corange) {
    return "Orange";
  } else if (e == Cwhite) {
    return "White";
  } else if (e == Cblue) {
    return "Blue";
  } else if (e == Cgreen) {
    return "Green";
  } else {
    return "No color";
  }
}

int getHexVal(char c) {
  if ('0' <= c and c <= '9') return c - '0';
  return 'c' - 'a' + 10;
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

              if (header.indexOf("GET /dynamic/on") >= 0) {
                Serial.println("Dynamic Lights on");
                state = ReactiveLights;
              } else if (header.indexOf("GET /dynamic/off") >= 0) {
                state = Off;
              } else if (header.indexOf("GET /static/on") >= 0) {
                state = StaticLights;
              } else if (header.indexOf("GET /static/off") >= 0) {
                state = Off;
              }

              if (header.indexOf("GET /color/red") >= 0) {
                currentColor = Cred;
              } else if (header.indexOf("GET /color/orange") >= 0) {
                currentColor = Corange;
              } else if (header.indexOf("GET /color/white") >= 0) {
                currentColor = Cwhite;
              } else if (header.indexOf("GET /color/blue") >= 0) {
                currentColor = Cblue;
              } else if (header.indexOf("GET /color/green") >= 0) {
                currentColor = Cgreen;
              } else if (header.indexOf("GET /color/") >= 0) {
                // "GET /color/decade"
                int r = getHexVal(header[11]) * 16 + getHexVal(header[12]);
                int g = getHexVal(header[13]) * 16 + getHexVal(header[14]);
                int b = getHexVal(header[15]) * 16 + getHexVal(header[16]);
                currentColor = RgbColor(r,g,b);
              }

              // Display the HTML web page
              client.println("<!DOCTYPE html><html>");
              client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
              client.println("<link rel=\"icon\" href=\"data:,\">");
              client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
              client.println(".button { background-color: #4CAF50; border: none; color: white; padding: 16px 40px; box-sizing: border-box;");
              client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
              client.println(".button2 { background-color: #555555; }");
              client.println(".red { background-color: red; }");
              client.println(".blue { background-color: blue; }");
              client.println(".orange { background-color: orange; }");
              client.println(".white { background-color: white; color: black; border: 2px solid black; padding: 14px 38px; }");
              client.println(".green { background-color: cyan; }");
              client.println("<style>");
              client.println(".my-input {");
              client.println("  width: 200px;");
              client.println("  padding: 10px;");
              client.println("  border: 1px solid #ccc;");
              client.println("  border-radius: 4px;");
              client.println("}");
              client.println(".my-input:focus {");
              client.println("  outline: none;");
              client.println("  border-color: blue;");
              client.println("}");
              client.println("</style></head>");

              // Web Page Heading
              client.println("<body><h1>Booming lights</h1>");
              String dynamicState = ((state == ReactiveLights) ? "On" : "Off");
              client.println("<p>Dynamic Lights - State " + dynamicState + "</p>");
              if (state == ReactiveLights) {
                client.println("<p><a href=\"/dynamic/off\"><button class=\"button button2\">OFF</button></a></p>");
              } else {
                client.println("<p><a href=\"/dynamic/on\"><button class=\"button\">ON</button></a></p>");
              }

              String staticState = ((state == StaticLights) ? "On" : "Off");
              client.println("<p>Static Lights - State " + staticState + "</p>");
              if (state == StaticLights) {
                client.println("<p><a href=\"/static/off\"><button class=\"button button2\">OFF</button></a></p>");
              } else {
                client.println("<p><a href=\"/static/on\"><button class=\"button\">ON</button></a></p>");
              }

              client.println("<p>Color Lights : " + colorToString(currentColor) + "</p>");
              client.println("<div class=\"container\">"); // Add a container class
              client.println("<div class=\"button-wrapper\">"); // Add a button wrapper div
              client.println("<a href=\"/color/red\"><button class=\"button red\">Red</button></a>");
              client.println("<a href=\"/color/blue\"><button class=\"button blue\">Blue</button></a>");
              client.println("<a href=\"/color/orange\"><button class=\"button orange\">Orange</button></a>");
              client.println("<a href=\"/color/white\"><button class=\"button white\">White</button></a>");
              client.println("<a href=\"/color/green\"><button class=\"button green\">Cyan</button></a>");
              client.println("<input type=\"color\" class=\"pickColor\"/>");
              client.println("<script> var selectedColor = document.querySelector(\".pickColor\"); selectedColor.addEventListener(\"change\", function (event){console.log(event.target.value);window.location = event.target.value.substring(1)}, false);</script>");
              client.println("</div>"); // Close the button wrapper div
              client.println("</div>"); // Close the container div

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
    if ( state == Off) {
      stopLights();
      delay(100);
      continue;
    } else if (state == StaticLights) {
      for (int i = 0; i < NUM_LEDS; i++) {
        strip.SetPixelColor(i, currentColor);
      }
      strip.Show();
    } else {
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
        strip.SetPixelColor(k / 2, RgbColor::LinearBlend(Cblack, currentColor, blendIndex));
      }
      strip.Show();
    }
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
