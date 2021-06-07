#include "peripheral_handling.h"

//
peripheral_handling::peripheral_handling()
{
    
}

void peripheral_handling::setup()
{
    Serial.begin(115200);
    pinMode(PIR_PIN, INPUT_PULLDOWN);   
    pinMode(SIREN_PIN,OUTPUT);
    pinMode(LED_BUILTIN,OUTPUT);
    pinMode(SETUP_PIN, INPUT); // SETUP KEY
    digitalWrite(SIREN_PIN,LOW);
    buzzer_init(3, 1000, 8);
}

void peripheral_handling::builtin_led_reverse()
{
    digitalWrite(LED_BUILTIN,!digitalRead(LED_BUILTIN));
}

void peripheral_handling::builtin_led_on()
{
    digitalWrite(LED_BUILTIN,LOW);
}

void peripheral_handling::builtin_led_off()
{
    digitalWrite(LED_BUILTIN,HIGH);
}

bool peripheral_handling::setup_pin_state()
{
   return digitalRead(SETUP_PIN);
}

void peripheral_handling::buzzer_init(int channel, int freq, int resolution)
{
    ledcSetup(3, 1000, 8);
    ledcAttachPin(SIREN_PIN, 3);
    ledcWrite(3,0);
}

void peripheral_handling::buzzer_off()
{
    digitalWrite(SIREN_PIN,LOW);
    buzzer_value(0);
}

void peripheral_handling::buzzer_value(int value)
{
    ledcWrite(BUZZER_PWM_CH,value);
}

void peripheral_handling::buzzer_cycle()
{
    for (int duty_cycle=10;duty_cycle<200;duty_cycle++)
    {
        buzzer_value(duty_cycle);
        vTaskDelay(2);
    }
    for (int duty_cycle=200;duty_cycle>10;duty_cycle--)
    {
        buzzer_value(duty_cycle);
        vTaskDelay(2);
    }

    //buzzer_value(0);
    buzzer_off();
}


bool peripheral_handling::pir_state()
{
    return digitalRead(PIR_PIN);
}