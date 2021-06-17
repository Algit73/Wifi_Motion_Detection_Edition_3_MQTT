#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "esp_camera.h"
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include <main.h>
#include "driver/gpio.h"
#include <PubSubClient.h>
#include "mqtt_handling.h"
#include "wifi_handling.h"
#include "peripheral_handling.h"



// MQTT Configs
const char* mqtt_host = "broker.hivemq.com";
//const char* mqtt_host = "broker.emqx.io";
const int mqtt_port = 1883;


wifi_handling wifi;

// Wifi and MQTT initialization
WiFiClient client_http;   // A client for HTTP
PubSubClient client_mqtt(client_http);
mqtt_handling mqtt(client_mqtt);
system_status status;

// Image Holder for Sending Through MQTT
uint8_t* image_holder[30];
size_t image_size_holder[30];

// Defining Handlers;
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
bool volatile is_tasks_defined = false;
int motion_counter = 0;   // A global counter to count the number of times PIR trigged

/// Tasks Handlers
static TaskHandle_t reset_task = NULL;
static TaskHandle_t real_time_capturing_task = NULL;
static TaskHandle_t wifi_communication_task = NULL;
static TaskHandle_t motion_counter_task = NULL;


//camera_config_t config;
camera_handling camera;


void on_page_not_found(AsyncWebServerRequest *request) 
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
  
  /// Finding out the operational mode
  if(wifi.get_mode())
    wifi_client_mode();
  else
    wifi.access_mode(on_page_not_found);

  #ifdef DEBUG_MODE
  Serial.println(F("OK"));
  #endif
  
}


/// Wifi client mode
void wifi_client_mode ()
{
  /// Adding offline threads: Motion Detection and Siren
  is_tasks_defined = false;
  calling_offline_threads();
  
  /// Start client mode
  start_client_mode();
  
  camera.camera_init(FRAMESIZE_XGA);
  delay(10);

  bool err = gpio_isr_handler_add(GPIO_NUM_13, &detectsMovement, (void *) 13);
  if (err != ESP_OK)
    printf_debug("handler add failed with error 0x%x \r\n", err);

  err = gpio_set_intr_type(GPIO_NUM_13, GPIO_INTR_NEGEDGE);
  if (err != ESP_OK)
    printf_debug("set intr type failed with error 0x%x \r\n", err);
  
  /// Defining online tasks
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

    xTaskCreatePinnedToCore(
    task_check_mqtt_token
    ,  "Request from the server"
    ,  1024  // Stack size
    ,  NULL
    ,  3  // Priority
    ,  &real_time_capturing_task 
    ,  ARDUINO_RUNNING_CORE);

  #ifdef DEBUG_MODE
  Serial.println(F("Threads Added"));
  #endif
  is_tasks_defined = true;
  rgb.wifi_connected();
}

/// Put device in client-mode
void start_client_mode()
{
  wifi.client_mode(check_wifi_reset_mode);
  wifi.local_ip();
  offline_mode = false;
  /// Establishing MQTT Connection
  mqtt.start(mqtt_host, mqtt_port,mqtt_callback);
  //mqtt.publish_status(get_status().c_str(),status);
  mqtt.publish_command(MQTT_PUB_SET_TOKEN,MQTT_DEVICE_ID);

}

/// Defining offline tasks
void calling_offline_threads()
{
  #ifdef DEBUG_MODE
  Serial.println(F("Offline Threads Begin"));
  #endif
  
  xTaskCreatePinnedToCore(
    task_rgb_handling
    ,  "RGB and lighting"   // A name just for humans
    ,  4096  // This stack size can be checked & adjusted by reading the Stack Highwater
    ,  NULL
    ,  3  // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
    ,  NULL 
    ,  ARDUINO_RUNNING_CORE);
  
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

/// Check if user want to put the device in the access mode (to change credntials)
void task_reset_device(void *pvParameters)  
{
  (void) pvParameters;
  #ifdef DEBUG_MODE
  Serial.println(F("Reset Task Created"));
  #endif

  check_wifi_reset_mode();
  vTaskDelete(NULL);
}

/// Defining multiple RGB actions for different scenarios 
void task_rgb_handling(void *pvParameters)  
{
  (void) pvParameters;

  for (;;)
  {
    
    //// Simple fading alarm
    /// User contrlled mode
    if(status.is_alarm_set&&status.is_hazard_beacon_set)
        rgb.siren_and_alarm();

    /// PIR trigged mode
    else if(pir_trigged)
    {
      Serial.println(F("RGB PIR trigged"));
      long cycle_time = xTaskGetTickCount() + ALARM_LENGTH; 
      while(cycle_time>xTaskGetTickCount())
        if(status.is_hazard_beacon_set)
          rgb.siren_and_alarm();

      rgb.on_off(OFF);
      vTaskDelay(1);
    }
    

    /// Default
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
    
    //// Simple Buzzer alarm mode
    /// User controlled mode
    if(status.is_alarm_set&&status.is_buzzer_set)
        peripheral.buzzer_cycle();

    
    /// PIR trigged mode
    else if(pir_trigged)
    {
      Serial.println(F("BUZ PIR trigged"));
      long cycle_time = xTaskGetTickCount() + ALARM_LENGTH;
      while(cycle_time>xTaskGetTickCount())
        if(status.is_buzzer_set)
          peripheral.buzzer_cycle();
      vTaskDelay(1);
    }
    
    /// Default
    else
    {
      peripheral.buzzer_off();
      vTaskDelay(100);
    }
    
   //vTaskDelay(50);
  }
  vTaskDelete(NULL);
}

/// PIR motion detection task
void task_motion_counter(void *pvParameters)  
{
  (void) pvParameters;

  for (;;)
  {
    /// Online mode, by default
    if(!offline_mode)
    {
      /// User controlled
      if(peripheral.pir_state()&&status.is_monitoring_set)
      {
        pir_trigged = true;
        motion_counter++; /// Global variable to count the number of detections
        Serial.println("Event Counted: "+String(motion_counter));
        mqtt.publish_command(mqtt.get_pub_service().move.c_str()
                              ,PIR_MOVE_DETECTED);
        while(peripheral.pir_state())
        {
          Serial.println(F("Event Loop:"));
          vTaskDelay(500);
        }
        mqtt.publish_command(mqtt.get_pub_service().move.c_str()
                              ,PIR_MOVE_NOT_DETECTED);
        pir_trigged = false;
        
      }
    }
    /// Offline mode
    else
    {
      if(peripheral.pir_state()&&is_tasks_defined)
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

/// Double checking if token has been received correctly
void task_check_mqtt_token(void *pvParameters)
{
  (void) pvParameters;
  vTaskDelay(3000);
  Serial.println(F("Receiving Token"));
  while(!mqtt.token_received())
  {
    mqtt.publish_command(MQTT_PUB_SET_TOKEN,MQTT_DEVICE_ID);
    vTaskDelay(2000);
  }
  Serial.println(F("Token Received"));
  vTaskDelete(NULL);
}


void task_wifi_communication_service(void *pvParameters)  // This is a task.
{
  (void) pvParameters;
  
  Serial.println("Scan done");
  Serial.println("");
  while(true)
  {
    if(wifi.is_connected())
    { 
      if(!realtime_capturing_activated)
      {
        if(status.is_sending_status_continous_set)
        //{mqtt.publish_status(get_status().c_str(),status);}
        {mqtt.publish_all_status(status);}

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
          for(int i=0;i<IMAGES_MAX_NUM;i++)
          {
            if(status.is_sending_status_continous_set)
              //mqtt.publish_status(get_status().c_str(),status);
              mqtt.publish_all_status(status);
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
      //wifi_trying_to_connect();
      ///TODO: needs modifications
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

      for(int i=0;i<IMAGES_MAX_NUM;i++)
      {
        camera_fb_t * fb = camera.capture();
        printf_debug("\nImage Num: %d",i+1);
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

  mqtt.get_status(topic_string, message_string, status, camera);
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
  //Serial.println(F("check_wifi_reset_mode"));
  if(!peripheral.setup_pin_state())
    while(!peripheral.setup_pin_state())  // This Ensures the Stability of wifi
    { 
      if(is_tasks_defined)
      {
        vTaskSuspend(real_time_capturing_task);
        vTaskSuspend(wifi_communication_task);
        vTaskSuspend(motion_counter_task);
      }

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
          
          WiFi.disconnect();  // To be sure that wifi tasks will not interfere
          wifi.set_mode(ACCESS_MODE);
          delay(100);
          esp_camera_deinit();
          delay(100);
          ESP.restart();
        }
        delay(500);
      }
      if(is_tasks_defined)
      {
        vTaskResume(real_time_capturing_task);
        vTaskResume(wifi_communication_task);
        vTaskResume(motion_counter_task);
      }
    }
}
