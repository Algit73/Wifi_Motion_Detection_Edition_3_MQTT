#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif
#include "pins_arduino.h"
#include <Adafruit_NeoPixel.h>

#define LED_RGB         15
#define NUMPIXELS       8
#define ALARM_ONLY      0
#define SIREN_ONLY      1
#define ALARM_AND_SIREN 2

class rgb_handling
{
    private:
    Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, LED_RGB, NEO_GRB + NEO_KHZ800);

    public:
        rgb_handling (void);
        void connecting_to_wifi(int,int);
        void wifi_connected();
        void error(void);
        void siren_alarm(void);
        void siren_and_alarm(void);
        void on_off(bool on);
};
