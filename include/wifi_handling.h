#include "Arduino.h"
#include <WiFi.h>
#include "e2prom_handling.h"
#include "rgb_handling.h"

#define ACESS_MODE_SSID     "Hushyar_Conf"
#define ACESS_MODE_PASS     "123456789"

#include <functional>
#define WIFI_RESET_CALLBACK std::function<void()> callback


class wifi_handling
{
    private: 
        rgb_handling rgb;
        wifi_authentications auth;
        
        


    public:
        wifi_handling();
        void client_mode (WIFI_RESET_CALLBACK);
        void access_mode(void);
        void trying_connection(void);
        void get_auth(wifi_authentications auth);
        bool is_connected(void);
        String local_ip(void);
        



};