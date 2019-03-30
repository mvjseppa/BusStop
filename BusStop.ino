#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ArduinoJson.h>


/*
 WIFI CONFIGURATION
 */
char SSID[] = "";
char pwd[] = "";

#define OLED_RESET 0  // GPIO0

// driver is for 128 by 64 so we need some tweaking
Adafruit_SSD1306 display(OLED_RESET); 

class BusStopClient{
private:
  const String timeUrl = "http://worldtimeapi.org/api/timezone/Europe/Helsinki";
  const String hslUrl = "http://api.digitransit.fi/routing/v1/routers/hsl/index/graphql";
  const String query = "{\"query\": \"{stop(id: \\\"HSL:1310146\\\") {stoptimesWithoutPatterns {realtimeArrival headsign trip {route {shortName}}}}}\"}";
  StaticJsonDocument<1024> jsonDoc;
  HTTPClient http;
  int realTimeSeconds = 0;
  int hslSeconds = 0;
  String details;

public:
  bool updateInfo()
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

  String getTime(){
    return String((hslSeconds - realTimeSeconds) / 60) + " min";
  }

  String& getDetails(){
    return details;
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

void setup() {
  Serial.begin(115200);
  WiFi.begin(SSID, pwd);
  Serial.print("Connecting");

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3C (for the 64x48)
  display.display();
  
  while (WiFi.status() != WL_CONNECTED)
  {
    wifiAnimation();
  }
  Serial.println();
  Serial.print("Connected, IP address: ");
  Serial.println(WiFi.localIP());

}

#define POST_INTERVAL 200 
#define SCREEN_WIDTH 64

BusStopClient busClient;

void loop() {

  static int scroll = 0;
  static int scrollDelta = 1;
  int timer = POST_INTERVAL;
  
  busClient.updateInfo();

  String busTime = busClient.getTime();
  String busDetails = busClient.getDetails();

  int textWidth = busDetails.length() * 6;
  int maxScroll = max(textWidth - SCREEN_WIDTH, 0);
  Serial.println(textWidth);

  if(maxScroll == 0) scrollDelta = 0;
  
  while(timer--){
    if(scroll > maxScroll) scrollDelta = -1;
    else if(scroll < 0) scrollDelta = 1;
    
    scroll += scrollDelta;
    
    showTextOnDisplay(busTime, busDetails, scroll);
    delay(50);
  }
}


void showTextOnDisplay(String msg, String msg2, int scroll){
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(33,8);
  display.print(msg);
  display.setCursor(33 - scroll,24);
  display.setTextSize(1);
  display.print(msg2);
  display.display();
}

// wifi reconnect, not tested
void reconnect() {  
  Serial.print("Reconnecting");
  WiFi.mode(WIFI_STA);  
  WiFi.begin(SSID, pwd);  
  while (WiFi.status() != WL_CONNECTED) {  
      delay(500);  
      Serial.print(".");
  }  
  Serial.println("Connected!");
}

void wifiAnimation(){
  static const char animation[5][6] = {
    {"  .  "},
    {"  o  "},
    {"  O  "},
    {" {.} "},
    {"{ . }"}
  };
  static const char* animation2 = ".oOo.oOo.oOo.oOo.";
  
  for(int i=0; i<5; i++){
    showTextOnDisplay(animation[i], animation2 + i, 0);
    delay(50);
  }  
}
