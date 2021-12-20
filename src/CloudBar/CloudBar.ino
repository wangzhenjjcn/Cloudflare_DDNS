#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266NetBIOS.h>
#include <HttpClient.h>
#include <Arduino_JSON.h>
 

#ifndef STASSID
#define STASSID "WIFISSID HERE"
#define STAPSK  "WIFI PASSWORD HERE"
#endif

long unsigned int  lastrefreshtime = 0;
const char* ssid = STASSID;
const char* password = STAPSK;
const uint8_t fingerprint[20] = {0x40, 0xaf, 0x00, 0x6b, 0xec, 0x90, 0x22, 0x41, 0x8e, 0xa3, 0xad, 0xfa, 0x1a, 0xe8, 0x25, 0x41, 0x1d, 0x1a, 0x54, 0xb3};
bool ledState = 0;
const int wifiPin = 0;
const int netPin = 2;


String CFKEY = "CloudFlareAPIkey";
String CFUSER = "CloudFlareUserNameEmail";
String CFZONE_NAME = "DomainName";
String CFRECORD_NAME = "DDNSDomainName";
String CFRECORD_TYPE = "A";//A-AAAA
String CFZONE_ID = "";
String CFRECORD_ID = "";
String CFTTL = "120";//120 default
String FORCE = "false";
String wanip = "0.0.0.0";
String cloudip = "0.0.0.0";
String IPCheckAddress = "http://ipv4.icanhazip.com";
ESP8266WiFiMulti WiFiMulti;
ESP8266WebServer wwwserver(80);


void setup() {
  Serial.begin(115200);
  pinMode(wifiPin, OUTPUT);
  pinMode(netPin, OUTPUT);
  digitalWrite(wifiPin, LOW);
  digitalWrite(netPin, LOW);
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
  wwwserver.on("/ip", []() {
    wwwserver.send(200, "text/plain", wanip);
  });
  NBNS.begin("ESP");
  wwwserver.begin();
  Serial.println("HTTP server started");
  digitalWrite(wifiPin, HIGH);
}

static void handleRoot(void) {
  String content;
  content = F("<!DOCTYPE HTML>\n<html>WangShiFu ESP8266 Processor~ HelloWorld~");
  content += F("<p>Now IP:<a>");
  content += wanip;
  content += F("</a></p>");
  content += F("</html>");
  wwwserver.send(200, F("text/html"), content);
}

void checkCFZONEID() {
  Serial.printf("Check CFZONEID ...\n");
  
  String url =  "https://api.cloudflare.com/client/v4/zones?name=" + CFZONE_NAME;
  std::unique_ptr<BearSSL::WiFiClientSecure>client(new BearSSL::WiFiClientSecure);
  client->setInsecure();
  HTTPClient https;
  Serial.println("[HTTPS] checkCFZONEID begin...\n");
  url.replace("\n", "");
  url.replace("\t", "");
  url.replace("\r", "");
  url.replace("\"", "");
  url.replace(" ", "");
  Serial.println(url);
  if (https.begin(*client, url)) {  // HTTPS
    Serial.println("[HTTPS] checkCFZONEID GET https://api.cloudflare.com/client/v4/zones?name=" + CFZONE_NAME + "...\n");
    https.addHeader("Content-Type", "application/json");
    https.addHeader("X-Auth-Email", "wangzhenjjcn@gmail.com");
    https.addHeader("X-Auth-Key", "875c2d32aa3dd7fc47f897f4a2a87f03a0edd");
    int httpCode = https.GET();
    if (httpCode > 0) {
      Serial.printf("[HTTPS] checkCFZONEID GET... code: %d\n", httpCode);
      if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
        String jsonDataString;
        String payload = https.getString();
        Serial.println(payload);
        jsonDataString = payload;
        JSONVar jsonData = JSON.parse(jsonDataString);
        if (JSON.typeof(jsonData) == "undefined") {
          Serial.println("Parsing input failed!");
          return;
        }
        JSONVar keys = jsonData.keys();
        int dataResult=-1;
        bool dataSuccess=false;
        String dataValue="";
        for (int i = 0; i < keys.length(); i++) {
          JSONVar value = jsonData[keys[i]];
          JSONVar dataKey=keys[i];
    
          JSONVar resultStr="result";
          if (dataKey==resultStr){
            dataResult=i;
          }
          JSONVar successKeyStr="success";
          JSONVar successValueStr=true;
          if (dataKey==successKeyStr){
            dataSuccess=(value==successValueStr);
          }
          Serial.print(keys[i]);
          Serial.print(" = ");
          Serial.println(value);
      
        }
        if (dataSuccess){
          Serial.println(" data success -=-=-=-=-=-=-=-= ");
        JSONVar idData=jsonData["result"][0]["id"];
        CFZONE_ID= JSON.stringify(idData);
        CFZONE_ID.replace("\n", "");
        CFZONE_ID.replace("\t", "");
        CFZONE_ID.replace("\r", "");
        CFZONE_ID.replace("\"", "");
        CFZONE_ID.replace(" ", "");
        Serial.println(idData);
        Serial.println("CFZONE_ID:"+CFZONE_ID);
        Serial.println(" data success -=-=-=-=-=-=-=-= ");
        }
      }else{
        String payload = https.getString();
        Serial.println("======err=====msg==========");
        Serial.println(payload);
        Serial.println("======err=====msg==========");
      }
    } else {
      Serial.printf("[HTTPS] checkCFZONEID GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
    }
    https.end();
  } else {
    Serial.printf("[HTTPS] Unable to connect\n");
  }

  //CFZONE_ID=$(curl -s -X GET "https://api.cloudflare.com/client/v4/zones?name=$CFZONE_NAME" -H "X-Auth-Email: $CFUSER" -H "X-Auth-Key: $CFKEY" -H "Content-Type: application/json" | grep -Po '(?<="id":")[^"]*' | head -1 )
}

void checkCFRECORDID() {
  Serial.printf("Check CFRECORDID ...\n");
 
  std::unique_ptr<BearSSL::WiFiClientSecure>client(new BearSSL::WiFiClientSecure);
  client->setInsecure();
  HTTPClient https;
  String url =   "https://api.cloudflare.com/client/v4/zones/" + CFZONE_ID + "/dns_records?name=" + CFRECORD_NAME ;
  url.replace("\n", "");
  url.replace("\t", "");
  url.replace("\r", "");
  url.replace("\"", "");
  url.replace(" ", "");
  Serial.println(url);
  Serial.print("[HTTPS] CFRECORDID begin...\n");
  if (https.begin(*client, url)) {  // HTTPS
    https.addHeader("Content-Type", "application/json");
    https.addHeader("X-Auth-Email", CFUSER);
    https.addHeader("X-Auth-Key", CFKEY);
    Serial.println("[HTTPS] CFRECORDID GET["+url+"]...\n");
    int httpCode = https.GET();
    if (httpCode > 0) {
      Serial.printf("[HTTPS] CFRECORDID GET... code: %d\n", httpCode);
      if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
        String payload = https.getString();
        Serial.println(" data success -=-=-=-=-=-=-=-= ");
        Serial.println(payload);
        Serial.println(" data success -=-=-=-=-=-=-=-= ");
        String jsonDataString;
        jsonDataString = payload;
        JSONVar jsonData = JSON.parse(jsonDataString);
        if (JSON.typeof(jsonData) == "undefined") {
          Serial.println("Parsing input failed!");
          return;
        }
        JSONVar keys = jsonData.keys();
        int dataResult=-1;
        bool dataSuccess=false;
        String dataValue="";
        for (int i = 0; i < keys.length(); i++) {
          JSONVar value = jsonData[keys[i]];
          JSONVar dataKey=keys[i];
    
          JSONVar resultStr="result";
          if (dataKey==resultStr){
            dataResult=i;
          }
          JSONVar successKeyStr="success";
          JSONVar successValueStr=true;
          if (dataKey==successKeyStr){
            dataSuccess=(value==successValueStr);
          }
          Serial.print(keys[i]);
          Serial.print(" = ");
          Serial.println(value);
      
        }
        if (dataSuccess){
          Serial.println(" data success -=-=-=-=-=-=-=-= ");
        JSONVar idData=jsonData["result"][0]["id"];
        CFRECORD_ID=JSON.stringify(idData);
        CFRECORD_ID.replace("\n", "");
        CFRECORD_ID.replace("\t", "");
        CFRECORD_ID.replace("\r", "");
        CFRECORD_ID.replace("\"", "");
        CFRECORD_ID.replace(" ", "");
        // Serial.println(idData);
        Serial.println("CFRECORD_ID:"+CFRECORD_ID); 
        Serial.println(" data success -=-=-=-=-=-=-=-= ");
        // String result=JSON.stringify(jsonData);
        // String id=JSON.stringify(jsonData["result"][0]["id"]);
        // String zone_id=JSON.stringify(jsonData["result"][0]["zone_id"]);
        // String zone_name=JSON.stringify(jsonData["result"][0]["zone_name"]);
        // String name=JSON.stringify(jsonData["result"][0]["name"]);
        // String type=JSON.stringify(jsonData["result"][0]["type"]);
        String content=JSON.stringify(jsonData["result"][0]["content"]);
        // String ttl=JSON.stringify(jsonData["result"][0]["ttl"]);
        // String modified_on=JSON.stringify(jsonData["result"][0]["modified_on"]);
        cloudip=content;
        cloudip.replace("\n", "");
        cloudip.replace("\t", "");
        cloudip.replace("\r", "");
        cloudip.replace("\"", "");
        cloudip.replace(" ", "");
        
        }
 

      }else{
        String payload = https.getString();
        Serial.println("======err=====msg==========");
        Serial.println(payload);
        Serial.println("======err=====msg==========");
      }
    } else {
      Serial.printf("[HTTPS] CFRECORDID GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
    }
    https.end();
  } else {
    Serial.printf("[HTTPS] Unable to connect\n");
  }
  //CFRECORD_ID=$(curl -s -X GET "https://api.cloudflare.com/client/v4/zones/$CFZONE_ID/dns_records?name=$CFRECORD_NAME" -H "X-Auth-Email: $CFUSER" -H "X-Auth-Key: $CFKEY" -H "Content-Type: application/json"  | grep -Po '(?<="id":")[^"]*' | head -1 )
}

void updateDNStoWanIP() {
  Serial.printf("Updating DNS to %s\n", wanip.c_str());
  std::unique_ptr<BearSSL::WiFiClientSecure>client(new BearSSL::WiFiClientSecure);
  client->setInsecure();
  HTTPClient https;
  String url =   "https://api.cloudflare.com/client/v4/zones/"+CFZONE_ID+"/dns_records/"+CFRECORD_ID ;
  url.replace("\n", "");
  url.replace("\t", "");
  url.replace("\r", "");
  url.replace("\"", "");
  url.replace(" ", "");
  Serial.println(url);
  Serial.print("[HTTPS] CFRECORDID begin...\n");
  if (https.begin(*client, url)) {  // HTTPS
  https.addHeader("Content-Type", "application/json");
  https.addHeader("X-Auth-Email", CFUSER);
  https.addHeader("X-Auth-Key", CFKEY);
    Serial.println("[HTTPS] CFRECORDID GET["+url+"]...\n");
    int httpCode = https.PUT("{\"id\":\""+CFZONE_ID+"\",\"type\":\""+CFRECORD_TYPE+"\",\"name\":\""+CFRECORD_NAME+"\",\"content\":\""+wanip+"\", \"ttl\":"+CFTTL+"}");
    if (httpCode > 0) {
      Serial.printf("[HTTPS] CFRECORDID GET... code: %d\n", httpCode);
      if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
        String payload = https.getString();
        Serial.println(" data success -=-=-=-=-=-=-=-= ");
        Serial.println(payload);
        Serial.println(" data success -=-=-=-=-=-=-=-= ");
        // String jsonDataString;
        // jsonDataString = payload;
        // JSONVar jsonData = JSON.parse(jsonDataString);
        // if (JSON.typeof(jsonData) == "undefined") {
        //   Serial.println("Parsing input failed!");
        //   return;
        // }
        // JSONVar keys = jsonData.keys();
        // int dataResult=-1;
        // bool dataSuccess=false;
        // String dataValue="";
        // for (int i = 0; i < keys.length(); i++) {
        //   JSONVar value = jsonData[keys[i]];
        //   JSONVar dataKey=keys[i];
    
        //   JSONVar resultStr="result";
        //   if (dataKey==resultStr){
        //     dataResult=i;
        //   }
        //   JSONVar successKeyStr="success";
        //   JSONVar successValueStr=true;
        //   if (dataKey==successKeyStr){
        //     dataSuccess=(value==successValueStr);
        //   }
        //   Serial.print(keys[i]);
        //   Serial.print(" = ");
        //   Serial.println(value);
      
        // }
        // if (dataSuccess){
        // Serial.println(" data success -=-=-=-=-=-=-=-= ");
        // String result=JSON.stringify(jsonData);
        // String id=JSON.stringify(jsonData["result"][0]["id"]);
        // String zone_id=JSON.stringify(jsonData["result"][0]["zone_id"]);
        // String zone_name=JSON.stringify(jsonData["result"][0]["zone_name"]);
        // String name=JSON.stringify(jsonData["result"][0]["name"]);
        // String type=JSON.stringify(jsonData["result"][0]["type"]);
        // String content=JSON.stringify(jsonData["result"][0]["content"]);
        // String ttl=JSON.stringify(jsonData["result"][0]["ttl"]);
        // String modified_on=JSON.stringify(jsonData["result"][0]["modified_on"]);
        // Serial.println(" data success -=-=-=-=-=-=-=-= ");
        
        // }
 

      }else{
        String payload = https.getString();
        Serial.println("======err=====msg==========");
        Serial.println(payload);
        Serial.println("======err=====msg==========");
      }
    } else {
      Serial.printf("[HTTPS] CFRECORDID GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
    }
    https.end();
  } else {
    Serial.printf("[HTTPS] Unable to connect\n");
  }
  // RESPONSE=$(curl -s -X PUT "https://api.cloudflare.com/client/v4/zones/$CFZONE_ID/dns_records/$CFRECORD_ID" \
  -H "X-Auth-Email: $CFUSER" \
  -H "X-Auth-Key: $CFKEY" \
  -H "Content-Type: application/json" \
  --data "{\"id\":\"$CFZONE_ID\",\"type\":\"$CFRECORD_TYPE\",\"name\":\"$CFRECORD_NAME\",\"content\":\"$WAN_IP\", \"ttl\":$CFTTL}")

}

void setCloudflareDNS() {
  refreshWanIP();
  if(CFZONE_ID==""){
  checkCFZONEID();
  }
  if (CFRECORD_ID==""){
  checkCFRECORDID();
  }
  if (wanip==cloudip){
    digitalWrite(netPin, LOW);
    digitalWrite(netPin, HIGH);
    digitalWrite(netPin, LOW);
    digitalWrite(netPin, HIGH);
    digitalWrite(netPin, LOW);
    digitalWrite(netPin, HIGH);
  }else{
  updateDNStoWanIP();
  }
}

void refreshWanIP() {
  // wait for WiFi connection
  if ((WiFiMulti.run() == WL_CONNECTED)) {
    digitalWrite(netPin, LOW);
    if (((millis() - lastrefreshtime) > 10000)) {
    digitalWrite(netPin, HIGH);
      lastrefreshtime = millis();
      WiFiClient client;
      HTTPClient http;
      if (http.begin(client, "http://ipv4.icanhazip.com")) {  // HTTP
        Serial.println("[HTTP]GET["+IPCheckAddress+"]...\n");
        // start connection and send HTTP header
        int httpCode = http.GET();
        // httpCode will be negative on error
        if (httpCode > 0) {
          // HTTP header has been send and Server response header has been handled
          Serial.printf("[HTTP] GET... code: %d\n", httpCode);
          // file found at server
          if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
            String payload = http.getString();
            String lastwanip = payload;
            lastwanip.replace("\n", "");
            lastwanip.replace("\t", "");
            lastwanip.replace("\r", "");
            lastwanip.replace(" ", "");
            Serial.println(lastwanip);
            if (wanip == lastwanip and wanip==cloudip) {
              Serial.printf("cache:[%s] now:[%s] ignore...\n", wanip.c_str(), lastwanip.c_str());
              setCloudflareDNS();
            } else {
              Serial.printf("cache:[%s] now:[%s] waiting update...\n", wanip.c_str(), lastwanip.c_str());
              wanip = lastwanip;
              wanip.replace("\n", "");
              wanip.replace("\t", "");
              wanip.replace("\r", "");
              wanip.replace(" ", "");
              setCloudflareDNS();
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
    digitalWrite(netPin, LOW);
  }
}

void loop() {
  wwwserver.handleClient();
  refreshWanIP();
}
