/*
    Copyright (C) 2021 kempermo


    ESP32GCalPrinter is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "calEvent.h"

WiFiClientSecure client;

String FetchGCal(String url);
String WebFetch(String url);

const char ssid[] = "XXX"; // put your wifi ssid here
const char pass[] = "XXX"; // put your wifi password here

#if __has_include(<urls.h>)
#  include <urls.h>
#  define URLS
#endif

#ifndef URLS
#define NETWORK_LIST_LENGTH 3
const char* script = "https://script.google.com/macros/s/A1B2C3D4/exec";
#else
const char* script = SCRIPT_URL;
#endif

// call the google script in defined timeframes
unsigned long lastScriptCall = 0;
unsigned long updateInterval = 1800000; // defined in milliseconds (e.g. 1000*seconds*minutes)

void setup()
{
  // start the serial connection
  Serial.begin(9600);
  delay(1000);

  // start the wifi
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  //Serial.print("Connecting to WiFi.");
  WiFi.begin(ssid, pass);

  // wait for the wifi to connect
  while (WiFi.status() != WL_CONNECTED) {
    //Serial.print(".");
    delay(300);
  }

  // print some debug information about the wifi connection
  //Serial.println("");
  //Serial.println("WiFi connected");
  //Serial.println("IP address: ");
  //Serial.println(WiFi.localIP());

  // reset timer
  lastScriptCall = millis() - updateInterval;
}

void loop()
{
  if (WiFi.status() == WL_CONNECTED)
  {
    if (millis() - lastScriptCall > updateInterval)
    {
      String response = FetchGCal(script);
      //Serial.printf("Today's Calendar:\n%s\n", response.c_str());

      //Extract current date
      String date = getValue(response, '\n', 0);
      date = date.substring(0, 15);
      //Serial.printf("Extracted Date: %s\n", date.c_str());

      //Create CalEvent Array
      byte eventsLength = getLength(response, '\n') - 1;
      //Serial.printf("Number Events: %u\n", eventsLength);

      // print todays events
      int i;
      for (i = 0; i < eventsLength; i++)
      {
        calEvent temp = calEvent(getValue(response, '\n', i + 1));
        Serial.println(temp.stringify());
        delay(5000);
      }

      lastScriptCall = millis();
    }
  }
}

String FetchGCal(String url)
{
  String Return1;
  String Return2;
  Return1 = WebFetch(url);
  Return2 = WebFetch(Return1);

  return (Return2);
}

String WebFetch(String url)
{
  const char *strURL;
  String Response;
  char server[80];
  bool Redirect = 0;

  strURL = url.c_str();
  //Serial.print("GCAL:URL:");
  //Serial.println(strURL);

  if (memcmp("https://", strURL, 8) == 0)
  {
    int i;
    for (i = 0; i < 80; i++)
    {
      if (strURL[i + 8] == '/')
        break;
      server[i] = strURL[i + 8];
    }
    server[i] = 0;
  }

  //Serial.print("GCAL:server:");
  //Serial.println(server);
  if (!client.connect(server, 443))
    //Serial.println("GCAL:No connection");
  else
  {
    //Serial.println("GCAL:Connect");
    // Make a HTTP request:
    client.print("GET ");
    client.print(url);
    client.println(" HTTP/1.0");

    client.print("Host: ");
    client.println(server);
    client.println("Connection: close");
    client.println();

    String header;
    while (client.connected())
    {
      String line = client.readStringUntil('\n');
      header = header + line + "\n";
      if (line.startsWith("Location: "))
      {
        Redirect = 1;
        Response = line.substring(line.indexOf("http"));
        //Serial.print("GCAL:REDIRECT:");
        //Serial.println(Response);
      }
      if (line == "\r")
        break;
    }
    //Serial.print("GCAL:HEADER:");
    //Serial.println(header);

    String body;
    while (client.available())
    {
      String line = client.readStringUntil('\n');
      body = body + line + "\n";
      if (line == "\r")
        break;
    }

    if (!Redirect)
    {
      //Serial.print("GCAL:BODY:");
      //Serial.println(body);
      Response = body;
    }

    client.stop();
  }

  return (Response);
}
