// Project has been designed for ESP8266

#define VERSION     "4.0"

#define AUTHOR      "SURBHIDEEP"
#define CONTACT     "surbhideep.27@gmail.com"
#define COPYRIGHT   "This code may be use as is, all modification need to be reviewed and pushed back into community repository"

#ifdef ESP8266
  
  #include "config.h"

  #include <Arduino.h>
  #include <math.h>
  #include "trace.h"
  #include <ESP8266WiFi.h>
  #include <WiFiClient.h>
  #include <ESP8266mDNS.h>
  #include <ArduinoOTA.h>
  #include <FlowMeter.h>

  #include <Arduino_JSON.h>
  #include <ArduinoJson.h>
  #include <LittleFS.h>

  #include <DNSServer.h>
  #include <ESPAsyncTCP.h>
  #include <ESPAsyncWebServer.h>
  #include <AsyncJson.h>

  #include "IrrigationController.h"

  bool switchRemoteUpdate = false;
  bool OTAUpdateOnProgress = false;
  bool needRestart = false;
  long dampeningTime = 0;

  unsigned int OTAUpdateProgress = 0;

  void setup();
  void loop();

  void initSerialCom(long speed, long timeout);
  void initOTAUpdate();
  void initWiFi();
  void initWebServer();
  void initController();
  void initBoard();
  void initLittleFS();

  bool loadConfig();
  bool saveConfig();


  // Config

  bool setupMode = true;
  bool wifiConnected = false;
  

  String WiFiSSID = "";
  String WiFiPass = "";

  void prinScanResult(int networksFound);

  String processor(const String& var);
  String processorWiFi(const String& var);
  IRAM_ATTR void MeterISR();

  class CaptiveRequestHandler : public AsyncWebHandler {
  public:
    CaptiveRequestHandler() {}
    virtual ~CaptiveRequestHandler() {}

    bool canHandle(AsyncWebServerRequest *request){
      request->addInterestingHeader("ANY");
      return true;
    }

    void handleRequest(AsyncWebServerRequest *request) {

      AsyncWebServerResponse *response = request->beginResponse(LittleFS, "/setup.html", "text/html", false, processorWiFi);
      
      response->addHeader("Cache-Control","no-cache, no-store, must-revalidate");
      response->addHeader("Pragma","no-cache");
      response->addHeader("Expires","-1");

      request->send(response);
    }
  };

  

#endif
