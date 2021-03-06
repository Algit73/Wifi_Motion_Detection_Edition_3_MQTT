#include "rgb_handling.h" 


rgb_handling::rgb_handling()
{
  pixels.begin(); // This initializes the NeoPixel library.
}

void rgb_handling::connecting_to_wifi(int index,int count)
{
  for (int pixel_value=0;pixel_value<256;pixel_value++)
  {
    pixels.setPixelColor(index, pixels.Color((count%3)>1?pixel_value:0,(count%3)>0?pixel_value:0,pixel_value));
    pixels.show();
    delay(1);
  }
}

void rgb_handling::wifi_connected()
{
  for (int i=0;i<3;i++)
  {
    for (int pixel_value=0;pixel_value<256;pixel_value++)
    {
      for (int index=0;index<NUMPIXELS;index++)
        pixels.setPixelColor(index, pixels.Color(pixel_value,pixel_value,pixel_value));
      pixels.show();
      delay(3);
    }
    for (int pixel_value=255;pixel_value>0;pixel_value--)
    {
      for (int index=0;index<NUMPIXELS;index++)
        pixels.setPixelColor(index, pixels.Color(pixel_value,pixel_value,pixel_value));
      pixels.show();
      delay(3);
    }
  }

  on_off(true);
}

void rgb_handling::error()
{
  for(int i=0;i<4;i++)
  {
    for (int index=0;index<NUMPIXELS;index++)
        pixels.setPixelColor(index, pixels.Color(30,0,0));
    pixels.show();
    delay(125);
    for (int index=0;index<NUMPIXELS;index++)
        pixels.setPixelColor(index, pixels.Color(0,0,0));
    pixels.show();
    delay(125);

  }
}

void rgb_handling::on_off(bool on)
{
  if(on)
  {
    for (int index=0;index<NUMPIXELS;index++)
      pixels.setPixelColor(index, pixels.Color(30,30,30));
    pixels.show();
  }
  else
  {
    for (int index=0;index<NUMPIXELS;index++)
      pixels.setPixelColor(index, pixels.Color(0,0,0));
    pixels.show();
  }
}

void rgb_handling::siren_alarm()
{
  for (int index=0;index<NUMPIXELS;index++)
  {
    pixels.setPixelColor(index, pixels.Color(0,0,0));
    pixels.setPixelColor((index+1)%8, pixels.Color(250,0,0));
    pixels.setPixelColor((index+2)%8, pixels.Color(250,0,0));
    pixels.setPixelColor((index+3)%8, pixels.Color(250,0,0));
    pixels.show();
    delay(100);
  }
  //pixels.show();
}


void rgb_handling::siren_and_alarm()
{
  //for (int i=0;i<3;i++)
  //{
    for (int pixel_value=10;pixel_value<200;pixel_value++)
    {

      for (int index=0;index<NUMPIXELS;index++)
        pixels.setPixelColor(index, pixels.Color(pixel_value,10,10));
      pixels.show();
      
      vTaskDelay(3);
    }
    for (int pixel_value=200;pixel_value>10;pixel_value--)
    {
        for (int index=0;index<NUMPIXELS;index++)
          pixels.setPixelColor(index, pixels.Color(pixel_value,10,10));
        pixels.show();

      vTaskDelay(3);
    }
  //}

  //on_off(true);
}
void rgb_handling::reset_mode()
{
  //for (int i=0;i<3;i++)
  //{
    for (int pixel_value=10;pixel_value<200;pixel_value++)
    {

      for (int index=0;index<NUMPIXELS;index++)
        pixels.setPixelColor(index, pixels.Color(10,pixel_value,10));
      pixels.show();
      
      vTaskDelay(3);
    }
    for (int pixel_value=200;pixel_value>10;pixel_value--)
    {
        for (int index=0;index<NUMPIXELS;index++)
          pixels.setPixelColor(index, pixels.Color(10,pixel_value,10));
        pixels.show();

      vTaskDelay(3);
    }
  //}

  //on_off(true);
}


