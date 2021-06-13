#include "Arduino.h"
#include <WiFi.h>
#include "e2prom_handling.h"
#include "rgb_handling.h"

#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

#define ACESS_MODE_SSID     "Hushyar_Conf"
#define ACESS_MODE_PASS     "123456789"

#include <functional>
#define WIFI_RESET_CALLBACK std::function<void()> callback
//
const char html_hub_page [] PROGMEM = R"rawliteral(
        <!DOCTYPE HTML><html><head>
        <title>ESP Input Form</title>
        <meta name="viewport" content="width=device-width, initial-scale=1">
        </head><body>
        <form action="/get">
            ssid: <input type="text" name="ssid"><br><br>
            password: <input type="text" name="password"><br><br>
            <input type="submit" value="Submit">
        </form><br>
        
        </body></html>)rawliteral";



#define MQTT_CALLBACK std::function<void(char*, uint8_t*, unsigned int)> callback
#define NOT_FOUND std::function<void(AsyncWebServerRequest *request)> 


class wifi_handling
{
    private: 
        rgb_handling rgb;
        wifi_authentications auth;
        String translateEncryptionType(wifi_auth_mode_t encryptionType);

    public:
        wifi_handling();
        bool get_mode();
        void set_mode(const char* mode);
        void client_mode (WIFI_RESET_CALLBACK);
        void access_mode(NOT_FOUND callback);
        void trying_connection(void);
        void scan_avaiable_wifis(void);
        void get_auth(wifi_authentications auth);
        bool is_connected(void);
        String local_ip(void);
        
};