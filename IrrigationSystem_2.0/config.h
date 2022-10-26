/*****
 * 
 * config.h contain dynamic data that the user may have to adjust before using the sketch.
 * 
 *****/

#define HOSTNAME        "irrigationcontroller"
#define EXPOSED_PORT    80

#ifdef ESP8266

    #define PIN_BOOST       D1
    #define PIN_BOOST_LEVEL A0
    #define PIN_REVERSER    D2

    #define PIN_STRIKE_1    D6
    #define PIN_STRIKE_2    D8
    #define PIN_STRIKE_3    D5
    #define PIN_STRIKE_4    D7

    #define PIN_FLOW_SENSOR 3

#endif