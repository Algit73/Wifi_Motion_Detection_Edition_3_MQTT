#include "camera_handling.h"

camera_handling::camera_handling()
{
  
}

void camera_handling::camera_init(framesize_t quality)
{
  Serial.println("Cam Config");
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 15000000;
  config.pixel_format = PIXFORMAT_JPEG;

    if(psramFound())
  {
    Serial.println("PSRAM found");
    config.frame_size = quality;
    config.jpeg_quality = 15;  //0-63 lower number means higher quality
    config.fb_count = 2;
  } 
  else 
  {
    config.frame_size = FRAMESIZE_CIF;
    config.jpeg_quality = 12;  //0-63 lower number means higher quality
    config.fb_count = 1;
  }
  
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) 
  {
    Serial.printf("Camera init failed with error 0x%x", err);
    delay(100);
    esp_camera_deinit();
    delay(100);
    ESP.restart();
  }
}

bool camera_handling::change_resolution(int index)
{
  framesize_t quality;

  switch (index)
  {
    case 0:  quality = FRAMESIZE_QQVGA;   break;
    case 1:  quality = FRAMESIZE_QQVGA2;  break;
    case 2:  quality = FRAMESIZE_QCIF;    break;
    case 3:  quality = FRAMESIZE_HQVGA;   break;
    case 4:  quality = FRAMESIZE_QVGA;    break;
    case 5:  quality = FRAMESIZE_CIF;     break;
    case 6:  quality = FRAMESIZE_VGA;     break;
    case 7:  quality = FRAMESIZE_SVGA;    break;
    case 8:  quality = FRAMESIZE_XGA;     break;
    
    default: quality = FRAMESIZE_XGA;     break;
  }
  
  sensor_t* sensor = esp_camera_sensor_get();
  if (sensor == nullptr) 
  {
    return false;
  }
  sensor->set_quality(sensor,20);
  if(sensor->set_framesize(sensor,quality)!=0)
    return false;
  else
    return true;
}

bool camera_handling::quality(int quality)
{
  sensor_t* sensor = esp_camera_sensor_get();
  if (sensor == nullptr) 
  {
    return false;
  }
  
  if(sensor->set_quality(sensor,quality)!=0)
    return false;
  else
    return true;
}

void camera_handling::camera_deinit()
{
  esp_camera_deinit();
}

camera_fb_t* camera_handling::capture()
{
  return esp_camera_fb_get();
}

void camera_handling::free_resource(camera_fb_t* fb)
{
  esp_camera_fb_return(fb);
}