#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266NetBIOS.h>
#define CFKEY ""
#define CFUSER ""
#define CFZONE_NAME ""
#define CFRECORD_NAME ""
#define CFRECORD_TYPE A
#define CFTTL 120
#define FORCE false
#ifndef STASSID
#define STASSID "HMLS-GUEST"
#define STAPSK  "hmls2333"
#endif
String wanip = "0.0.0.0";
String content;
ESP8266WiFiMulti WiFiMulti;
ESP8266WebServer wwwserver(80);
long unsigned int  lastrefreshtime = 0;
const char* ssid = STASSID;
const char* password = STAPSK;
const uint8_t fingerprint[20] = {0x40, 0xaf, 0x00, 0x6b, 0xec, 0x90, 0x22, 0x41, 0x8e, 0xa3, 0xad, 0xfa, 0x1a, 0xe8, 0x25, 0x41, 0x1d, 0x1a, 0x54, 0xb3};
void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  wwwserver.on("/", handleRoot);
  wwwserver.on("/wow", []() {
    wwwserver.send(200, "text/plain", "this works as well");
  });
  NBNS.begin("ESP");
  wwwserver.begin();
  Serial.println("HTTP server started");
}
static void handleRoot(void) {
  content = F("<!DOCTYPE HTML>\n<html>Hello world from ESP8266");
  content += F("<p>");
  content += F("</html>");
  wwwserver.send(200, F("text/html"), content);
}
void setCloudflareDNS(){
  if ((WiFiMulti.run() == WL_CONNECTED)) {
    std::unique_ptr<BearSSL::WiFiClientSecure>client(new BearSSL::WiFiClientSecure);
    client->setInsecure();
    HTTPClient https;
    Serial.print("[HTTPS] begin...\n");
    if (https.begin(*client, "https://www.myazure.org")) {  // HTTPS
      Serial.print("[HTTPS] GET...\n");
      // start connection and send HTTP header
      int httpCode = https.GET();
      // httpCode will be negative on error
      if (httpCode > 0) {
        // HTTP header has been send and Server response header has been handled
        Serial.printf("[HTTPS] GET... code: %d\n", httpCode);
        // file found at server
        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
          String payload = https.getString();
          Serial.println(payload);
        }
      } else {
        Serial.printf("[HTTPS] GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
      }
      https.end();
    } else {
      Serial.printf("[HTTPS] Unable to connect\n");
    }
  }
}



void loop() {
  wwwserver.handleClient();
  // wait for WiFi connection
  if ((WiFiMulti.run() == WL_CONNECTED)) {
   
    if((millis()-lastrefreshtime)>10000){
    lastrefreshtime=millis();
    WiFiClient client;
    HTTPClient http;
    Serial.print("[HTTP] begin...\n");
    if (http.begin(client, "http://ipv4.icanhazip.com")) {  // HTTP
      Serial.print("[HTTP] GET...\n");
      // start connection and send HTTP header
      int httpCode = http.GET();
      // httpCode will be negative on error
      if (httpCode > 0) {
        // HTTP header has been send and Server response header has been handled
        Serial.printf("[HTTP] GET... code: %d\n", httpCode);
        // file found at server
        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
          String payload = http.getString();
          Serial.println(payload);
          if(wanip==payload){
            Serial.println("same from local cache ignore");
            setCloudflareDNS();
          }else{
            setCloudflareDNS();
            wanip=payload;
            Serial.println("not same from local cache waiting update");
          }
        }
      } else {
        Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
      }
      http.end();
    } else {
      Serial.printf("[HTTP} Unable to connect\n");
    }
  }
  }
}
