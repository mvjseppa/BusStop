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
  const String url = "http://api.digitransit.fi/routing/v1/routers/hsl/index/graphql";
  const String query = "{\"query\": \"{stop(id: \\\"HSL:1310146\\\") {stoptimesWithoutPatterns {realtimeArrival headsign}}}\"}";
  StaticJsonDocument<1024> jsonDoc;

public:
  bool updateInfo()
  {
    if (WiFi.status() != WL_CONNECTED) {  
      Serial.println("no wifi!");
      return false;
     }  

     HTTPClient http;
     http.begin(url);
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
     
     return true;
  }

  String getTime(){
    int seconds = jsonDoc["data"]["stop"]
        ["stoptimesWithoutPatterns"][0]
        ["realtimeArrival"];
  
    int minutes = seconds / 60;
    seconds = seconds % 60;

    int hours = minutes / 60 % 24;
    minutes = minutes % 60;

    char time[8];
    sprintf(time, "%02d:%02d:%02d", hours, minutes, seconds);
    return String(time);
  }

  String getDestination(){
    const char* dest = jsonDoc["data"]["stop"]
        ["stoptimesWithoutPatterns"][0]
        ["headsign"];
  
    return String(dest);
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
    loadingAnimation();
  }
  Serial.println();
  Serial.print("Connected, IP address: ");
  Serial.println(WiFi.localIP());

}

#define POST_INTERVAL 200 
#define SCREEN_WIDTH 60

BusStopClient busClient;

void loop() {

  static int scroll = 0;
  static int scrollDelta = 1;
  static int timer = POST_INTERVAL;
  
  busClient.updateInfo();

  String busTime = "123";//parseBusTime(payload);
  String busDestination = "Alakurtti";//parseBusDestination(payload);

  int textWidth = busDestination.length() * 7;
  int maxScroll = max(textWidth - SCREEN_WIDTH, 0);
  Serial.println(textWidth);

  if(maxScroll == 0) scrollDelta = 0;
  
  while(timer--){
    if(scroll > maxScroll) scrollDelta = -1;
    else if(scroll < 0) scrollDelta = 1;
    
    scroll += scrollDelta;
    
    showTextOnDisplay(
      busClient.getTime(), 
      busClient.getDestination(), 
      scroll
    );
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


/*
String parseBusTime(String payload){
  String fieldname = "realtimeArrival";
  int startIdx = payload.indexOf(fieldname) + fieldname.length() + 2;
  int endIdx = payload.indexOf(',', startIdx);
  
  String slicedPayload = payload.substring(startIdx, endIdx);

  Serial.println(slicedPayload);
  
  return timeStringFromSeconds(slicedPayload.toInt());
}


String parseBusDestination(String payload){
  String fieldname = "headsign";
  int startIdx = payload.indexOf(fieldname) + fieldname.length() + 3;
  int endIdx = payload.indexOf('"', startIdx);
  
  String destination = payload.substring(startIdx, endIdx);

  Serial.println(destination);
  
  return destination;
}
*/
const char animation[5][6] = {
  {"  .  "},
  {"  o  "},
  {"  O  "},
  {" {.} "},
  {"{ . }"}
};

void loadingAnimation(){
  for(int i=0; i<5; i++){
    showTextOnDisplay(animation[i], "", 0);
    delay(50);
  }  
}
