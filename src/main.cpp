#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "esp_camera.h"

#include <ArduinoJson.h>


#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

#include <main.h>
#include "driver/gpio.h"
#include <PubSubClient.h>
#include "mqtt_handling.h"
#include "wifi_handling.h"
#include "camera_handling.h"
#include "peripheral_handling.h"



AsyncWebServer server(80);


// MQTT Configs
const char* mqtt_host = "broker.hivemq.com";
//const char* mqtt_host = "broker.emqx.io";
const int mqtt_port = 1883;


//WiFiClientSecure client; // A client for HTTPS

wifi_handling wifi;

// Wifi and MQTT initialization
WiFiClient client_http;   // A client for HTTP
PubSubClient client_mqtt(client_http);
mqtt_handling mqtt(client_mqtt);
system_status status;

// Image Holder for Sending Through MQTT
uint8_t* image_holder[30];
size_t image_size_holder[30];

// Initializing EEPROM
e2prom_handling e2prom;

//Handling rgb functions;
rgb_handling rgb;

peripheral_handling peripheral;



/// JSON Doc for Receving Command and Sending Device's Status
StaticJsonDocument<250> doc_status;

// A just in case expression: can be removed
// Choosing the Core to Run FreeRTOS
#if CONFIG_FREERTOS_UNICORE
#define ARDUINO_RUNNING_CORE 0
#else
#define ARDUINO_RUNNING_CORE 1
#endif



// Controling parameters
bool reset_activated = false; // A global variable to stop functionalities when user pushes the reset button
bool pir_trigged = false;   // A global variable to siganl if the PIR trigged
bool offline_mode = true;   // A global variable to set the mode of device into offline
bool realtime_capturing_activated = false;
bool ready_to_send = false;
int motion_counter = 0;   // A global counter to count the number of times PIR trigged


//camera_config_t config;
camera_handling camera;


void not_found(AsyncWebServerRequest *request) 
{
  request->send(404, "text/plain", "Not found");
}



// the setup function runs once when you press reset or power the board
void setup() 
{

  peripheral.setup();
  Serial.println(F("Run"));
  
  // Finding out the operational mode
  if(e2prom.wifi_mode_read())
    wifi_client_mode();
  else
    wifi_access_mode();
  
}

void calling_offline_threads()
{
  xTaskCreatePinnedToCore(
    task_peripherals_handling
    ,  "RGB and lighting"   // A name just for humans
    ,  4096  // This stack size can be checked & adjusted by reading the Stack Highwater
    ,  NULL
    ,  3  // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
    ,  NULL 
    ,  ARDUINO_RUNNING_CORE);

  xTaskCreatePinnedToCore(
    task_motion_counter
    ,  "Counting the detection"
    ,  2048  // Stack size
    ,  NULL
    ,  3  // Priority
    ,  NULL 
    ,  ARDUINO_RUNNING_CORE);

}


///////// Wifi Modes /////////

// Wifi acts as an access point
void wifi_access_mode()
{
  // Put Wifi into Access mode
  wifi.access_mode();
  // Send web page with input fields to client
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
  {
      request->send_P(200, "text/html", wifiConfigurationIndexPage);
  });

  // Send a GET request to <ESP_IP>/get?input1=<inputMessage>
  server.on("/get", HTTP_GET, [] (AsyncWebServerRequest *request) 
  {
    String inputParam,inputParam2;
    String SSID,Password;
    // GET input1 value on <ESP_IP>/get?input1=<inputMessage>
    if (request->hasParam(PARAM_INPUT_1)) 
    {
      SSID = request->getParam(PARAM_INPUT_1)->value();
      Password = request->getParam(PARAM_INPUT_2)->value();
      inputParam = PARAM_INPUT_1;
      inputParam2 = PARAM_INPUT_2;

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
    Serial.println(SSID+"\n"+Password);
    
    request->send_P(200, "text/html", wifiConfigurationSucceedPage);
    delay(3000); // Wait to settle system
    ESP.restart();  // Automtically restarts system into client mode
    
  });

  
  
  server.onNotFound(not_found);
  server.begin();
  
}

// Wifi acts as a client

void wifi_client_mode ()
{
  // Adding offline threads: Motion Detection and Siren
  calling_offline_threads();

  // Connecting to the WIFI Network
  wifi.get_auth(e2prom.wifi_auth_read());
  wifi_trying_to_connect();

  camera.camera_init(FRAMESIZE_XGA);
  delay(10);

  
  xTaskCreatePinnedToCore(
    task_wifi_communication_service
    ,  "Request from the server"
    ,  1024*4  // Stack size
    ,  NULL
    ,  5  // Priority
    ,  NULL 
    ,  ARDUINO_RUNNING_CORE);


  xTaskCreatePinnedToCore(
    task_capturing_real_time
    ,  "Request from the server"
    ,  1024*6  // Stack size
    ,  NULL
    ,  5  // Priority
    ,  NULL 
    ,  ARDUINO_RUNNING_CORE);
  //*/  
  
  Serial.println(F("Threads Added"));
  rgb.wifi_connected();
}


// Connecting to the local hotspot
void wifi_trying_to_connect()
{
  
  wifi.client_mode(check_wifi_reset_mode);
  wifi.local_ip();
  offline_mode = false;
  // Establishing MQTT Connection
  mqtt.start(mqtt_host, mqtt_port,mqtt_callback);
  mqtt.publish_status(get_status().c_str(),status);
  
}

// Reset to Go to Access Mode
void loop()
{
  check_wifi_reset_mode();
  peripheral.builtin_led_reverse(); // Built-In LED Blinking
  vTaskDelay(200);
}

//////////////////////////////////////////////////////
/*..................................................*/
//////////////// Tasks definitions ///////////////////
/*..................................................*/
//////////////////////////////////////////////////////


void task_peripherals_handling(void *pvParameters)  
{
  (void) pvParameters;


  for (;;)
  {

    if(status.is_alarm_set)
    {

      if(status.is_light_set)
        peripheral.light_on();
      else
        peripheral.light_off();

      if(status.is_buzzer_set)
      {
        if(status.is_hazard_beacon_set)
          rgb.siren_and_alarm(ALARM_AND_SIREN);
        else
          rgb.siren_and_alarm(ALARM_ONLY);
      }
        
      else
      {
        if(status.is_hazard_beacon_set)
          rgb.siren_and_alarm(SIREN_ONLY); 
        ledcWrite(3,0);
      }

        
    }
    else
    {

      ledcWrite(3,0);
      rgb.on_off(ON);
    }
    
    if(pir_trigged)
    {
      
      long cycle_time = xTaskGetTickCount()+cycle_to_alarm;
      while(cycle_time>xTaskGetTickCount())
      {
        if(status.is_light_set)
          peripheral.light_on();

        if(status.is_buzzer_set)
        {
          if(status.is_hazard_beacon_set)
            rgb.siren_and_alarm(ALARM_AND_SIREN);
          else
            rgb.siren_and_alarm(ALARM_ONLY);
        }
        else
        {
          if(status.is_hazard_beacon_set)
            rgb.siren_and_alarm(SIREN_ONLY); 
        }
        vTaskDelay(1);

      }

      peripheral.light_off();
      peripheral.buzzer_off();
      ledcWrite(3,0);
      rgb.on_off(OFF);
      
    }
    else
    
      vTaskDelay(100);
  
  }
}

// Counting the number of detection event
void task_motion_counter(void *pvParameters)  
{
  (void) pvParameters;

  for (;;)
  {
    if(!offline_mode)
    {
      if(peripheral.pir_state()&&status.is_monitoring_set)
      {
        pir_trigged = true;
        motion_counter++;
        Serial.println("Event Counted: "+String(motion_counter));
        while(peripheral.pir_state())
        {
          Serial.println(F("Event Loop:"));
          vTaskDelay(500);
        }
        pir_trigged = false;
        
      }
    }
    else
    {
      if(peripheral.pir_state())
      {
        pir_trigged = true;
        while(peripheral.pir_state());
        pir_trigged = false;
      }
    }

    vTaskDelay(100);
  }
}


void task_wifi_communication_service(void *pvParameters)  // This is a task.
{
  (void) pvParameters;
  vTaskDelay(1000);
  while(true)
  {
    if(wifi.is_connected())
    { 

      if(!realtime_capturing_activated)
      {
        if(status.is_sending_status_continous_set)
        {
          mqtt.publish_status(get_status().c_str(),status);
          Serial.println("");
        }

        // Sending images on demand
        if(status.is_recording_set)
          mqtt.send_photo();
        
        mqtt.loop();
        vTaskDelay(100);
      }
      
      else
      {
        if(ready_to_send)
        {
          for(int i=0;i<30;i++)
          {
            if(status.is_sending_status_continous_set)
              mqtt.publish_status(get_status().c_str(),status);
            mqtt.send_photo_RT(image_holder[i],image_size_holder[i]);
            free(image_holder[i]);
          }
          ready_to_send = false;
        }
        vTaskDelay(500);
      }
      

    }
    else
    {
      Serial.println(F("WiFi Disconnected"));
      offline_mode = true;
      wifi_trying_to_connect();
      vTaskDelay(100);
    }
    
  }
}

void task_capturing_real_time(void *pvParameters) 
{
  (void) pvParameters;

  for (;;)
  {
    
    if(pir_trigged&&!ready_to_send&&!status.is_recording_set)
    {
      realtime_capturing_activated = true;
      for(int i=0;i<30;i++)
      {
        camera_fb_t * fb = camera.capture();
        Serial.printf(". Image Num: %d",i+1);
        image_size_holder[i] = fb->len;
        image_holder[i] = (uint8_t*)ps_malloc(fb->len);
        std::copy(fb->buf, fb->buf + fb->len, image_holder[i]);
        camera.free_resource(fb);
        if(i>1)
          ready_to_send = true;
        vTaskDelay(500);
      }
    }
    else
    {
      realtime_capturing_activated = false;
      vTaskDelay(1000);
    }
  }
}


//////////////////////////////////////////////////////
/*..................................................*/
////////////// Functions definitions /////////////////
/*..................................................*/
//////////////////////////////////////////////////////


///////////////////////////////////////////////////
///            Handling MQTT Callback           ///
///////////////////////////////////////////////////

void mqtt_callback(char* topic, byte* message, unsigned int length) 
{
  Serial.print("On Command Arrived: ");
  Serial.print(topic);
  Serial.println(". Message: ");
  Serial.println();

  // Concatenating received chars
  String message_string;
  String topic_string = topic;

  for (int i = 0; i < length; i++) 
  {
    Serial.print((char)message[i]);
    message_string += (char)message[i];
  }
  Serial.println();

  if(message_string.equals(MQTT_SUBSCRIBE_SEND_STATUS))
  {
    mqtt.publish_status(get_status().c_str(),status);
    return;
  }

  if(topic_string.equals(MQTT_SUB_COM_SET_CONT))
    status.is_sending_status_continous_set = message_string.toInt();//logic; 
  else if(topic_string.equals(MQTT_SUB_COM_SET_BEACON))
    status.is_hazard_beacon_set = message_string.toInt();//logic;
  else if(topic_string.equals(MQTT_SUB_COM_SET_BUZZER))
    status.is_buzzer_set = message_string.toInt();//logic;
  else if(topic_string.equals(MQTT_SUB_COM_SET_ALARM))
    status.is_alarm_set = message_string.toInt();//logic;
  else if(topic_string.equals(MQTT_SUB_COM_SET_LIGHT))
    status.is_light_set = message_string.toInt();//logic;
  else if(topic_string.equals(MQTT_SUB_COM_SET_MON))
    status.is_monitoring_set = message_string.toInt();//logic;
  else if(topic_string.equals(MQTT_SUB_COM_SET_REC))
    status.is_recording_set = message_string.toInt();//logic;
  else if(topic_string.equals(MQTT_SUB_COM_SET_RES))
  {
    status.camera_resolution = message_string.toInt();
    camera.change_resolution(status.camera_resolution);
  }

}

///////////////////////////////////////////////////
//               Get System Status               //
///////////////////////////////////////////////////

String get_status()
{
  doc_status["isRecording"] = status.is_recording_set;
  doc_status["isMonitoring"] = status.is_monitoring_set;
  doc_status["isHazardBeaconSet"] = status.is_hazard_beacon_set;
  doc_status["isLightSet"] = status.is_light_set;
  doc_status["isAlarmSet"] = status.is_alarm_set;
  doc_status["isBuzzerSet"] = status.is_buzzer_set;
  doc_status["isStatusContinous"] = status.is_sending_status_continous_set;
  doc_status["camRes"] = status.camera_resolution;

  String status_string;
  serializeJson(doc_status,status_string);
  return status_string;
}

///////////////////////////////////////////////////
// Capturing photos in the specified time cycle ///
///////////////////////////////////////////////////

// Check if user wants to reset the wifi
void check_wifi_reset_mode()
{
  while(!peripheral.setup_pin_state())  // This Ensures the Stability of wifi
  {
    //digitalWrite(LED_BUILTIN,LOW);
    peripheral.builtin_led_on();
    long timer_counter = xTaskGetTickCount()+3000; // Three seconds pushing
    Serial.println(F("Pushed"));
    
    reset_activated = true;   // Disables GET and POST while entering the mode
    while(!peripheral.setup_pin_state())
    {
      if(xTaskGetTickCount()>timer_counter)
      {
        Serial.println("Reset Activated");
        WiFi.disconnect();  // To be sure that wifi tasks will not interfere
        e2prom.wifi_mode_write (ACCESS_MODE);
        delay(100);
        esp_camera_deinit();
        delay(100);
        ESP.restart();

      }
      
      delay(500);
    }
    reset_activated = false;  // Enables GET and POST
  }

}