#include "wifi_handling.h"




wifi_handling::wifi_handling()
{

}

void wifi_handling::get_auth(wifi_authentications auth)
{
    this->auth = auth;
}

void wifi_handling::access_mode()
{
    WiFi.softAP(ACESS_MODE_SSID, ACESS_MODE_PASS);

    IPAddress IP = WiFi.softAPIP();
    Serial.print(F("AP IP address: "));
    Serial.println(IP);

    
}

void wifi_handling::client_mode(WIFI_RESET_CALLBACK)
{
    WiFi.begin(auth.ssid.c_str(),auth.password.c_str());
    //WiFi.begin("Pixel4AL","1234567890");
    Serial.print(F("Connecting to: "));
    Serial.println(auth.ssid);
    int index=0;

  while(!is_connected()) 
  {
    index++;
    rgb.connecting_to_wifi(index%NUMPIXELS,index/NUMPIXELS);
    callback();
    Serial.print(".");
  }

}

bool wifi_handling::is_connected()
{
    return WiFi.status() == WL_CONNECTED;
}

String wifi_handling::local_ip()
{
    String local_ip = WiFi.localIP().toString();
    Serial.println("");
    Serial.print(F("Connected to WiFi network with IP Address: "));
    Serial.println(local_ip);
    return local_ip;
}

