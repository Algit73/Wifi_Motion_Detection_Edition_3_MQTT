#include "Arduino.h"
#include "pins_arduino.h"
#include <PubSubClient.h>
#include "esp_camera.h"
#include "Client.h"
#include "Stream.h"
#include <WiFi.h>
#include <camera_handling.h>

#define MQTT_ID                     "HuHoScSy"
#define MQTT_DEVICE_ID              "utd_n1/"
#define MQTT_SERVICE_STATUS         MQTT_DEVICE_ID "system_status"
#define MQTT_SUBSCRIBE_COMMAND      MQTT_DEVICE_ID "system_command"
#define MQTT_SUBSCRIBE_SEND_STATUS  MQTT_DEVICE_ID "send_status"

#define MQTT_PUB_IMAGE              MQTT_DEVICE_ID "camera_image"

#define MQTT_SUB_COM_SET_REC        MQTT_DEVICE_ID "set_recording"
#define MQTT_SUB_COM_SET_ALARM      MQTT_DEVICE_ID "set_alarm"
#define MQTT_SUB_COM_SET_BEACON     MQTT_DEVICE_ID "set_beacon"
#define MQTT_SUB_COM_SET_MON        MQTT_DEVICE_ID "set_monitoring"
#define MQTT_SUB_COM_SET_LIGHT      MQTT_DEVICE_ID "set_light"
#define MQTT_SUB_COM_SET_RES        MQTT_DEVICE_ID "set_resolution"
#define MQTT_SUB_COM_SET_BUZZER     MQTT_DEVICE_ID "set_buzzer"
#define MQTT_SUB_COM_SET_CONT       MQTT_DEVICE_ID "set_continous"

#define MQTT_PUB_STAT_IS_REC        MQTT_DEVICE_ID "is_recording"
#define MQTT_PUB_STAT_IS_ALARM      MQTT_DEVICE_ID "is_alarm"
#define MQTT_PUB_STAT_IS_BEACON     MQTT_DEVICE_ID "is_beacon"
#define MQTT_PUB_STAT_IS_MON        MQTT_DEVICE_ID "is_monitoring"
#define MQTT_PUB_STAT_IS_LIGHT      MQTT_DEVICE_ID "is_light"
#define MQTT_PUB_STAT_IS_RES        MQTT_DEVICE_ID "is_resolution"
#define MQTT_PUB_STAT_IS_BUZZER     MQTT_DEVICE_ID "is_buzzer"
#define MQTT_PUB_STAT_IS_CONT       MQTT_DEVICE_ID "is_continous"

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
};



#include <functional>
#define MQTT_CALLBACK std::function<void(char*, uint8_t*, unsigned int)> callback



class mqtt_handling
{
    
    private:

        PubSubClient client_mqtt;
        String mqtt_token;
        String pub_image;
        String pub_move;
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
        void start(const char* host, const int port, MQTT_CALLBACK);
        void publish_command(const char* Key, const char* Value);
        void publish_status(const char* status, system_status single_status);
        void disconnect(void);
        void loop();
        void set_callback(MQTT_CALLBACK);
        bool send_photo ();
        void send_photo_RT(uint8_t * buf,size_t len);
        void get_status(String topic, String message, system_status &status, camera_handling &camera);
        mqtt_sub get_sub_service();
        mqtt_pub get_pub_service();
};