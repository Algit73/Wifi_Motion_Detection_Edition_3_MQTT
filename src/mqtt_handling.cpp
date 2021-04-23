#include "mqtt_handling.h"
#include <ArduinoJson.h>



mqtt_handling::mqtt_handling(PubSubClient &client_mqtt)
{
  this->client_mqtt = client_mqtt;
}

void mqtt_handling::start(const char* host, const int port,MQTT_CALLBACK_SIGNATURE)
{
  client_mqtt.setServer(host, port);
  client_mqtt.setCallback(callback);
}


void mqtt_handling::reconnect() 
{
  // Loop until we're reconnected
  while (!client_mqtt.connected()) 
  {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client_mqtt.connect(MQTT_ID)) 
    {
      Serial.println("connected");
      // Subscribe
      client_mqtt.subscribe(MQTT_SUBSCRIBE_COMMAND);
      client_mqtt.subscribe(MQTT_SUB_COM_SET_REC);
      client_mqtt.subscribe(MQTT_SUB_COM_SET_ALARM);
      client_mqtt.subscribe(MQTT_SUB_COM_SET_BEACON);
      client_mqtt.subscribe(MQTT_SUB_COM_SET_MON);
      client_mqtt.subscribe(MQTT_SUB_COM_SET_LIGHT);
      client_mqtt.subscribe(MQTT_SUB_COM_SET_RES);
      client_mqtt.subscribe(MQTT_SUB_COM_SET_BUZZER);
      client_mqtt.subscribe(MQTT_SUB_COM_SET_CONT);

      //client_mqtt.loop();
      //client_mqtt.publish(MQTT_SERVICE_STATUS,"Device Connected");
    } 
    else 
    {
      Serial.print("failed, rc=");
      Serial.print(client_mqtt.state());
      Serial.println(" try again in 2 seconds");
      // Wait 2 seconds before retrying
      delay(2000);
    }
  }

}

void mqtt_handling::check_connection()
{
    if (!client_mqtt.connected()) 
      {
        reconnect();
      }
}

void mqtt_handling::loop()
{
    check_connection();
    client_mqtt.loop();
}

void mqtt_handling::publish_command(const char* key, const char* value)
{
    loop();
    Serial.print(key);
    Serial.print(": ");
    Serial.println(value);
    client_mqtt.publish(key,value);
}

void mqtt_handling::publish_status(const char* status, system_status single_status)
{
  //loop();
  publish_command(MQTT_SERVICE_STATUS,status);

  publish_command(MQTT_PUB_STAT_IS_RES,String(single_status.camera_resolution).c_str());
  publish_command(MQTT_PUB_STAT_IS_ALARM,String(single_status.is_alarm_set).c_str());
  publish_command(MQTT_PUB_STAT_IS_BUZZER,String(single_status.is_buzzer_set).c_str());
  publish_command(MQTT_PUB_STAT_IS_BEACON,String(single_status.is_hazard_beacon_set).c_str());
  publish_command(MQTT_PUB_STAT_IS_LIGHT,String(single_status.is_light_set).c_str());
  publish_command(MQTT_PUB_STAT_IS_MON,String(single_status.is_monitoring_set).c_str());
  publish_command(MQTT_PUB_STAT_IS_REC,String(single_status.is_recording_set).c_str());
  publish_command(MQTT_PUB_STAT_IS_CONT,String(single_status.is_sending_status_continous_set).c_str());

}

bool mqtt_handling::send_photo() 
{
  loop();

  camera_fb_t * fb = NULL;
  fb = esp_camera_fb_get();

    
  if (fb->len) // send only images with size >0
  {
    //if (client_mqtt.beginPublish("test_loc/esp32-cam/pic_p1", fb->len + sizeof(long), false))
    if (client_mqtt.beginPublish(MQTT_PUB_IMAGE, fb->len + sizeof(long), false))
    {
      // send image data + millis()
      unsigned long m = millis();  

      client_mqtt.write(fb->buf, fb->len);
      client_mqtt.write((byte *) &m, sizeof(long));
      
      if (!client_mqtt.endPublish())
      {
        // error!
        Serial.println(F("\nError sending data."));
      }
      Serial.println(F("\nImage has been sent"));
    }
  }
  esp_camera_fb_return(fb);

  return true;
}


void mqtt_handling::send_photo_RT(uint8_t * buf,size_t len) 
{
    
  loop();
  if (len) // send only images with size >0
  {
    //if (client_mqtt.beginPublish("test_loc/esp32-cam/pic_p1", len + sizeof(long), false))
    if (client_mqtt.beginPublish(MQTT_PUB_IMAGE, len + sizeof(long), false))
    {
      // send image data + millis()
      unsigned long m = millis();  

      client_mqtt.write(buf, len);
      client_mqtt.write((byte *) &m, sizeof(long));
      if (!client_mqtt.endPublish())
      {
        // error!
        Serial.println(F("\nError sending data."));
      }
      Serial.println(F("\nImage has been sent"));
    }
  }

  
}


void mqtt_handling::set_callback(MQTT_CALLBACK_SIGNATURE) 
{
  client_mqtt.setCallback(callback);
  //this->callback = callback;
  //return ;
}




