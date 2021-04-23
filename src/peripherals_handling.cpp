#include "peripheral_handling.h"


peripheral_handling::peripheral_handling()
{
    
}

void peripheral_handling::setup()
{
    Serial.begin(115200);
    pinMode(PIR_PIN, INPUT_PULLDOWN);   
    pinMode(SIREN_PIN,OUTPUT);
    pinMode(LIGHTING,OUTPUT);
    pinMode(LED_BUILTIN,OUTPUT);
    pinMode(SETUP_PIN, INPUT); // SETUP KEY
    digitalWrite(SIREN_PIN,LOW);
    digitalWrite(LIGHTING,LOW);

    ledcSetup(3, 1000, 8);
    ledcAttachPin(SIREN_PIN, 3);
    ledcWrite(3,0);
}

void peripheral_handling::builtin_led_reverse()
{
    digitalWrite(LED_BUILTIN,!digitalRead(LED_BUILTIN));
}

void peripheral_handling::builtin_led_on()
{
    digitalWrite(LED_BUILTIN,HIGH);
}

void peripheral_handling::builtin_led_off()
{
    digitalWrite(LED_BUILTIN,LOW);
}

bool peripheral_handling::setup_pin_state()
{
   return digitalRead(SETUP_PIN);
}

void peripheral_handling::buzzer_off()
{
    digitalWrite(SIREN_PIN,LOW);
}

void peripheral_handling::buzzer_on()
{
    digitalWrite(SIREN_PIN,HIGH);
}

void peripheral_handling::light_on()
{
    digitalWrite(LIGHTING,HIGH);
}

void peripheral_handling::light_off()
{
    digitalWrite(LIGHTING,LOW);
}

bool peripheral_handling::pir_state()
{
    return digitalRead(PIR_PIN);
}