/*
  ===========================================
       Copyright (c) 2017 Stefan Kremser
              github.com/spacehuhn
  ===========================================
*/

// Including some libraries we need //
#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <FS.h>

// Settings //

// More Includes! //
extern "C" {
  #include "user_interface.h"
}

ESP8266WebServer server(80);

#include <EEPROM.h>
#include "data.h"
#include "NameList.h"
#include "APScan.h"
#include "ClientScan.h"
#include "Attack.h"
#include "Settings.h"
#include "SSIDList.h"

/* ========== DEBUG ========== */
// turn this off, otherwise it will be sending garbage to the attiny!!
const bool debug = true;
/* ========== DEBUG ========== */

// Run-Time Variables //
String wifiMode = "";
String attackMode_deauth = "";
String attackMode_beacon = "";
String scanMode = "SCAN";

bool warning = true;

NameList nameList;

APScan apScan(debug);
ClientScan clientScan(debug);
Attack attack(debug);
Settings settings(debug);
SSIDList ssidList(debug);

void sniffer(uint8_t *buf, uint16_t len) {
  clientScan.packetSniffer(buf, len);
}

void startWifi() {
  
  Serial.println("<r1:0>");
  Serial.println("<r2:0>");
  Serial.println("<g1:0>");
  Serial.println("<g2:0>");
  Serial.println("<b1:100>");
  Serial.println("<b2:100>");
  Serial.println("<playf:100>");
  Serial.println("<playb:100>");  

  if (debug)
    Serial.println("\nStarting WiFi AP:");

  WiFi.mode(WIFI_STA);
  wifi_set_promiscuous_rx_cb(sniffer);
  WiFi.softAP((const char*)settings.ssid.c_str(), (const char*)settings.password.c_str(), settings.apChannel, settings.ssidHidden); //for an open network without a password change to:  WiFi.softAP(ssid);
  
  if (debug) {

    Serial.println("SSID     : '" + settings.ssid+"'");
    Serial.println("Password : '" + settings.password+"'");
    Serial.println("-----------------------------------------------");
  }

  if (settings.password.length() < 8 && debug) Serial.println("WARNING: password must have at least 8 characters!");
  if ((settings.ssid.length() < 1 || settings.ssid.length() > 32) && debug) Serial.println("WARNING: SSID length must be between 1 and 32 characters!");
  wifiMode = "ON";
}

void stopWifi() {

  Serial.println("<r1:0>");
  Serial.println("<r2:0>");
  Serial.println("<g1:0>");
  Serial.println("<g2:0>");
  Serial.println("<b1:0>");
  Serial.println("<b2:0>");

  if (debug) {

    Serial.println("stopping WiFi AP");
    Serial.println("-----------------------------------------------");
  }

  WiFi.disconnect();
  wifi_set_opmode(STATION_MODE);
  wifiMode = "OFF";
}

void loadIndexHTML() {
  if(warning){
    sendFile(200, "text/html", data_indexHTML, sizeof(data_indexHTML));
  }else{
    sendFile(200, "text/html", data_apscanHTML, sizeof(data_apscanHTML));
  }
}
void loadAPScanHTML() {
  warning = false;
  sendFile(200, "text/html", data_apscanHTML, sizeof(data_apscanHTML));
}
void loadStationsHTML() {
  sendFile(200, "text/html", data_stationsHTML, sizeof(data_stationsHTML));
}
void loadAttackHTML() {
  sendFile(200, "text/html", data_attackHTML, sizeof(data_attackHTML));
}
void loadSettingsHTML() {
  sendFile(200, "text/html", data_settingsHTML, sizeof(data_settingsHTML));
}
void load404() {
  sendFile(200, "text/html", data_errorHTML, sizeof(data_errorHTML));
}
void loadInfoHTML(){
  sendFile(200, "text/html", data_infoHTML, sizeof(data_infoHTML));
}
void loadLicense(){
  sendFile(200, "text/plain", data_license, sizeof(data_license));
}

void loadFunctionsJS() {
  sendFile(200, "text/javascript", data_js_functionsJS, sizeof(data_js_functionsJS));
}
void loadAPScanJS() {
  sendFile(200, "text/javascript", data_js_apscanJS, sizeof(data_js_apscanJS));
}
void loadStationsJS() {
  sendFile(200, "text/javascript", data_js_stationsJS, sizeof(data_js_stationsJS));
}
void loadAttackJS() {
  attack.ssidChange = true;
  sendFile(200, "text/javascript", data_js_attackJS, sizeof(data_js_attackJS));
}
void loadSettingsJS() {
  sendFile(200, "text/javascript", data_js_settingsJS, sizeof(data_js_settingsJS));
}

void loadStyle() {
  sendFile(200, "text/css;charset=UTF-8", data_styleCSS, sizeof(data_styleCSS));
}


void startWiFi(bool start) {
  if (start) startWifi();
  else stopWifi();
  clientScan.clearList();
}

//==========AP-Scan==========
void startAPScan() {

  Serial.println("<r1:0>");
  Serial.println("<r2:0>");
  Serial.println("<g1:150>");
  Serial.println("<g2:150>");
  Serial.println("<b1:150>");
  Serial.println("<b2:150>");
  Serial.println("<playb:100>");
  Serial.println("<playe:100>");

  scanMode = "scanning...";

  if (apScan.start()) {

    server.send ( 200, "text/json", "true");
    attack.stopAll();
    scanMode = "SCAN";
  }
}

void sendAPResults() {
  apScan.sendResults();
}

void selectAP() {
  if (server.hasArg("num")) {
    apScan.select(server.arg("num").toInt());
    server.send( 200, "text/json", "true");
    attack.stopAll();
  }
}

//==========Client-Scan==========
void startClientScan() {

  Serial.println("<r1:150>");
  Serial.println("<r2:150>");
  Serial.println("<g1:150>");
  Serial.println("<g2:150>");
  Serial.println("<b1:0>");
  Serial.println("<b2:0>");
  Serial.println("<playa:100>");
  Serial.println("<playg:100>");  

  if (server.hasArg("time") && apScan.getFirstTarget() > -1 && !clientScan.sniffing) {
    server.send(200, "text/json", "true");
    clientScan.start(server.arg("time").toInt());
    attack.stopAll();
  } else server.send( 200, "text/json", "Error: no selected access point");
}

void sendClientResults() {
  clientScan.send();
}
void sendClientScanTime() {
  server.send( 200, "text/json", (String)settings.clientScanTime );
}

void selectClient() {
  if (server.hasArg("num")) {
    clientScan.select(server.arg("num").toInt());
    attack.stop(0);
    server.send( 200, "text/json", "true");
  }
}

void addClientFromList(){
  if(server.hasArg("num")) {
    int _num = server.arg("num").toInt();
    clientScan.add(nameList.getMac(_num));
    
    server.send( 200, "text/json", "true");
  }else server.send( 200, "text/json", "false");
}

void setClientName() {
  if (server.hasArg("id") && server.hasArg("name")) {
    if(server.arg("name").length()>0){
      nameList.add(clientScan.getClientMac(server.arg("id").toInt()), server.arg("name"));
      server.send( 200, "text/json", "true");
    }
    else server.send( 200, "text/json", "false");
  }
}

void deleteName() {
  if (server.hasArg("num")) {
    int _num = server.arg("num").toInt();
    nameList.remove(_num);
    server.send( 200, "text/json", "true");
  }else server.send( 200, "text/json", "false");
}

void clearNameList() {
  nameList.clear();
  server.send( 200, "text/json", "true" );
}

void editClientName() {
  if (server.hasArg("id") && server.hasArg("name")) {
    nameList.edit(server.arg("id").toInt(), server.arg("name"));
    server.send( 200, "text/json", "true");
  }else server.send( 200, "text/json", "false");
}

void addClient(){
  if(server.hasArg("mac") && server.hasArg("name")){
    String macStr = server.arg("mac");
    macStr.replace(":","");

    if(debug)
      Serial.println("add "+macStr+" - "+server.arg("name"));
    
    if(macStr.length() < 12 || macStr.length() > 12) server.send( 200, "text/json", "false");
    else{
      Mac _newClient;
      for(int i=0;i<6;i++){
        const char* val = macStr.substring(i*2,i*2+2).c_str();
        uint8_t valByte = strtoul(val, NULL, 16);

        if (debug) {

          Serial.print(valByte,HEX);
          Serial.print(":");
        }

        _newClient.setAt(valByte,i);
      }

      if (debug)
        Serial.println();

      nameList.add(_newClient,server.arg("name"));
      server.send( 200, "text/json", "true");
    }
  }
}

//==========Attack==========
void sendAttackInfo() {
  attack.sendResults();
}

void startAttack() {

  Serial.println("<r1:150>");
  Serial.println("<r2:150>");
  Serial.println("<g1:0>");
  Serial.println("<g2:0>");
  Serial.println("<b1:0>");
  Serial.println("<b2:0>");
  Serial.println("<playa:100>");
  Serial.println("<playg:100>");  

  if (server.hasArg("num")) {
    int _attackNum = server.arg("num").toInt();
    if (apScan.getFirstTarget() > -1 || _attackNum == 1 || _attackNum == 2) {
      attack.start(server.arg("num").toInt());
      server.send ( 200, "text/json", "true");
    } else server.send( 200, "text/json", "false");
  }
}

void addSSID() {
  if(server.hasArg("ssid") && server.hasArg("num") && server.hasArg("enc")){
    int num = server.arg("num").toInt();
    if(num > 0){
      ssidList.addClone(server.arg("ssid"),num, server.arg("enc") == "true");
    }else{
      ssidList.add(server.arg("ssid"), server.arg("enc") == "true" || server.arg("enc") == "1");
    }
    attack.ssidChange = true;
    server.send( 200, "text/json", "true");
  }else server.send( 200, "text/json", "false");
}

void cloneSelected(){
  if(apScan.selectedSum > 0){
    int clonesPerSSID = 48/apScan.selectedSum;
    ssidList.clear();
    for(int i=0;i<apScan.results;i++){
      if(apScan.isSelected(i)){
        ssidList.addClone(apScan.getAPName(i),clonesPerSSID, apScan.getAPEncryption(i) != "none");
      }
    }
  }
  attack.ssidChange = true;
  server.send( 200, "text/json", "true");
}

void deleteSSID() {
  ssidList.remove(server.arg("num").toInt());
  attack.ssidChange = true;
  server.send( 200, "text/json", "true");
}

void randomSSID() {
  ssidList._random();
  attack.ssidChange = true;
  server.send( 200, "text/json", "true");
}

void clearSSID() {
  ssidList.clear();
  attack.ssidChange = true;
  server.send( 200, "text/json", "true");
}

void resetSSID() {
  ssidList.load();
  attack.ssidChange = true;
  server.send( 200, "text/json", "true");
}

void saveSSID() {
  ssidList.save();
  server.send( 200, "text/json", "true");
}

void restartESP() {
  server.send( 200, "text/json", "true");
  ESP.reset();
}

void enableRandom(){
  attack.changeRandom(server.arg("interval").toInt());
  server.send( 200, "text/json", "true");
}

//==========Settings==========
void getSettings() {
  settings.send();
}

void saveSettings() {
  if (server.hasArg("ssid")) settings.ssid = server.arg("ssid");
  if (server.hasArg("ssidHidden")) {
    if (server.arg("ssidHidden") == "false") settings.ssidHidden = false;
    else settings.ssidHidden = true;
  }
  if (server.hasArg("password")) settings.password = server.arg("password");
  if (server.hasArg("apChannel")) {
    if (server.arg("apChannel").toInt() >= 1 && server.arg("apChannel").toInt() <= 14) {
      settings.apChannel = server.arg("apChannel").toInt();
    }
  }
  if (server.hasArg("macAp")) {
    String macStr = server.arg("macAp");
    macStr.replace(":","");
    Mac tempMac;
     if(macStr.length() == 12){
       for(int i=0;i<6;i++){
         const char* val = macStr.substring(i*2,i*2+2).c_str();
         uint8_t valByte = strtoul(val, NULL, 16);
         tempMac.setAt(valByte,i);
       }
       if(tempMac.valid()) settings.macAP.set(tempMac);
     } else if(macStr.length() == 0){
       settings.macAP.set(settings.defaultMacAP);
     }
  }
  if (server.hasArg("randMacAp")) {
    if (server.arg("randMacAp") == "false") settings.isMacAPRand = false;
    else settings.isMacAPRand = true;
  }
  if (server.hasArg("scanTime")) settings.clientScanTime = server.arg("scanTime").toInt();
  if (server.hasArg("timeout")) settings.attackTimeout = server.arg("timeout").toInt();
  if (server.hasArg("deauthReason")) settings.deauthReason = server.arg("deauthReason").toInt();
  if (server.hasArg("packetRate")) settings.attackPacketRate = server.arg("packetRate").toInt();
  if (server.hasArg("apScanHidden")) {
    if (server.arg("apScanHidden") == "false") settings.apScanHidden = false;
    else settings.apScanHidden = true;
  }
  if (server.hasArg("beaconInterval")) {
    if (server.arg("beaconInterval") == "false") settings.beaconInterval = false;
    else settings.beaconInterval = true;
  }
  if (server.hasArg("useLed")) {
    if (server.arg("useLed") == "false") settings.useLed = false;
    else settings.useLed = true;
    attack.refreshLed();
  }
  if (server.hasArg("channelHop")) {
    if (server.arg("channelHop") == "false") settings.channelHop = false;
    else settings.channelHop = true;
  }
  if (server.hasArg("multiAPs")) {
    if (server.arg("multiAPs") == "false") settings.multiAPs = false;
    else settings.multiAPs = true;
  }
  if (server.hasArg("multiAttacks")) {
    if (server.arg("multiAttacks") == "false") settings.multiAttacks = false;
    else settings.multiAttacks = true;
  }
  
  if (server.hasArg("ledPin")) settings.setLedPin(server.arg("ledPin").toInt());
  if(server.hasArg("macInterval")) settings.macInterval = server.arg("macInterval").toInt();

  settings.save();
  server.send( 200, "text/json", "true" );
}

void resetSettings() {
  settings.reset();
  server.send( 200, "text/json", "true" );
}

void setup() {

  randomSeed(os_random());
  
#ifdef USE_LED16
  pinMode(16, OUTPUT);
  digitalWrite(16, LOW);
#endif
  
  Serial.begin(9600);
  delay(2000);

  pinMode(2, OUTPUT);
  delay(50);
  digitalWrite(2, HIGH);
  
  attackMode_deauth = "START";
  attackMode_beacon = "START";

  EEPROM.begin(4096);
  SPIFFS.begin();

  settings.load();
  if (debug) settings.info();
  settings.syncMacInterface();
  nameList.load();
  ssidList.load();

  attack.refreshLed();

  delay(500); // Prevent bssid leak

  startWifi();
  attack.stopAll();
  attack.generate();

  /* ========== Web Server ========== */

  /* HTML */
  server.onNotFound(load404);

  server.on("/", loadIndexHTML);
  server.on("/index.html", loadIndexHTML);
  server.on("/apscan.html", loadAPScanHTML);
  server.on("/stations.html", loadStationsHTML);
  server.on("/attack.html", loadAttackHTML);
  server.on("/settings.html", loadSettingsHTML);
  server.on("/info.html", loadInfoHTML);
  server.on("/license", loadLicense);

  /* JS */
  server.on("/js/apscan.js", loadAPScanJS);
  server.on("/js/stations.js", loadStationsJS);
  server.on("/js/attack.js", loadAttackJS);
  server.on("/js/settings.js", loadSettingsJS);
  server.on("/js/functions.js", loadFunctionsJS);

  /* CSS */
  server.on ("/style.css", loadStyle);

  /* JSON */
  server.on("/APScanResults.json", sendAPResults);
  server.on("/APScan.json", startAPScan);
  server.on("/APSelect.json", selectAP);
  server.on("/ClientScan.json", startClientScan);
  server.on("/ClientScanResults.json", sendClientResults);
  server.on("/ClientScanTime.json", sendClientScanTime);
  server.on("/clientSelect.json", selectClient);
  server.on("/setName.json", setClientName);
  server.on("/addClientFromList.json", addClientFromList);
  server.on("/attackInfo.json", sendAttackInfo);
  server.on("/attackStart.json", startAttack);
  server.on("/settings.json", getSettings);
  server.on("/settingsSave.json", saveSettings);
  server.on("/settingsReset.json", resetSettings);
  server.on("/deleteName.json", deleteName);
  server.on("/clearNameList.json", clearNameList);
  server.on("/editNameList.json", editClientName);
  server.on("/addSSID.json", addSSID);
  server.on("/cloneSelected.json", cloneSelected);
  server.on("/deleteSSID.json", deleteSSID);
  server.on("/randomSSID.json", randomSSID);
  server.on("/clearSSID.json", clearSSID);
  server.on("/resetSSID.json", resetSSID);
  server.on("/saveSSID.json", saveSSID);
  server.on("/restartESP.json", restartESP);
  server.on("/addClient.json",addClient);
  server.on("/enableRandom.json",enableRandom);

  server.begin();

  if(debug){
    Serial.println("\nStarting...\n");
  }

  Serial.println("<r1:0>");
  Serial.println("<r2:0>");
  Serial.println("<g1:100>");
  Serial.println("<g2:100>");
  Serial.println("<b1:0>");
  Serial.println("<b2:0>");
  Serial.println("<playf:100>");
  Serial.println("<playe:100>");  
}

void loop() {

  if (clientScan.sniffing) {

    if (clientScan.stop()) startWifi();
  } else {
    
    server.handleClient();
    attack.run();
  }
}
