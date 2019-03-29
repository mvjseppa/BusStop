#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>


/*
 WIFI CONFIGURATION
 */
char SSID[] = "uuid";
char pwd[] = "pwd";


#define POST_INTERVAL_SECONDS 10

/*
 * Oled screen
 */
#define OLED_RESET 0  // GPIO0

// driver is for 128 by 64 so we need some tweaking
Adafruit_SSD1306 display(OLED_RESET); 

void setup() {
  // ---------- WIFI
  Serial.begin(115200);
  WiFi.begin(SSID, pwd);
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Connected, IP address: ");
  Serial.println(WiFi.localIP());

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3C (for the 64x48)
  display.display();
}

#define SCREEN_WIDTH 60
int timer = 0;
int scroll = SCREEN_WIDTH;
int scrollDelta = 1;

void loop() {
  
  timer = POST_INTERVAL_SECONDS * 20;
  String payload = queryBusStop();

  String busTime = parseBusTime(payload);
  String busDestination = parseBusDestination(payload);

  int textWidth = busDestination.length() * 6;
  
  while(timer--){
    if(scroll > textWidth) scrollDelta = -1;
    else if(scroll < SCREEN_WIDTH) scrollDelta = 1;
    scroll += scrollDelta;

    Serial.print("scroll:");
    Serial.println(scroll);
    
    showTextOnDisplay(busTime, busDestination, scroll);
    delay(50);
  }
}


void showTextOnDisplay(String msg, String msg2, int scroll){
  display.clearDisplay();
  // text display tests
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(33,8);
  display.print(msg);
  display.setCursor(96 - scroll,24);
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

String queryBusStop()
{
  // first check the wifi status
  if (WiFi.status() != WL_CONNECTED) {  
    reconnect();
  }  

   HTTPClient http;    //Declare object of class HTTPClient
 
   http.begin("http://api.digitransit.fi/routing/v1/routers/hsl/index/graphql");      //Specify request destination
   http.addHeader("Content-Type", "application/json");  //Specify content-type header
 
   int httpCode = http.POST(
     "{\"query\": \"{stop(id: \\\"HSL:1310146\\\") {stoptimesWithoutPatterns {realtimeArrival headsign}}}\"}"
   );   //Send the request
   String payload = http.getString();                  //Get the response payload

   Serial.println(httpCode);   //Print HTTP return code
   Serial.println(payload);    //Print request response payload
 
   http.end();  //Close connection

   return payload;
}

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

String timeStringFromSeconds(int seconds){
  int minutes = seconds / 60;
  seconds = seconds % 60;

  int hours = minutes / 60;
  minutes = minutes % 60;

  char time[8];
  sprintf(time, "%02d:%02d:%02d", hours, minutes, seconds);
  return String(time);
}
