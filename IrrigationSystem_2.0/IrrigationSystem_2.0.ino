/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * 
 *                 IRRIGATION SYSTEM CONTROLLER
 * 
 *      #VERSION: 2.4.0
 *      
 * 
 * 
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "IrrigationSystem_2.0.h"

IrrigationController    irrigationController(PIN_REVERSER, PIN_BOOST);
IrrigationZone*         zone[8];
FlowMeter               *Meter;

AsyncWebServer          server(EXPOSED_PORT);
DNSServer               dnsServer;

void setup() {

  initBoard();
  initSerialCom(115200, 10000);

  Serial.println("Setting up Controller ...");
  irrigationController.setState(ENABLE);
  irrigationController.setStatus(IDLE);

  Serial.println("Add zones...");

  zone[0] = new IrrigationZone(0, PIN_STRIKE_1);
  zone[1] = new IrrigationZone(1, PIN_STRIKE_2);
  zone[2] = new IrrigationZone(2, PIN_STRIKE_3);
  zone[3] = new IrrigationZone(3, PIN_STRIKE_4);

  zone[0]->state = ENABLE;
  zone[1]->state = ENABLE;
  zone[2]->state = ENABLE;
  zone[3]->state = ENABLE;

  Serial.println("Setting up LittleFS ...");
  initLittleFS();

  if (loadConfig())
    Serial.println("Last config loaded");
  else
    Serial.println("Error loading config.");

  Serial.println("Setting up WiFi ...");
  initWiFi();

  Serial.println("Setting up OTA Update...");
  initOTAUpdate();

  Serial.println("Setting up FlowMeter ...");
  Meter = new FlowMeter(digitalPinToInterrupt(PIN_FLOW_SENSOR), FS300A, MeterISR, RISING);
  

  // Upon reboot, reset all zone with a stop signal. This force the 
  // actuator to get its position and make sure all valve get glosed 
  // if the unit get restarted by power lost.

  Serial.println("Initializing Controller ...");
  irrigationController.initSequance(zone);

  Serial.println("Setting up WebServer ...");
  initWebServer();

  Serial.println("You may reach the RestAPI at:");
  
  Serial.print("\thttp://");
  Serial.print(WiFi.localIP().toString());
  Serial.println("/");

  Serial.print("\thttp://");
  Serial.print(HOSTNAME);
  Serial.print(".local/");
}

void loop() {


  if (needRestart) {
    restart();
  }
  else if (setupMode) {
    dnsServer.processNextRequest();
  }
  else if (switchRemoteUpdate) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(500);
    ArduinoOTA.handle();
    digitalWrite(LED_BUILTIN, LOW);
  } 
  else {  // Check if WiFi is connected
    if (WiFi.status() != WL_CONNECTED)
      restart();
    
    MDNS.update();
  }

  delay(300);

  Meter->tick(300);

  // if (irrigationController.getActiveZone() != NULL)
  //   irrigationController.getActiveZone()->flow = Meter->getCurrentFlowrate();

  irrigationController.safetyCheck(zone);
  irrigationController.timeoutCheck(zone);
  irrigationController.handleRequests();
}

void initBoard() {
  //pinMode(PIN_FLOW_SENSOR, FUNCTION_3);
  pinMode(A0, INPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
}

void initLittleFS() {
  if(!LittleFS.begin()){
    //Serial.println("An Error has occurred while mounting LittleFS");
    return;
  }
}

void initSerialCom(long speed, long timeout) {
  Serial.begin(speed, SERIAL_8N1, SERIAL_TX_ONLY);
  Serial.setTimeout(timeout);
  Serial.println("Serial Com as been initialized");
  Serial.println("------------------------------");
}

void initWiFi() {

  if (setupMode) { // if there is no wifi config, or if btn is press then go in setupMode, captive portal.
    
    Serial.println("MODE: Initial Setup");
    //WiFi.mode(WIFI_AP_STA);

    WiFi.softAP("ISC", "12345678");

    IPAddress myIP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(myIP);
    
    WiFi.scanNetworksAsync(NULL);

    dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
    bool dnsStarted = dnsServer.start(53, "*", myIP);

    if (!dnsStarted)
      Serial.println("Error starting DNS!!");

  } 
  else {
    Serial.println("MODE: Normal");
    //WiFi.mode(WIFI_STA);
    WiFi.begin(WiFiSSID, WiFiPass);
    while (WiFi.waitForConnectResult() != WL_CONNECTED) {

      switch (WiFi.waitForConnectResult()) {
        case WL_NO_SSID_AVAIL:
        case WL_WRONG_PASSWORD:
          setupMode = true;
          saveConfig();
          delay(5000);
          restart();
        break;
        default:
          Serial.println("Connection Failed! Rebooting...");
          delay(5000);
          restart();
      }

    }
  }

  if (!MDNS.begin(HOSTNAME)) {
    Serial.println("Error setting up MDNS responder!");
  }

  MDNS.addService("http", "tcp", 80);
}

void initWebServer() {

  server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");

  // For captive portal, only then setuping the device
  server.on("/config", HTTP_GET, [](AsyncWebServerRequest *request) {  
    request->send(LittleFS, "/config.json", String(), false);
  }).setFilter(ON_AP_FILTER);

  server.on("/config", HTTP_POST, [](AsyncWebServerRequest *request) {  
    
    if (request->hasParam("WiFiSSID", true) && request->hasParam("WiFiPASS", true)) {
      WiFiSSID = request->getParam("WiFiSSID", true)->value().c_str();
      WiFiPass = request->getParam("WiFiPASS", true)->value().c_str();

      zone[0]->name = request->getParam("ZoneName0", true)->value().c_str();
      zone[1]->name = request->getParam("ZoneName1", true)->value().c_str();
      zone[2]->name = request->getParam("ZoneName2", true)->value().c_str();
      zone[3]->name = request->getParam("ZoneName3", true)->value().c_str();


      setupMode = false;
      saveConfig();

      if (request->hasParam("restart", true))
        needRestart = true;

      request->send(201, "text/html", "ISC will now connect to your WiFi, you may get back to your primary network.");
    }

    request->send(409);

  }).setFilter(ON_AP_FILTER);

  server.on("/config", HTTP_PUT, [](AsyncWebServerRequest *request) {  
    request->send(201);
  },
  [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final){},
  [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    
    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, data);

    if (error) {
      //Serial.print(F("deserializeJson() failed: "));
      //Serial.println(error.f_str());
      request->send(409, "application/json", "{\"status\" : \"deserializeJson() failed:\"}");
    }

    WiFiSSID = String(doc["wifiSSID"]);
    WiFiPass = String(doc["wifiPASS"]);

    zone[0]->name = String(doc["zoneName0"]);
    zone[1]->name = String(doc["zoneName1"]);
    zone[2]->name = String(doc["zoneName2"]);
    zone[3]->name = String(doc["zoneName3"]);

    setupMode = false;
    saveConfig();

    if (request->hasParam("restart")) {
      needRestart = true;
    }

    request->send(201, "application/json", "{\"status\" : \"All right\"}");

  });

  server.addHandler(new CaptiveRequestHandler()).setFilter(ON_AP_FILTER);

  // For captive portal, only then setuping the device END

  server.on("/stats", HTTP_GET, [](AsyncWebServerRequest *request) {  
    request->send(LittleFS, "/stats.json", String(), false, processor);
  });

  server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request) {  
    request->send(LittleFS, "/status.json", String(), false, processor);
  });

  server.on("/reset", HTTP_PUT, [](AsyncWebServerRequest *request) {  
    resetConfig();
    needRestart = true;
    request->send(201);
  });

  server.onNotFound([](AsyncWebServerRequest *request){
    request->send(404, "text/plain", "Endpoint your are looking for does not exist.");
  });

  /* Enable Over The Air update mode */
  server.on("/ota-update", HTTP_PUT, [](AsyncWebServerRequest *request) {
    switchRemoteUpdate = true;    
    request->send(200, "application/json", "{\"status\": \"You may now update using OTA\"}");
  }).setAuthentication("user", "pass");

  server.on("/restart", HTTP_POST, [](AsyncWebServerRequest *request) {  
    needRestart = true;
    request->send(200, "application/json", "{\"status\": \"Ok will do a restart in a momomomomomoment\"}");
  });

  server.on("/boost", HTTP_POST, [](AsyncWebServerRequest *request) {  
    request->send(200, "application/json", "{\"status\": \"Ok boost\"}");
    irrigationController.boost();
  });

  /* Modify an existing zone */
  server.on("/zone", HTTP_PUT, [](AsyncWebServerRequest *request) {  
    
    if (request->hasParam("action")) {
      
      String action = request->getParam("action")->value().c_str();
      action.trim();

      if (request->hasParam("id")) {
        
        short id = (short) atoi(request->getParam("id")->value().c_str());

        if (action == "rename") {
          if (request->hasParam("name", true)) {
            zone[id]->name = request->getParam("name", true)->value().c_str();
            saveConfig();
            request->send(204);
          }
          else
            request->send(404, "application/json", "{\"status\": \"Missing argument: name.\"}");
        } else if (action == "enable") {
            zone[id]->state = ENABLE;
            saveConfig();
            request->send(204);
        } else if (action == "disable") {
            zone[id]->state = DISABLE;
            saveConfig();
            request->send(204);
        } else
          request->send(404, "application/json", "{\"status\": \"The action you requested does not exist here.\"}");
      } else
        request->send(400, "application/json", "{\"status\": \"You need to specify a zone id.\"}");
    } else
      request->send(400, "application/json", "{\"status\": \"You need to specify an action.\"}");

  });

  server.on("/zone/start", HTTP_POST, [](AsyncWebServerRequest *request) {

    if (irrigationController.getStatus() == IDLE) {
      if (request->hasParam("id")) {

        if (dampeningTime > millis()) {
          AsyncWebServerResponse *response = request->beginResponse(503, "application/json", "{\"status\": \"The controller is in DAMPENING TIME state.\"}");
          response->addHeader("Retry-After","2");
          request->send(response);
        } else {

          short id = (short) atoi(request->getParam("id")->value().c_str());
          
          if (request->hasParam("timeout")) {
            short timeout = (short) atoi(request->getParam("timeout")->value().c_str());
            irrigationController.startZone(zone[id], timeout * 1000);
          } else
            irrigationController.startZone(zone[id]);
          
          dampeningTime = millis() + 2000; // Give 2 sec of dampening before another zone action.

          //request->send(204);
          request->redirect("/stats");
        }

      } else
        request->send(400, "application/json", "{\"status\": \"You need to specify a zone id.\"}");
    } else
      request->send(503, "application/json", "{\"status\": \"The controller is not in IDLE state.\"}");

  });

  server.on("/zone/stop", HTTP_POST, [](AsyncWebServerRequest *request) {

        if (request->hasParam("id")) {

          if (dampeningTime > millis()) {
            AsyncWebServerResponse *response = request->beginResponse(503, "application/json", "{\"status\": \"The controller is in DAMPENING TIME state.\"}");
            response->addHeader("Retry-After","2");
            request->send(response);
          } else {

            short id = (short) atoi(request->getParam("id")->value().c_str());

            irrigationController.stopZone(zone[id]);
            dampeningTime = millis() + 2000; // Give 2 sec of dampening before another zone action.
            request->send(204);
          }
        } else
          request->send(400, "application/json", "{\"status\": \"You need to specify a zone id.\"}");

  });


  server.on("/flow", HTTP_GET, [](AsyncWebServerRequest *request){
    JSONVar ret;
    double a = 2.19;
    ret[0] = Meter->getCurrentFlowrate();
    ret[1] = Meter->getCurrentVolume();
    ret[2] = Meter->getTotalFlowrate();
    ret[3] = Meter->getTotalVolume();
    ret[4] = a;
    
    String jsonString = JSON.stringify(ret);
    
    request->send(200, "application/json", jsonString);
  });
    
  server.begin();
}

void initOTAUpdate() {
  ArduinoOTA.setHostname(HOSTNAME);
  
  ArduinoOTA.onStart([]() {

    String type;

    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_FS
      type = "filesystem";
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    //Serial.println("Start updating " + type);
    OTAUpdateOnProgress = true;
    
  });

  ArduinoOTA.onEnd([]() {
    //Serial.println("\nEnd");
    OTAUpdateOnProgress = false;
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    unsigned int pct_progress = (progress / (total / 100));
    //Serial.printf("Progress: %u%%\r", pct_progress);
    OTAUpdateProgress = pct_progress;
  });

  ArduinoOTA.onError([](ota_error_t error) {
    //Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      //Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      //Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      //Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      //Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      //Serial.println("End Failed");
    }
  });

  ArduinoOTA.begin();
  
  Serial.println("OTA Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void initController() {
}


IRAM_ATTR void MeterISR() { Meter->count(); }


// Find and match token in static file for stats.
String processor(const String& var) {

  if(var == "VERSION") {
    return String(VERSION);
  }
  else if(var == "UPTIME") {
    return String(round(millis() / 1000));
  }
  // else if(var == "BOOST_LEVEL") {
  //   return String(irrigationController.getBoostLevel());
  // }
  else if (var == "CONTROLLER_STATUS") {
    return String(irrigationController.getStatus());
  }
  else if (var == "CONTROLLER_STATE") {
    return String(irrigationController.getState());
  }
  else if (var == "ACTIVE_ZONE") {
    if (irrigationController.getActiveZone() != NULL)
      return String(irrigationController.getActiveZone()->id);
    else
      return "-1";
  }

  else if (var == "TIMELEFT") {
    long zt = zone[0]->getTimeLeft() + zone[1]->getTimeLeft() + zone[2]->getTimeLeft() + zone[3]->getTimeLeft();
    return String(zt);
  }

  else if (var == "ZONE_0_NAME")
    return String(zone[0]->name);
  else if (var == "ZONE_0_STATUS")
    return String(zone[0]->status);
  else if (var == "ZONE_0_STATE")
    return String(zone[0]->state);
  else if (var == "ZONE_0_FLOW_LAST_MIN")
    return String(zone[0]->flow);
  else if (var == "ZONE_0_TIMELEFT")
    return String(zone[0]->getTimeLeft());

  else if (var == "ZONE_1_NAME")
    return String(zone[1]->name);
  else if (var == "ZONE_1_STATUS")
    return String(zone[1]->status);
  else if (var == "ZONE_1_STATE")
    return String(zone[1]->state);
  else if (var == "ZONE_1_FLOW_LAST_MIN")
    return String(zone[1]->flow);
  else if (var == "ZONE_1_TIMELEFT")
    return String(zone[1]->getTimeLeft());

  else if (var == "ZONE_2_NAME")
    return String(zone[2]->name);
  else if (var == "ZONE_2_STATUS")
    return String(zone[2]->status);
  else if (var == "ZONE_2_STATE")
    return String(zone[2]->state);
  else if (var == "ZONE_2_FLOW_LAST_MIN")
    return String(zone[2]->flow);
  else if (var == "ZONE_2_TIMELEFT")
    return String(zone[2]->getTimeLeft());

  else if (var == "ZONE_3_NAME")
    return String(zone[3]->name);
  else if (var == "ZONE_3_STATUS")
    return String(zone[3]->status);
  else if (var == "ZONE_3_STATE")
    return String(zone[3]->state);
  else if (var == "ZONE_3_FLOW_LAST_MIN")
    return String(zone[3]->flow);
  else if (var == "ZONE_3_TIMELEFT")
    return String(zone[3]->getTimeLeft());

  else if (var == "ISUPDATING")
    return String(OTAUpdateOnProgress);
  else if (var == "UPDATEPROGRESS")
    return String(OTAUpdateProgress);

  return F("UNDEFINED");
}

String processorWiFi(const String& var) {

  if(var == "AVAIL_SSID") {

    String Page;

    int n = WiFi.scanComplete();

    if (n > 0) {
      for (int i = 0; i < n; i++) {
        Page += String("\r\n<option value=\"") + WiFi.SSID(i) + String("\">") + WiFi.SSID(i) + String(" ") + WiFi.RSSI(i) + String("</option>");
      }
    } else {
      Page += F("<option>No WLAN found</option>");
    }

    return Page;
  }

  return F("<option>No SSID Found</option>");
}

void restart() {
  Serial.end();
  ESP.restart();
}

bool loadConfig() {

  File configFile = LittleFS.open("/config.json", "r");
  if (!configFile) {
    Serial.println("Failed to open config file");
    return false;
  }

  size_t size = configFile.size();
  if (size > 1024) {
    Serial.println("Config file size is too large");
    return false;
  }

  // Allocate a buffer to store contents of the file.
  std::unique_ptr<char[]> buf(new char[size]);

  // We don't use String here because ArduinoJson library requires the input
  // buffer to be mutable. If you don't use ArduinoJson, you may as well
  // use configFile.readString instead.
  configFile.readBytes(buf.get(), size);

  StaticJsonDocument<200> doc;
  auto error = deserializeJson(doc, buf.get());
  if (error) {
    Serial.println("Failed to parse config file");
    return false;
  }

  WiFiSSID = String(doc["wifiSSID"]);
  WiFiPass = String(doc["wifiPass"]);
  setupMode = boolean(doc["setupMode"]);

  zone[0]->name = String(doc["zone"][0]);
  zone[1]->name = String(doc["zone"][1]);
  zone[2]->name = String(doc["zone"][2]);
  zone[3]->name = String(doc["zone"][3]);


  return true;
}

bool saveConfig() {
  StaticJsonDocument<200> doc;
  
  doc["wifiSSID"] = WiFiSSID;
  doc["wifiPass"] = WiFiPass;
  doc["setupMode"] = setupMode;

  doc["zone"][0] = zone[0]->name;
  doc["zone"][1] = zone[1]->name;
  doc["zone"][2] = zone[2]->name;
  doc["zone"][3] = zone[3]->name;

  File configFile = LittleFS.open("/config.json", "w");
  if (!configFile) {
    Serial.println("Failed to open config file for writing");
    return false;
  }

  serializeJson(doc, configFile);
  return true;
}

bool resetConfig() {
  StaticJsonDocument<200> doc;
  
  doc["wifiSSID"] = "none";
  doc["wifiPass"] = "none";
  doc["setupMode"] = "true";

  File configFile = LittleFS.open("/config.json", "w");
  if (!configFile) {
    Serial.println("Failed to open config file for writing");
    return false;
  }

  serializeJson(doc, configFile);
  return true;
}