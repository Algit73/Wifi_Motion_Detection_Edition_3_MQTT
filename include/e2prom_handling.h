// Flash storage size
#include "WString.h"

#define EEPROM_SIZE 120 // The maximum amount of using flash storage
#define ACCESS_MODE "0"   // To set the device in the Access mode for confiuration
#define CLIENT_MODE "1"   // To the set the device in Client mode for functioning


struct wifi_authentications     // A struct to handle SSID and Password
{
    String ssid;
    String password;
};

class e2prom_handling
{
    private:
        void init();

    public:
        void write(const char* string, int index_end);
        String read(int index_begin, int index_end);
        wifi_authentications wifi_auth_read(void);
        void wifi_mode_write (const char* mode);
        bool wifi_mode_read(void);
};