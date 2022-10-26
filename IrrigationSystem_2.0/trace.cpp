 #include "trace.h"

HTTPClient http;
WiFiClient client;

void trace(String message) {
  trace("UNDEFINED", TRACE, message);
};

void trace(String module, TraceLevel level, String message) {

  char buffer[1024];
  sprintf(buffer, "{\"uptime\" : %d, \"module\" : \"%s\", \"message\" : \"%s\", \"level\" : %d}", millis(), module.c_str(), message.c_str(), level);

  //Serial.print(level);
  //Serial.print(" ");  
  //Serial.print(millis());
  //Serial.print(" ");
  //Serial.print(module);
  //Serial.print(" ");
  //Serial.println(message);

  //Serial.println(buffer);

  //String payload = "{\"uptime\" : " + millis() + ", \"module\" : \"" + module + "\", \"message\" : \"" + message + "\", \"level\" : \"" + level + "\"}";
  //String payload = "aaaa" + "aa";


//   if (http.begin(client, "http://192.168.1.7:34888")) {  // HTTP
// //Serial.println("a");
//     http.addHeader("Content-Type", "application/json");
//     //Serial.println("b");
//     int httpCode = http.POST(String(buffer));


//     // httpCode will be negative on error
//     if (httpCode > 0) {
//       // HTTP header has been send and Server response header has been handled
//       //Serial.printf("[HTTP] POST... code: %d\n", httpCode);

//       // file found at server
//       if (httpCode == HTTP_CODE_OK) {
//         const String& payload = http.getString();
//         //Serial.println("received payload:\n<<");
//         //Serial.println(payload);
//         //Serial.println(">>");
//       }
//     } else {
//       //Serial.printf("[HTTP] POST... failed, error: %s\n", http.errorToString(httpCode).c_str());
//     }

//     //Serial.println("c");
//     http.end();
//     //Serial.println("d");
//   } else {
//     //Serial.println("e");
//     //Serial.printf("[HTTP} Unable to connect\n");
//   }

};
