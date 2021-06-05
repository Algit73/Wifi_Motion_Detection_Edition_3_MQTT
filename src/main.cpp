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

#define DEBUG_MODE



// Controling parameters
bool volatile reset_activated = false; // A global variable to stop functionalities when user pushes the reset button
bool volatile pir_trigged = false;   // A global variable to siganl if the PIR trigged
bool offline_mode = true;   // A global variable to set the mode of device into offline
bool volatile realtime_capturing_activated = false;
bool volatile ready_to_send = false;
int motion_counter = 0;   // A global counter to count the number of times PIR trigged

/// Tasks Handlers
static TaskHandle_t reset_task = NULL;
static TaskHandle_t real_time_capturing_task = NULL;
static TaskHandle_t wifi_communication_task = NULL;
static TaskHandle_t motion_counter_task = NULL;


//camera_config_t config;
camera_handling camera;


void not_found(AsyncWebServerRequest *request) 
{
  request->send(404, "text/plain", "Not found");
}

static void IRAM_ATTR detectsMovement(void * arg)
{
  #ifdef DEBUG_MODE
  Serial.println(F("Interrupt"));
  #endif 

  if(!peripheral.setup_pin_state())
    //check_wifi_reset_mode();
    xTaskCreatePinnedToCore(
      task_reset_device
      ,  "Reset Device"
      ,  1024*2  // Stack size
      ,  NULL
      ,  5  // Priority
      ,  &reset_task 
      ,  ARDUINO_RUNNING_CORE);
}

void serial_debug(String string)
{
  #ifdef DEBUG_MODE
  Serial.println(string);
  #endif
}

template <typename T>
void printf_debug(String string, T msg)
{
  #ifdef DEBUG_MODE
  Serial.printf(string.c_str(), msg);
  #endif
}



// the setup function runs once when you press reset or power the board
void setup() 
{

  peripheral.setup();

  #ifdef DEBUG_MODE
  Serial.println(F("Run"));
  #endif
  
  // Finding out the operational mode
  if(e2prom.wifi_mode_read())
    wifi_client_mode();
  else
    wifi_access_mode();

  #ifdef DEBUG_MODE
  Serial.println(F("OK"));
  #endif
  
  
}

void calling_offline_threads()
{
  #ifdef DEBUG_MODE
  Serial.println(F("Offline Threads Begin"));
  #endif
  //*
  xTaskCreatePinnedToCore(
    task_peripherals_handling
    ,  "RGB and lighting"   // A name just for humans
    ,  4096  // This stack size can be checked & adjusted by reading the Stack Highwater
    ,  NULL
    ,  3  // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
    ,  NULL 
    ,  ARDUINO_RUNNING_CORE);
  //*/
  xTaskCreatePinnedToCore(
    task_motion_counter
    ,  "Counting the detection"
    ,  1024*4  // Stack size
    ,  NULL
    ,  3  // Priority
    ,  &motion_counter_task 
    ,  ARDUINO_RUNNING_CORE);

    
    xTaskCreatePinnedToCore(
      task_buzzer_alarm
      ,  "Buzzer Controller"
      ,  1024*4  // Stack size
      ,  NULL
      ,  3  // Priority
      ,  NULL 
      ,  ARDUINO_RUNNING_CORE);
      

    #ifdef DEBUG_MODE
    Serial.println(F("Offline Threads End"));
    #endif

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
      request->send_P(200, "text/html", html_page);
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
    //Serial.println(SSID+"\n"+Password);
    serial_debug(SSID+"\n"+Password);
    request->send(200, "text/html", "HTTP GET request sent to your ESP on input field (" 
                                     + inputParam +""+inputParam2+ ") with value: " + SSID +" and "+ Password+ 
                                     "<br><a href=\"/\">Return to Home Page</a>");
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

  bool err = gpio_isr_handler_add(GPIO_NUM_13, &detectsMovement, (void *) 13);
  if (err != ESP_OK)
    //Serial.printf("handler add failed with error 0x%x \r\n", err);
    printf_debug("handler add failed with error 0x%x \r\n", err);

  err = gpio_set_intr_type(GPIO_NUM_13, GPIO_INTR_NEGEDGE);
  if (err != ESP_OK)
    //Serial.printf("set intr type failed with error 0x%x \r\n", err);
    printf_debug("set intr type failed with error 0x%x \r\n", err);
  

  xTaskCreatePinnedToCore(
    task_wifi_communication_service
    ,  "Request from the server"
    ,  1024*4  // Stack size
    ,  NULL
    ,  5  // Priority
    ,  &wifi_communication_task 
    ,  ARDUINO_RUNNING_CORE);

  xTaskCreatePinnedToCore(
    task_capturing_real_time
    ,  "Request from the server"
    ,  1024*6  // Stack size
    ,  NULL
    ,  5  // Priority
    ,  &real_time_capturing_task 
    ,  ARDUINO_RUNNING_CORE);

  #ifdef DEBUG_MODE
  Serial.println(F("Threads Added"));
  #endif
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
  vTaskDelete(NULL);
}

//////////////////////////////////////////////////////
/*..................................................*/
//////////////// Tasks definitions ///////////////////
/*..................................................*/
//////////////////////////////////////////////////////
void task_reset_device(void *pvParameters)  
{
  (void) pvParameters;
  #ifdef DEBUG_MODE
  Serial.println(F("Reset Task Created"));
  #endif

  check_wifi_reset_mode();
  vTaskDelete(NULL);

}

void task_peripherals_handling(void *pvParameters)  
{
  (void) pvParameters;


  for (;;)
  {
    if(status.is_alarm_set&&status.is_hazard_beacon_set)
        rgb.siren_and_alarm();

    else if(pir_trigged)
    {
      long cycle_time = xTaskGetTickCount()+cycle_to_alarm;
      while(cycle_time>xTaskGetTickCount())
        if(status.is_hazard_beacon_set)
          rgb.siren_and_alarm();

      rgb.on_off(OFF);
      vTaskDelay(1);
      
    }
    else
    {
      rgb.on_off(ON);
      vTaskDelay(100);
    }

  }
}

void task_buzzer_alarm(void *pvParameters)  
{
  (void) pvParameters;

  for (;;)
  {
    /// If user acticate the alarm
    if(status.is_alarm_set&&status.is_buzzer_set)
        peripheral.buzzer_cycle();

    /// If PIR being trigged
    else if(pir_trigged)
    {
      long cycle_time = xTaskGetTickCount()+cycle_to_alarm;
      while(cycle_time>xTaskGetTickCount())
        if(status.is_buzzer_set)
          peripheral.buzzer_cycle();
      vTaskDelay(1);
    }
    /// No event happens
    else
    {
      peripheral.buzzer_off();
      vTaskDelay(200);
    }
  }
  vTaskDelete(NULL);
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
    peripheral.builtin_led_reverse(); // Built-In LED Blinking
    vTaskDelay(100);
  }
}


void task_wifi_communication_service(void *pvParameters)  // This is a task.
{
  (void) pvParameters;
  vTaskDelay(1000);
  WiFi.scanNetworks();
  while(true)
  {
    if(wifi.is_connected())
    { 

      if(!realtime_capturing_activated)
      {
        if(status.is_sending_status_continous_set)
        {
          mqtt.publish_status(get_status().c_str(),status);
          //Serial.println("");
        }

        // Sending images on demand
        if(status.is_recording_set)
          mqtt.send_photo();
        
        mqtt.loop();
        vTaskDelay(100);
      }
      
      else
      {
        if(ready_to_send)//&&!reset_activated)
        {
          for(int i=0;i<30;i++)
          {
            //if(reset_activated)
             // break;
            if(status.is_sending_status_continous_set)
              mqtt.publish_status(get_status().c_str(),status);
            mqtt.send_photo_RT(image_holder[i],image_size_holder[i]);
            free(image_holder[i]);
          }
          //for(int i=0;i<30;i++)
            //free(image_holder[i]);
          ready_to_send = false;
        }
        vTaskDelay(500);
      }
      

    }
    else
    {
      Serial.println(F("WiFi Disconnected"));
      offline_mode = true;
      //wifi_trying_to_connect();
      WiFi.reconnect();
      while (WiFi.status() != WL_CONNECTED)
      {
        /* code */
      }
      
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
      //pinMode(SETUP_PIN, OUTPUT);
      for(int i=0;i<30;i++)
      {
        camera_fb_t * fb = camera.capture();
        //Serial.printf(". Image Num: %d",i+1);
        printf_debug("\nImage Num: %d",i+1);
        image_size_holder[i] = fb->len;
        image_holder[i] = (uint8_t*)ps_malloc(fb->len);
        std::copy(fb->buf, fb->buf + fb->len, image_holder[i]);
        camera.free_resource(fb);
        if(i>1)
          ready_to_send = true;
        vTaskDelay(500);
      }
      //pinMode(SETUP_PIN, INPUT);
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
  #ifdef DEBUG_MODE
  Serial.print("On Command Arrived: ");
  Serial.print(topic);
  Serial.println(". Message: ");
  Serial.println();
  #endif

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
  {
    status.is_sending_status_continous_set = message_string.toInt();
    mqtt.publish_command(MQTT_PUB_STAT_IS_CONT,message_string.c_str());
  }
  else if(topic_string.equals(MQTT_SUB_COM_SET_BEACON))
  {
    status.is_hazard_beacon_set = message_string.toInt();
    mqtt.publish_command(MQTT_PUB_STAT_IS_BEACON,message_string.c_str());
  }
  else if(topic_string.equals(MQTT_SUB_COM_SET_BUZZER))
  {
    status.is_buzzer_set = message_string.toInt();
    mqtt.publish_command(MQTT_PUB_STAT_IS_BUZZER,message_string.c_str());
  }
  else if(topic_string.equals(MQTT_SUB_COM_SET_ALARM))
  {
    status.is_alarm_set = message_string.toInt();
    mqtt.publish_command(MQTT_PUB_STAT_IS_ALARM,message_string.c_str());
  }
  else if(topic_string.equals(MQTT_SUB_COM_SET_LIGHT))
  {
    status.is_light_set = message_string.toInt();
    mqtt.publish_command(MQTT_PUB_STAT_IS_LIGHT,message_string.c_str());
  }
  else if(topic_string.equals(MQTT_SUB_COM_SET_MON))
  {
    status.is_monitoring_set = message_string.toInt();
    mqtt.publish_command(MQTT_PUB_STAT_IS_MON,message_string.c_str());
  }
  else if(topic_string.equals(MQTT_SUB_COM_SET_REC))
  {
    status.is_recording_set = message_string.toInt();
    mqtt.publish_command(MQTT_PUB_STAT_IS_REC,message_string.c_str());
  }
  else if(topic_string.equals(MQTT_SUB_COM_SET_RES))
  {
    status.camera_resolution = message_string.toInt();
    camera.change_resolution(status.camera_resolution);
    mqtt.publish_command(MQTT_PUB_STAT_IS_RES,message_string.c_str());
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
  if(!peripheral.setup_pin_state())
    while(!peripheral.setup_pin_state())  // This Ensures the Stability of wifi
    { 
      vTaskSuspend(real_time_capturing_task);
      vTaskSuspend(wifi_communication_task);
      vTaskSuspend(motion_counter_task);

      peripheral.builtin_led_on();
      long timer_counter = xTaskGetTickCount()+3000; // Three seconds pushing

      #ifdef DEBUG_MODE
      Serial.println(F("Pushed"));
      #endif

      while(!peripheral.setup_pin_state())
      {
        if(xTaskGetTickCount()>timer_counter)
        {
          #ifdef DEBUG_MODE
          Serial.println(F("Reset Activated"));
          #endif
          /*
          WiFi.disconnect();  // To be sure that wifi tasks will not interfere
          e2prom.wifi_mode_write (ACCESS_MODE);
          delay(100);
          esp_camera_deinit();
          delay(100);
          ESP.restart();
          */
        }
        delay(500);
      }

      vTaskResume(real_time_capturing_task);
      vTaskResume(wifi_communication_task);
      vTaskResume(motion_counter_task);
      //reset_activated = false;  // Enables GET and POST
    }
  
}