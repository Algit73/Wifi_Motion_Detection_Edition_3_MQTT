#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif
#include "pins_arduino.h"

#if CONFIG_FREERTOS_UNICORE
#define ARDUINO_RUNNING_CORE 0
#else
#define ARDUINO_RUNNING_CORE 1
#endif







// Tasks and Functions' Signatures
void task_peripherals_handling( void *pvParameters );
void task_wifi_communication_service( void *pvParameters );
void task_capturing_real_time( void *pvParameters );
void task_siren_alarm( void *pvParameters );
void task_motion_counter( void *pvParameters );
void wifi_client_mode (void);
void wifi_access_mode (void);
void wifi_trying_to_connect(void);
void calling_offline_threads(void);
void wifi_receiving_command(const char *,size_t);
void check_wifi_reset_mode (void);
void mqtt_callback(char* topic, byte* message, unsigned int length); 
String get_status(void);

const char* PARAM_INPUT_1 = "ssid";
const char* PARAM_INPUT_2 = "password";
// HTML web page to handle 3 input fields (input1, input2, input3)
const char wifiConfigurationIndexPage [] PROGMEM = "<!DOCTYPE html> <html lang='fa' dir='rtl'> <head> <meta charset='utf-8'><title>هوشیار | تنظیمات شبکه</title> <meta name='viewport' content='width=device-width, initial-scale=1'></head> <body> <form action='/get'> <legend> نام شبکه و رمز ورود به شبکه داخلی خود را وارد کنید. </legend> <fieldset> <ul> <li> <label for='txtssid'> نام شبکه: </label> <input id = 'txtssid' type='text' name='ssid'> </li> <li> <label for='txtpwd'> رمز شبکه: </label> <input id = 'txtpwd' type='text' name='password'> </li> </ul> <input type='submit' value='ذخیره'> </fieldset> </form><br> </body> </html>";
const char wifiConfigurationSucceedPage [] PROGMEM = "<!DOCTYPE html><html lang='fa' dir='rtl'><head><meta charset='utf-8'><title>هوشیار | تنظیمات شبکه</title><meta name='viewport' content='width=device-width, initial-scale=1'></head><body><h1>تبریک</h1><p>در صورت صحیح بودن اطلاعات اتصال به شبکه ی داخلی،دستگاه شما تا لحضات دیگر به شبکه متصل خواهد شد...</p></body></html>";


#define GPIO_OUTPUT_PIN_SEL  ((1ULL<<LED_BUILTIN) | (1ULL<<SIREN_PIN))

// Variables
#define TIME_TO_CALL_OFF    20000
#define cycle_to_alarm    20000

// Post Configs
#define POST_HEAD   "--SendingImage\r\nContent-Disposition: form-data; name=\"imageFile\"; filename=\"esp32-cam.jpg\"\r\nContent-Type: image/jpeg\r\n\r\n"
#define POST_TAIL   "\r\n--SendingImage--\r\n"








// Defining check points
#define SETUP_IOS                    100
#define OFFLINE_THREADS_INITIATED    101
#define WIFI_ACCESS_MODE             102
#define WIFI_CLIENT_MODE             103
#define WIFI_ACCESS_MODE_INITIATED   104
#define WIFI_CLIENT_MODE_INITIATED   105
#define CAMERA_INITIATED             106
#define WIFI_CONNECTION_INITIATED    107
#define RESET_MODE_INITIATED         108
#define TASK_RGB_DONE                109
#define TASK_PIR_DONE                110
#define TASK_IMAGE_POSTING_DONE      111
#define TASK_COMMUNICATION_DONE      112
#define DESERIALLIZATION_DONE        113
#define CAPTURING_DONE               114
#define SEND_IMAGE_DONE              115

#define CHECK_BEGIN "0"
#define CHECK_DONE "1"

