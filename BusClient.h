#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>

class BusStopClient{
public:
  bool updateStatus()
  {
    if (WiFi.status() != WL_CONNECTED) {  
      Serial.println("no wifi!");
      return false;
     }  

     queryHslApi();
     wdt_reset(); //HTTP calls are slow; need to kick the dog.
     queryWorldTimeApi();
     
     return true;
  }

  String getTime(){
    return String((hslSeconds - realTimeSeconds) / 60) + " min";
  }

  String& getDetails(){
    return details;
  }
  

private:
  const String timeUrl = "http://worldtimeapi.org/api/timezone/Europe/Helsinki";
  const String hslUrl = "http://api.digitransit.fi/routing/v1/routers/hsl/index/graphql";
  const String query = "{\"query\": \"{stop(id: \\\"HSL:1310146\\\") {stoptimesWithoutPatterns {realtimeArrival headsign trip {route {shortName}}}}}\"}";
  StaticJsonDocument<1024> jsonDoc;
  HTTPClient http;
  int realTimeSeconds = 0;
  int hslSeconds = 0;
  String details;

  bool queryHslApi(){
     http.begin(hslUrl);
     http.addHeader("Content-Type", "application/json");
     int httpCode = http.POST(query);   
     String payload = http.getString();
  
     Serial.println(httpCode);
     Serial.println(payload);
   
     http.end();
     auto error = deserializeJson(jsonDoc, payload.c_str());

     if(error){
      Serial.println("json error");
      Serial.println(error.c_str());
      return false;
     }

     hslSeconds = jsonDoc["data"]["stop"]
        ["stoptimesWithoutPatterns"][0]
        ["realtimeArrival"];

     setDetails();

     return true;
  }

  bool queryWorldTimeApi(){
    http.begin(timeUrl);
    http.addHeader("Content-Type", "application/json");
    int httpCode = http.GET();
    String payload = http.getString();

    Serial.println(httpCode);
    Serial.println(payload);

    http.end();

    auto error = deserializeJson(jsonDoc, payload.c_str());

    if(error){
      Serial.println("json error");
      Serial.println(error.c_str());
      return false;
    }

    int Y, M, D, h, m;
    float s; 
    sscanf(jsonDoc["datetime"], 
      "%d-%d-%dT%d:%d:%f", 
      &Y, &M, &D, &h, &m, &s);
      
    realTimeSeconds = (h * 60 + m)*60 + s;
    Serial.print("rt: ");
    Serial.print(h);
    Serial.print(m);
    Serial.println(s);
    Serial.println(realTimeSeconds);
  }

  String setDetails(){
    
    const char* dest = jsonDoc["data"]["stop"]
        ["stoptimesWithoutPatterns"][0]
        ["headsign"];

    const char* line = jsonDoc["data"]["stop"]
        ["stoptimesWithoutPatterns"][0]
        ["trip"]["route"]["shortName"];
  
    details = String(line) + ": " + String(dest);
  }
};
