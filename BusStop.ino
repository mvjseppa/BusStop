#include <ESP8266WiFi.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "BusClient.h"


/*
 WIFI CONFIGURATION
 */
char SSID[] = "";
char pwd[] = "";

#define OLED_RESET 0  // GPIO0

// driver is for 128 by 64 so we need some tweaking
Adafruit_SSD1306 display(OLED_RESET); 

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
  
  busClient.updateStatus();

  String busTime = busClient.getTime();
  String busDetails = busClient.getDetails();

  int textWidth = busDetails.length() * 6;
  int maxScroll = max(textWidth - SCREEN_WIDTH, 0);

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
  display.setCursor(35,8);
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
  static const char pattern1[5][6] = {
    {"  .  "},
    {"  o  "},
    {"  O  "},
    {" { } "},
    {"{ . }"}
  };
  static const char* pattern2 = ".oO .oO .oO .oO";
  const char* anim2 = pattern2 + strlen(pattern2);
  
  
  for(int i=0; i<5; i++){

    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.setCursor(35,12);
    display.print(pattern1[i]);
    display.display();
    delay(10);
  }  
}
