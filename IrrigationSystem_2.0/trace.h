#ifndef TRACING001
  #define TRACING001

	#include <Arduino.h>
	#include <ESP8266HTTPClient.h>

  enum TraceLevel {
    ERROR,
    WARNING,
    INFO,
    TRACE
  };

  void trace(String message);
	void trace(String module, TraceLevel level, String message);



#endif