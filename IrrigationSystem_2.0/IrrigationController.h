#ifndef IRRIGATIONCONTROLLER
  #define IRRIGATIONCONTROLLER

  #include <Arduino.h>
  #include "trace.h"
  
  enum IrrigationACTION {
    NONE,
    STARTZONE,
    STOPZONE
  };

  enum IrrigationSTATUS {
    IDLE,                                 // Controller or Zone is not used
    STARTING,                             // Controller or Zone is in the process of initiating an anction, loading the booster, opening the valve etc
    RUNNING,                              // Controller or Zone is actively running, meaning that water is coming out somewhere !
    STOPPING,                             // Controller or Zone is in the process of teminating an action, loading the booster, closing the valve etc
    BOOSTING                              // Controller is loding the booster
  };

  enum IrrigationSTATE {
    DISABLE,                             // Controller or Zone is disable, no action will be permit
    ENABLE                               // Controller or Zone is enable, action can be made on
  };

  class IrrigationZone {
    public:
      short                   id;              // Zone ID
      short                   pin;              // GPIO where relay is hooked-up to
      String                  name;             // Readable name of that zone.
      IrrigationSTATUS        status;           // Status of the zone, idle, starting, running, stopping
      IrrigationSTATE         state;            // State of the zone, enable, disable
      double                  flow;             // Flow based on main thread delay
      long                    timeout;

      IrrigationZone(short zoneID, short zonePIN) {
        id = zoneID;
        pin = zonePIN;
        name = "UNDEFINED";
        state = DISABLE;
        status = IDLE;
        timeout = 0;

        pinMode(pin, OUTPUT);
        digitalWrite(pin, LOW);
      };

      long getTimeLeft() {
        if (timeout > 0)
          return (timeout - millis()) / 1000;
        else
          return 0;
      }
  };

  struct IrrigationJob {
    IrrigationACTION        action;
    IrrigationZone*         zone;
    long                    timeout;
  };
  
  class IrrigationController {

    private:
      IrrigationJob     jobPlan;
      bool              needBoost;

      short             _polarityReverserPin = -1;
      short             _relayBoosterPin = -1;

      IrrigationSTATE   _state;
      IrrigationSTATUS  _status;

      IrrigationZone*   activeZone = NULL;//new IrrigationZone(-1, -1);

      long              latestTickTime = 0;
      long              safetyKillTime = 0; // Used to stop activity after 1h if valve has been forgot.
      long              safetyKillTimeOutMills = 3600000; // Timeout set to 1h.
              
      long              actionTimeout;    // Used to plan the next action when triggering a strike.
              
      void              reversePolarity(bool);
              
      void              startBooster();
      void              stopBooster();
              
      void              startZoneON(IrrigationZone* zone);
      void              startZoneOFF(IrrigationZone* zone);
      void              stopZoneON(IrrigationZone* zone);
      void              stopZoneOFF(IrrigationZone* zone);

    public:
      IrrigationController(short polarityReverserPin, short relayBoosterPin);

      void              boost();

      void              setState(IrrigationSTATE state) { _state = state; };
      void              setStatus(IrrigationSTATUS status) { _status = status; };

      IrrigationSTATE   getState() { return _state; };
      IrrigationSTATUS  getStatus() { return _status; };

      IrrigationZone*   getActiveZone() { return activeZone; };
      // short             getBoostLevel(); // Read the boost level, usefull to see if an action can be made, which need boost.

      bool              startZone(IrrigationZone* zone);
      bool              startZone(IrrigationZone* zone, long timeout);
      bool              stopZone(IrrigationZone* zone);

      void              initSequance(IrrigationZone* zone[]);
      
      void              timeoutCheck(IrrigationZone* zone[]);
      void              safetyCheck(IrrigationZone* zone[]);

      void              handleRequests();
  };

#endif
