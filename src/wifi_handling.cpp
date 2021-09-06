#include "wifi_handling.h"



AsyncWebServer server_hub(80);
e2prom_handling e2prom;
const char* PARAM_SSID = "ssid";
const char* PARAM_PASS = "password";

wifi_handling::wifi_handling()
{
    
}

void wifi_handling::get_auth(wifi_authentications auth)
{
    this->auth = auth;
}

bool wifi_handling::get_mode()
{
    return e2prom.wifi_mode_read();
}

void wifi_handling::set_mode(const char* mode)
{
    e2prom.wifi_mode_write(mode);
}



void wifi_handling::access_mode(NOT_FOUND callback)
{
    WiFi.softAP(ACESS_MODE_SSID, ACESS_MODE_PASS);

    IPAddress IP = WiFi.softAPIP();
    Serial.print(F("AP IP address: "));
    Serial.println(IP);

    server_hub.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
    {
        request->send_P(200, "text/html", html_hub_page);
    });

    server_hub.on("/get", HTTP_GET, [] (AsyncWebServerRequest *request) 
    {
        String inputParam,inputParam2;
        String SSID,Password;
        // GET input1 value on <ESP_IP>/get?input1=<inputMessage>
        if (request->hasParam(PARAM_SSID)) 
        {
        SSID = request->getParam(PARAM_SSID)->value();
        Password = request->getParam(PARAM_PASS)->value();
        inputParam = PARAM_SSID;
        inputParam2 = PARAM_PASS;

        // Writing SSID and Password on EEPROM
        String authenticaiton  =(SSID+"\n"+Password+"\n");
        e2prom.write(authenticaiton.c_str(),0);
        e2prom.wifi_mode_write (CLIENT_MODE);
        }
        else 
        {
        SSID = "No message sent";
        inputParam = "none";
        }
        //Serial.println(SSID+"\n"+Password);
        Serial.println(SSID+"\n"+Password);
        request->send(200, "text/html", SUCCESS_PAGE_HTML);
        delay(3000); // Wait to settle system
        ESP.restart();  // Automtically restarts system into client mode
        
    });
    server_hub.onNotFound(callback);
    server_hub.begin();

    
}

void wifi_handling::client_mode(WIFI_RESET_CALLBACK)
{
    auth = e2prom.wifi_auth_read();
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

void wifi_handling::scan_avaiable_wifis()
{
    int n = WiFi.scanNetworks();
  if (n == 0) 
  {
    Serial.println("no networks found");
  } 
  else 
  {
    Serial.print(n);
    Serial.println(" networks found");
    for (int i = 0; i < n; ++i) 
    {
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(WiFi.SSID(i));
      Serial.print(" (");
      Serial.print(WiFi.RSSI(i));
      Serial.print(") ");
      Serial.print(" [");
      Serial.print(WiFi.channel(i));
      Serial.print("] ");
      String encryptionTypeDescription = translateEncryptionType(WiFi.encryptionType(i));
      Serial.println(encryptionTypeDescription);
      Serial.println("");
      delay(10);
    }
  }
}

String wifi_handling::translateEncryptionType(wifi_auth_mode_t encryptionType) 
{
  switch (encryptionType) 
  {
    case (0):
      return "Open";
    case (1):
      return "WEP";
    case (2):
      return "WPA_PSK";
    case (3):
      return "WPA2_PSK";
    case (4):
      return "WPA_WPA2_PSK";
    case (5):
      return "WPA2_ENTERPRISE";
    default:
      return "UNKOWN";
  }
}
