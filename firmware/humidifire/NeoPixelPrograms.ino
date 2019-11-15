/* Collection of NeoPixel animation and helper functions
 * Moved to seperate file to clean up setup{} section of main code file
 * 
 * Includes:
 *  - colorWipe
 *  - larsonScanner
 *  - rainbow
 *  - rainbowCycle
 *  - theaterChase
 *  - theaterChaseRainbow
 *  - Wheel
 *  - hsb2rgb
 *  - flame
 * 
 */

// Fill the dots one after the other with a color
void colorWipe(uint32_t c, uint8_t wait) 
{
  for(uint16_t i=0; i<pixels.numPixels(); i++) {
      pixels.setPixelColor(i, c);
      pixels.show();
      delay(wait);
  }
} // END colorWipe()


int pos = 0, dir = 1; // Position, direction of "eye" for larson scanner animation
void larsonScanner(uint32_t c, uint8_t wait)
{
   int j;
   
   for(uint16_t i=0; i<pixels.numPixels()+5; i++) 
   {
      // Draw 5 pixelss centered on pos.  setPixelColor() will clip any
      // pixelss off the ends of the strip, we don't need to watch for that.
      pixels.setPixelColor(pos - 2, 0x003b85); // Dark red
      pixels.setPixelColor(pos - 1, 0x005ed2); // Medium red
      pixels.setPixelColor(pos , 0x00c0ff); // Center pixels is brightest
      pixels.setPixelColor(pos + 1, 0x005ed2); // Medium red
      pixels.setPixelColor(pos + 2, 0x003b85); // Dark red
         
      pixels.show();
      delay(wait);
     
      // Rather than being sneaky and erasing just the tail pixels,
      // it's easier to erase it all and draw a new one next time.
      for(j=-2; j<= 2; j++) pixels.setPixelColor(pos+j, 0);
     
      // Bounce off ends of strip
      pos += dir;
      if(pos < 0) 
      {
        pos = 1;
        dir = -dir;
      } 
      else if(pos >= pixels.numPixels()) 
      {
        pos = pixels.numPixels() - 2;
        dir = -dir;
      } 
   } 
 //colorWipe(pixels.Color(0, 0, 0), 20);
 
} // END larsonScanner()





void rainbow(uint8_t wait) 
{
  uint16_t i, j;

  for(j=0; j<256; j++) {
    for(i=0; i<pixels.numPixels(); i++) {
      pixels.setPixelColor(i, Wheel((i+j) & 255));
    }
    pixels.show();
    delay(wait);
  }
  
} // END rainbow()


// Slightly different, this makes the rainbow equally distributed throughout
void rainbowCycle(uint8_t wait) 
{
  uint16_t i, j;

  for(j=0; j<256*5; j++) { // 5 cycles of all colors on wheel
    for(i=0; i< pixels.numPixels(); i++) {
      pixels.setPixelColor(i, Wheel(((i * 256 / pixels.numPixels()) + j) & 255));
    }
    pixels.show();
    delay(wait);
  }
  
} // END rainbowCycle()


//Theatre-style crawling lights
void theaterChase(uint32_t c, uint8_t wait) 
{
  for (int j=0; j<10; j++) {  //do 10 cycles of chasing
    for (int q=0; q < 3; q++) {
      for (int i=0; i < pixels.numPixels(); i=i+3) {
        pixels.setPixelColor(i+q, c);    //turn every third pixels on
      }
      pixels.show();

      delay(wait);

      for (int i=0; i < pixels.numPixels(); i=i+3) {
        pixels.setPixelColor(i+q, 0);        //turn every third pixels off
      }
    }
  }
} // END theaterChase()


//Theatre-style crawling lights with rainbow effect
void theaterChaseRainbow(uint8_t wait) 
{
  for (int j=0; j < 256; j++) {     // cycle all 256 colors in the wheel
    for (int q=0; q < 3; q++) {
      for (int i=0; i < pixels.numPixels(); i=i+3) {
        pixels.setPixelColor(i+q, Wheel( (i+j) % 255));    //turn every third pixels on
      }
      pixels.show();

      delay(wait);

      for (int i=0; i < pixels.numPixels(); i=i+3) {
        pixels.setPixelColor(i+q, 0);        //turn every third pixels off
      }
    }
  }
} // END theaterChaseRainbow()

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) 
{
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return pixels.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return pixels.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return pixels.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  
} // END Wheel()

/******************************************************************************
 hsb2rgb()
 Convert Hue, Saturation, Brightness to Red, Green, Blue values
 
 https://blog.adafruit.com/2012/03/14/constant-brightness-hsb-to-rgb-algorithm/
 
 * accepts hue, saturation and brightness values and outputs three 8-bit color
 * values in an array (color[])
 *
 * saturation (sat) and brightness (bright) are 8-bit values.
 *
 * hue (index) is a value between 0 and 767. hue values out of range are
 * rendered as 0.
 *
 *****************************************************************************/
//Use color names as index values for color[]
#define RED     1
#define GREEN   2
#define BLUE    3

//Hue Value Color Map from Adafruit
//https://blog.adafruit.com/2012/03/14/constant-brightness-hsb-to-rgb-algorithm/
#define RED_VAL   0
#define BLUE_VAL  512

//Initialize array to hold RED, GREEN, BLUE color values from hsb2rgb() routine
uint8_t color[3]; 

void hsb2rgb(uint16_t index, uint8_t sat, uint8_t bright, uint8_t color[3])
{
  uint16_t r_temp, g_temp, b_temp;
  uint8_t index_mod;
  uint8_t inverse_sat = (sat ^ 255);

  index = index % 768;
  index_mod = index % 256;

  if (index < 256)
  {
    r_temp = index_mod ^ 255;
    g_temp = index_mod;
    b_temp = 0;
  }

  else if (index < 512)
  {
    r_temp = 0;
    g_temp = index_mod ^ 255;
    b_temp = index_mod;
  }

  else if ( index < 768)
  {
    r_temp = index_mod;
    g_temp = 0;
    b_temp = index_mod ^ 255;
  }

  else
  {
    r_temp = 0;
    g_temp = 0;
    b_temp = 0;
  }

  r_temp = ((r_temp * sat) / 255) + inverse_sat;
  g_temp = ((g_temp * sat) / 255) + inverse_sat;
  b_temp = ((b_temp * sat) / 255) + inverse_sat;

  r_temp = (r_temp * bright) / 255;
  g_temp = (g_temp * bright) / 255;
  b_temp = (b_temp * bright) / 255;

  color[RED]  = (uint8_t)r_temp;
  color[GREEN]  = (uint8_t)g_temp;
  color[BLUE] = (uint8_t)b_temp;
  
} // END hsb2rgb()

void flame()
{

  //int r = 200,  g = 160,  b = 40;     // Yellow flame
  int r = 226,  g = 121,  b = 35;     // Orange flame
  //int r = 158,  g = 8,    b = 148;    // Purple flame 
  //int r = 74,   g = 150,  b = 12;     // Green flame
  
  for(int x = 0; x < pixels.numPixels(); x++)
  {
    //int flicker = random(0,150);
    int flicker = random(0,55);
    int r1 = r-flicker;
    int g1 = g-flicker;
    int b1 = b-flicker;
    if(g1<0) g1=0;
    if(r1<0) r1=0;
    if(b1<0) b1=0;
    pixels.setPixelColor(x, r1, g1, b1);
    pixels.show();
    
    delay(random(0,15));
  }
  
} // END flame()

