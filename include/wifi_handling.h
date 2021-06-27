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
<!DOCTYPE html>
<html lang="en" dir="rtl">
  <head>
    <meta charset="utf-8">
    <title>هوشیار | تنظیمات شبکه</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
      body{font-family:Tahoma;}
      input[type=text]{font-family:Tahoma;font-size:18px;}
      input[type=submit]{font-family:Tahoma;font-size:18px;display: block;margin-left: auto;margin-right: auto; background:linear-gradient(270deg,#0080ff,#0f61fd);cursor: pointer;box-shadow:0 3px 5px 0 #39f;width: 100%;border-radius: 4px;color: #fff;border: 0;height:40px}
      footer{font-size:10px;}
      img{display: block;margin-left: auto;margin-right: auto;}
      li{margin:0 10px 10px 0; width:20px;}
      hr{margin:10px 0 30px 0;}
    </style>
  </head>
  <body>
    <img src="data:image/png;base64,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"/>
    <hr>
    <form action="/get">
      <legend>
        اطلاعات اتصال به مودم خود را وارد کنید
      </legend>
      <ul>
        <li>
          <label for="ssid">
            SSID:
          </label>
          <input type="text" name="ssid" id="ssid" autocorrect="off" autocapitalize="none">
        </li>
        <li>
          <label for="password">
            Password:
          </label>
          <input type="text" name="password" id="password" autocorrect="off" autocapitalize="none">
        </li>
      </ul>
      <input type="submit" value="ذخیره">
    </form>
    <hr>
    <footer>
      هوشیار | سیسیتم یکپارچه پایش امنیت محیط
    </footer>
  </body>
</html>
)rawliteral";


const char SUCCESS_PAGE_HTML [] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en" dir="rtl">
  <head>
    <meta charset="utf-8">
    <title>هوشیار | تنظیمات شبکه</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
      body{font-family:Tahoma;}
      footer{font-size:10px;}
      h1{font-size:18px;}
      img{display: block;margin-left: auto;margin-right: auto;}
      a{text-decoration: none;font-family:Tahoma;font-size:18px;display: block;margin-left: auto;margin-right: auto;text-align:center; background:linear-gradient(270deg,#0080ff,#0f61fd);cursor: pointer;box-shadow:0 3px 5px 0 #39f;width: 100%;border-radius: 4px;color: #fff;border: 0;height:40px}
      hr{margin:10px 0 30px 0;}
      main{margin:0 10px 0 10px;}
    </style>
  </head>
  <body>
    <hr>
    <main>
      <h1>
        اطلاعات شما، با موفقیت ذخیره شد
      </h1>
      <p>
        در صورت صحیح بودن اطلاعات، هوشیار به شبکه ی داخلی شما متصل خواهد شد.
      </p>
      <p>
        شما میتوانید با ورود به لینک زیر، مدیریت هوشیار را در دست داشته باشید
      </p>
      <a href="https://panel.hushyaar.ir/">
        ورود به مدیریت هوشیار
      </a>
    </main>

    <hr>
    <footer>
      هوشیار | سیسیتم یکپارچه پایش امنیت محیط
    </footer>
  </body>
</html>
)rawliteral";




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