#include "Arduino.h"
#include "pins_arduino.h"
#include <PubSubClient.h>
#include "esp_camera.h"
#include "Client.h"
#include "Stream.h"
#include <WiFi.h>
#include <camera_handling.h>


#define MQTT_DEVICE_ID              "e351aac7-e0af-4418-8671-2b598944dbe2"
#define MQTT_USER_NAME              "=0eGDWR0cCS3"
#define MQTT_PASSWORD               "nld-y1=Zj5y4"
#define MQTT_SUB_SET_TOKEN          "HSHYR_" MQTT_DEVICE_ID "/setToken"
#define MQTT_PUB_SET_TOKEN          "HSHYR_register"


#define MQTT_SERVICE_STATUS         MQTT_DEVICE_ID "system_status"
#define MQTT_SUBSCRIBE_COMMAND      MQTT_DEVICE_ID "system_command"
#define MQTT_SUBSCRIBE_SEND_STATUS  MQTT_DEVICE_ID "send_status"



#define PIR_MOVE_DETECTED       "1"
#define PIR_MOVE_NOT_DETECTED   "0"

#define MONITORING_STATUS       0
#define RECORDING_STATUS        1        
#define BEACON_STATUS           2
#define BUZZER_STATUS           3
#define ALARM_STATUS            4
#define RESOLUTION_STATUS       5
#define CONTINOUS_STATUS        6
#define ALL_STATUS              7

struct system_status
{
    bool is_monitoring_set = false;
    bool is_recording_set = false;
    bool is_hazard_beacon_set = false;
    bool is_buzzer_set = false;
    bool is_light_set = false; 
    bool is_sending_status_continous_set = false; 
    bool is_alarm_set = false;
    int camera_resolution = 8;
    String token;
};

struct mqtt_pub
{
    String monitoring;
    String recording;
    String beacon;
    String buzzer;
    String alarm;
    String resolution;
    String continous;
    String status;
    String token;
    String image;
    String move;
};

struct mqtt_sub
{
    String monitoring;
    String recording;
    String beacon;
    String buzzer;
    String alarm;
    String resolution;
    String continous;
    String token;
    String status;
};



#include <functional>
#define MQTT_CALLBACK std::function<void(char*, uint8_t*, unsigned int)> callback



class mqtt_handling
{
    
    private:

        PubSubClient client_mqtt;
        bool is_token_received = false;
        String mqtt_token;
        String mqtt_user;
        String mqtt_password;
        mqtt_pub pub_service;
        mqtt_sub sub_service;
        void init(void);
        String get_status();
        void check_connection();
        void reconnect();
        void subscribe();
        void unsubscribe();
        

    public:

        mqtt_handling(PubSubClient &client_mqtt);
        void start(const char* host, const int port, const char* user, const char* password, MQTT_CALLBACK);
        void publish_command(const char* Key, const char* Value);
        void publish_status(const char* status, system_status single_status);
        void publish_single_status(int index, system_status single_status);
        void disconnect(void);
        void loop();
        bool token_received();
        void set_callback(MQTT_CALLBACK);
        bool send_photo ();
        void send_photo_RT(uint8_t * buf,size_t len);
        void get_status(String topic, String message, system_status &status, camera_handling &camera);
        mqtt_sub get_sub_service();
        mqtt_pub get_pub_service();
};