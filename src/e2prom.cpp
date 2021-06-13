#include "EEPROM.h"
#include "e2prom_handling.h"

#define access_client_mode_address  119

wifi_authentications ssid_password;

void e2prom_handling::init()
{
  if (!EEPROM.begin(EEPROM_SIZE))
  {
    Serial.println(F("failed to initialise EEPROM")); delay(1000);
  }
}

void e2prom_handling::write(const char* string,int index)
{
  init();

  for (int i=0; i<(0+strlen(string));i++)
    EEPROM.write(i+index, string[i]);

  EEPROM.commit();

}

String e2prom_handling::read(int index_begin, int index_end)
{
  init();
  String string="";

  for(int i=index_begin;i<(index_end+1);i++)
    string += (char) EEPROM.read(i);
    
  return string;

}

// Reading from flash to retrieve SSID and Password
wifi_authentications e2prom_handling::wifi_auth_read()
{
  init();

  Serial.println(F("Reading from Flash0:"));
  for (int i = 0; i < EEPROM_SIZE; i++)
  {
    char beta = EEPROM.read(i);
    Serial.print(beta); Serial.print(" ");
  }
    
  Serial.println(F("Reading SSID from Flash:"));
  int ssid_length;
  for (int i = 0; i < EEPROM_SIZE/2; i++)
  {
    char character;
    character = EEPROM.read(i);
    Serial.print(character);
    ssid_length = i;
    if(character=='\n')
      break;
    ssid_password.ssid += character;
  }
  Serial.print('\n');
  Serial.println(F("Reading Password from Flash:"));
  for (int i = ssid_length+1; i < EEPROM_SIZE; i++)
  {
    char character;
    character = EEPROM.read(i);
    Serial.print(character);
    if(character=='\n')
      break;
    ssid_password.password += character;// byte(EEPROM.read(i))
  }

  Serial.print(F("SSID: ")); Serial.println(ssid_password.ssid);
  Serial.print(F("Password: ")); Serial.println(ssid_password.password);

  return ssid_password;
}

void e2prom_handling::wifi_mode_write(const char* mode)
{
  
  Serial.println(F("E2PROM Write..."));
  Serial.println(mode);
  write(mode,access_client_mode_address);
  wifi_mode_read();
  
}

bool e2prom_handling::wifi_mode_read()
{

  String mode = read(access_client_mode_address,access_client_mode_address);
  Serial.print(F("WIFI Mode:")); Serial.println(mode);

  if(mode==CLIENT_MODE)
    return true;
  else
    return false;
  
}