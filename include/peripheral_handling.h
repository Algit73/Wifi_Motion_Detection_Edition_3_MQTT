#include "Arduino.h"

// Pins assigning
#define PIR_PIN         2
#define LED_BUILTIN     12
#define SIREN_PIN       14
#define RGB_TEST        15
#define SETUP_PIN       16
#define ON              true
#define OFF             false

#define BUZZER_PWM_CH   3


class peripheral_handling
{
    private:

    public:
    peripheral_handling();
    void setup(void);
    void builtin_led_reverse(void);
    void builtin_led_on(void);
    void builtin_led_off(void);
    void buzzer_cycle(void);
    void buzzer_off(void);
    void buzzer_value(int value);
    void buzzer_init(int channel, int freq, int resolution);
    bool setup_pin_state(void);
    bool pir_state(void);



};
