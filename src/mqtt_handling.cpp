#include "mqtt_handling.h"
#include <ArduinoJson.h>
#include <e2prom_handling.h>


e2prom_handling e2_prom;


mqtt_handling::mqtt_handling(PubSubClient &client_mqtt)
{
  this->client_mqtt = client_mqtt;
}

void mqtt_handling::start(const char* host, const int port,MQTT_CALLBACK_SIGNATURE)
{
  client_mqtt.setServer(host, port);
  client_mqtt.setCallback(callback);
  init();
  
}

void mqtt_handling::init()
{
  mqtt_token = e2_prom.read(80,85);
  String sub_string = mqtt_token + "/sub/";
  String pub_string = mqtt_token + "/pub/";

  sub_service.alarm = sub_string + "alarm";
  sub_service.beacon = sub_string + "beacon";
  sub_service.buzzer = sub_string + "buzzer";
  sub_service.continous = sub_string + "continous";
  sub_service.monitoring = sub_string + "monitoring";
  sub_service.recording = sub_string + "recording";
  sub_service.resolution = sub_string + "resolution";
  sub_service.token = sub_string + "token";

  pub_service.alarm = pub_string + "alarm";
  pub_service.beacon = pub_string + "beacon";
  pub_service.buzzer = pub_string + "buzzer";
  pub_service.continous = pub_string + "continous";
  pub_service.monitoring = pub_string + "monitoring";
  pub_service.recording = pub_string + "recording";
  pub_service.resolution = pub_string + "resolution";
  pub_service.status = pub_string + "status";
  pub_service.token = pub_string + "token";
  pub_image = pub_string + "image";
  pub_move = pub_string + "move";
}


void mqtt_handling::reconnect() 
{
  // Loop until we're reconnected
  while (!client_mqtt.connected()) 
  {
    if(WiFi.status() != WL_CONNECTED)
      break;

    Serial.print(F("Attempting MQTT connection..."));
    // Attempt to connect
    if (client_mqtt.connect(MQTT_ID)) 
    {
      Serial.println(F("connected"));
      // Subscribes
      subscribe();
    } 
    else 
    {
      Serial.print(F("failed, rc="));
      Serial.print(client_mqtt.state());
      Serial.println(F(" try again in 2 seconds"));
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
    //Serial.print(key);
    //Serial.print(": ");
    //Serial.println(value);
    client_mqtt.publish(key,value);
}

void mqtt_handling::publish_status(const char* status, system_status single_status)
{
  //loop();
  publish_command(pub_service.status.c_str(),status);
  publish_command(pub_service.resolution.c_str(),String(single_status.camera_resolution).c_str());
  publish_command(pub_service.alarm.c_str(),String(single_status.is_alarm_set).c_str());
  publish_command(pub_service.buzzer.c_str(),String(single_status.is_buzzer_set).c_str());
  publish_command(pub_service.beacon.c_str(),String(single_status.is_hazard_beacon_set).c_str());
  publish_command(pub_service.monitoring.c_str(),String(single_status.is_monitoring_set).c_str());
  publish_command(pub_service.recording.c_str(),String(single_status.is_recording_set).c_str());
  publish_command(pub_service.continous.c_str(),String(single_status.is_sending_status_continous_set).c_str());
  publish_command(pub_service.token.c_str(),mqtt_token.c_str());
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
}

mqtt_sub mqtt_handling::get_sub_service() 
{
  return sub_service;
}

mqtt_pub mqtt_handling::get_pub_service() 
{
  return pub_service;
}

void mqtt_handling::disconnect() 
{
  client_mqtt.disconnect();
}

void mqtt_handling::get_status(String topic, String message, system_status &status, camera_handling &camera)
{
  if(topic.equals(sub_service.continous))
  {
    status.is_sending_status_continous_set = message.toInt();
    publish_command(pub_service.continous.c_str()
                          ,message.c_str());
  }
  else if(topic.equals(sub_service.beacon))
  {
    status.is_hazard_beacon_set = message.toInt();
    publish_command(pub_service.beacon.c_str()
                          ,message.c_str());
  }
  else if(topic.equals(sub_service.buzzer))
  {
    status.is_buzzer_set = message.toInt();
    publish_command(pub_service.buzzer.c_str()
                          ,message.c_str());
  }
  else if(topic.equals(sub_service.alarm))
  {
    status.is_alarm_set = message.toInt();
    publish_command(pub_service.alarm.c_str()
                          ,message.c_str());
  }
  else if(topic.equals(sub_service.monitoring))
  {
    status.is_monitoring_set = message.toInt();
    publish_command(pub_service.monitoring.c_str()
                          ,message.c_str());
  }
  else if(topic.equals(sub_service.recording))
  {
    status.is_recording_set = message.toInt();
    publish_command(pub_service.recording.c_str()
                          ,message.c_str());
  }

  else if(topic.equals(sub_service.token))
  {
    e2_prom.write(message.c_str(),80);
    status.token = message;
    publish_command(pub_service.token.c_str()
                          ,message.c_str());

    unsubscribe();
    disconnect();
    init();
    reconnect();
  }

  else if(topic.equals(sub_service.resolution))
  {
      status.camera_resolution = message.toInt();
      camera.change_resolution(status.camera_resolution);
      publish_command(pub_service.resolution.c_str()
                            ,message.c_str());
  }
  
}


void mqtt_handling::subscribe()
{
  client_mqtt.subscribe(sub_service.recording.c_str());
  client_mqtt.subscribe(sub_service.alarm.c_str());
  client_mqtt.subscribe(sub_service.beacon.c_str());
  client_mqtt.subscribe(sub_service.monitoring.c_str());
  client_mqtt.subscribe(sub_service.resolution.c_str());
  client_mqtt.subscribe(sub_service.buzzer.c_str());
  client_mqtt.subscribe(sub_service.continous.c_str());
  client_mqtt.subscribe(sub_service.token.c_str());
}

void mqtt_handling::unsubscribe()
{
  client_mqtt.unsubscribe(sub_service.alarm.c_str());
  client_mqtt.unsubscribe(sub_service.beacon.c_str());
  client_mqtt.unsubscribe(sub_service.buzzer.c_str());
  client_mqtt.unsubscribe(sub_service.continous.c_str());
  client_mqtt.unsubscribe(sub_service.monitoring.c_str());
  client_mqtt.unsubscribe(sub_service.recording.c_str());
  client_mqtt.unsubscribe(sub_service.resolution.c_str());
  client_mqtt.unsubscribe(sub_service.token.c_str());
}