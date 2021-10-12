/*
1.) Add a "static" and "rotate" button at the top of the "gradient picker" section.
2.) Get the Number of LEDs input to work. (Not Easy)

4.) Add the RGB Rain effect.
5.) Add effects from "FastLED Basics Episode 6 - Noise" video.
6.) Change the "ripple" effect.
7.) Add a "speed" slider to the "Function Control" section.
8.) Add a new section for users to pick their own gradient blend.
9.) Work out how to choose whether to add the controller to an Internet-enabled Router, or act as LocalHost.
10.) Design and print an injection-mouldable controller housing.

*/
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////  INCLUDE FILES  //////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiClient.h>
#include <WebSocketsServer.h>
#include <FS.h>
#include <ArduinoJson.h>
#include "FastLED.h"
#include <sys/time.h>
using namespace std;
#include <vector>


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////  OBJECT DEFINITIONS  ///////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

ESP8266WebServer server; // NEW SHIT(80)
WebSocketsServer webSocket = WebSocketsServer(81);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////  VARIABLE DECLARATIONS  //////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef APSSID
#define APSSID "RGB_LED_Controller"
#define APPSK "LEDwise123"
#endif
char* ssid = APSSID;
char* password = APPSK;
//IPAddress ip(192,168,1,81);         // NEW SHIT
//IPAddress subnet(255,255,255,0);    // NEW SHIT


#define DATA_PIN    2
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB
#define NUM_LEDS    60
#define BRIGHTNESS  75

#define UPDATES_PER_SECOND 100

CRGB leds[NUM_LEDS];
CRGB comet_LEDs[NUM_LEDS];

uint8_t paletteIndex = 0;

#define SECONDS_PER_PALETTE 10

#define COOLING  55
#define SPARKING 120

#define ARRAYSIZE(x) (sizeof(x)/sizeof(x[0]))   // Count elements in a static array

static const CRGB ballColors [] =
{
    CRGB::Green,
    CRGB::Red,
    CRGB::Blue,
    CRGB::Orange,
    CRGB::Indigo
};

int num_LEDs = 1;
int ID = 0;
int func_tion = 0;
int red = 20;
int green = 20;
int blue = 20;
int stat_us = 0;

int red_last = 20;
int green_last = 20;
int blue_last = 20;

int loop_counter;
long loop_timer_now;
long previous_millis;
float loop_time;
int loop_test_times = 20000;
uint16_t brightness = 50;
uint8_t thishue = 0;

//FOR RIPPLES//
uint8_t colour;                                               // Ripple colour is randomized.
int center = 0;                                               // Center of the current ripple.
int step = -1;                                                // -1 is the initializing step.
uint8_t myfade = 255;                                         // Starting brightness.
#define maxsteps 16                                           // Case statement wouldn't allow a variable.
uint8_t bgcol = 0;                                            // Background colour rotates.
int thisdelay = 100;  

uint8_t colorIndex[NUM_LEDS];

uint8_t brightnessScale = 150;
uint8_t indexScale = 50;


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////  CLASSES  /////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class BouncingBallEffect
{
  private:

    double InitialBallSpeed(double height) const
    {
        return sqrt(-2 * Gravity * height);         // Because MATH!
    }

    size_t  _cLength;           
    size_t  _cBalls;
    byte    _fadeRate;
    bool    _bMirrored;

    const double Gravity = -9.81;                   // Because PHYSICS!
    const double StartHeight = 1;                   // Drop balls from max height initially
    const double ImpactVelocity = InitialBallSpeed(StartHeight);
    const double SpeedKnob = 4.0;                   // Higher values will slow the effect

    vector<double> ClockTimeAtLastBounce, Height, BallSpeed, Dampening;
    vector<CRGB>   Colors;

    static double Time()
    {
        timeval tv = { 0 };
        gettimeofday(&tv, nullptr);
        return (double)(tv.tv_usec / 1000000.0 + (double) tv.tv_sec);
    }
    
  public:

    // BouncingBallEffect
    //
    // Caller specs strip length, number of balls, persistence level (255 is least), and whether the
    // balls should be drawn mirrored from each side.

    BouncingBallEffect(size_t cLength, size_t ballCount = 3, byte fade = 0, bool bMirrored = false)
        : _cLength(cLength - 1),
          _cBalls(ballCount),
          _fadeRate(fade),
          _bMirrored(bMirrored),
          ClockTimeAtLastBounce(ballCount),
          Height(ballCount),
          BallSpeed(ballCount),
          Dampening(ballCount),
          Colors(ballCount)
    {
        for (size_t i = 0; i < ballCount; i++)
        {
            Height[i]                = StartHeight;         // Current Ball Height
            ClockTimeAtLastBounce[i] = Time();              // When ball last hit ground state
            Dampening[i]             = 0.90 - i / pow(_cBalls, 2);  // Bounciness of this ball
            BallSpeed[i]             = InitialBallSpeed(Height[i]); // Don't dampen initial launch
            Colors[i]                = ballColors[i % ARRAYSIZE(ballColors) ];
        }
    }

    // Draw
    //
    // Draw each of the balls.  When any ball settles with too little energy, it it "kicked" to restart it

    virtual void Draw()
    {
        if (_fadeRate != 0)
        {
            for (size_t i = 0; i < _cLength; i++)
                leds[i].fadeToBlackBy(_fadeRate);
        }
        else
            FastLED.clear();
        
        // Draw each of the balls

        for (size_t i = 0; i < _cBalls; i++)
        {
            double TimeSinceLastBounce = (Time() - ClockTimeAtLastBounce[i]) / SpeedKnob;

            // Use standard constant acceleration function - https://en.wikipedia.org/wiki/Acceleration
            Height[i] = 0.5 * Gravity * pow(TimeSinceLastBounce, 2.0) + BallSpeed[i] * TimeSinceLastBounce;

            // Ball hits ground - bounce!
            if (Height[i] < 0)
            {
                Height[i] = 0;
                BallSpeed[i] = Dampening[i] * BallSpeed[i];
                ClockTimeAtLastBounce[i] = Time();

                if (BallSpeed[i] < 0.01)
                    BallSpeed[i] = InitialBallSpeed(StartHeight) * Dampening[i];
            }

            size_t position = (size_t)(Height[i] * (_cLength - 1) / StartHeight);

            leds[position]   += Colors[i];
            leds[position+1] += Colors[i];

            if (_bMirrored)
            {
                leds[_cLength - 1 - position] += Colors[i];
                leds[_cLength - position]     += Colors[i];
            }
        }
        delay(20);
    }
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////  SETUP ROUTINE  //////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CRGBPalette16 currentPalette;
TBlendType    currentBlending;

extern CRGBPalette16 myRedWhiteBluePalette;
extern const TProgmemPalette16 myRedWhiteBluePalette_p PROGMEM;

void setup() {
  // Wait for Power up...
  delay(1000);

  FastLED.addLeds<LED_TYPE,DATA_PIN,COLOR_ORDER>(leds, NUM_LEDS)
    //.setCorrection(TypicalLEDStrip) // cpt-city palettes have different color balance
    .setDither(BRIGHTNESS < 255);

  // set master brightness control
  FastLED.setBrightness(BRIGHTNESS);

  currentPalette = CloudColors_p;
  currentBlending = LINEARBLEND;

  Serial.begin(115200);
  //Start the SPIFFS file system
  SPIFFS.begin();
  
  Serial.println(SPIFFS.begin());  

  //Any pins to set up
  //pinMode(PIN_LED, OUTPUT);

  //Start the serial port
  
  Serial.println();
  Serial.print("Configuring access point...");

  //Configure the access point
  WiFi.softAP(ssid,password);
  //WiFi.mode(WIFI_AP);
  //WiFi.softAPConfig(ip, ip, subnet);        // declared as: bool softAPConfig (IPAddress local_ip, IPAddress gateway, IPAddress subnet)
  //WiFi.softAP("SOME_NAME", "password", 7); 

  //Wait for a connection
  /*while(WiFi.status()!=WL_CONNECTED)
  {
    Serial.print(".");
    delay(500);
  }*/
  Serial.println(" ");
  //switchOff();
  //Print the Access Point IP Address to use in the chosen device browser
  Serial.print("AP IP Address: ");
  Serial.println(WiFi.softAPIP());

  

  //Serve all the required files to the server
  server.on("/",serveIndexFile);
  server.on("/style.css",HTTP_GET,serveCssFile);
  server.on("/bootstrap.min.css", bootstrapmincss);
  server.on("bootstrap.min.css", bootstrapmincss);
  server.on("/bootstrap.min.js", bootstrapminjs);
  server.on("bootstrap.min.js", bootstrapminjs);

  server.begin();

  //Start the websocket
  webSocket.begin();
  //Start the websocket event handler
  webSocket.onEvent(webSocketEvent);

  brightness=0; 
  FastLED.setBrightness(brightness);
  CRGB color = CRGB(0,0,0);
  fill_solid(leds, NUM_LEDS, color);
  FastLED.show();
  FastLED.clear();

}

extern const TProgmemRGBGradientPalettePtr gGradientPalettes[];
extern const uint8_t gGradientPaletteCount;

// Current palette number from the 'playlist' of color palettes
uint8_t gCurrentPaletteNumber = 0;

CRGBPalette16 gCurrentPalette( CRGB::Black);
CRGBPalette16 gTargetPalette( gGradientPalettes[0] );

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////// PERSONAL GRADIENTS /////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CRGBPalette16 lavaPalette = CRGBPalette16(
  CRGB::DarkRed, CRGB::Maroon, CRGB::DarkRed, CRGB::Maroon,
  CRGB::DarkRed, CRGB::Maroon, CRGB::DarkRed, CRGB::DarkRed,
  CRGB::DarkRed, CRGB::Maroon, CRGB::Red, CRGB::Orange,
  CRGB::White, CRGB::Orange, CRGB::Red, CRGB::DarkRed
  );

DEFINE_GRADIENT_PALETTE (diy_heatmap){
  0,    255,  255,  255,
  80,   255,  255,  0,
  175,  255,  0,    0,
  255,  0,    0,    0
};
CRGBPalette16 myPal_1 = diy_heatmap;

DEFINE_GRADIENT_PALETTE (ice_fire){
  0,    255,  0,  0,
  80,   255,  0,  0,
  175,  255,  0,  128,
  255,  0,  255,  255
};
CRGBPalette16 myPal_2 = ice_fire;

DEFINE_GRADIENT_PALETTE (grass_sky){
  0,    0,    255,  0,
  64,  0,    255,  0,
  179,  0,    255,  255,
  255,  0,    255,  255
};
CRGBPalette16 myPal_3 = grass_sky;

DEFINE_GRADIENT_PALETTE (orange_plums){
  0,   255,    0,    0,
  64,  255,    0,   0,
  179, 255,    128,   0,
  255, 255,    128,   0
};
CRGBPalette16 myPal_4 = orange_plums;

DEFINE_GRADIENT_PALETTE (pina_colada){
  0,   255,   0,  128,
  64,  255,   0,  128,
  179, 255,   128,  0,
  255, 255,   128,  0
};
CRGBPalette16 myPal_5 = pina_colada;

DEFINE_GRADIENT_PALETTE (cut_grass){
  0,   128,   255,  0,
  64,  128,   255,  0,
  179, 0,   255,  128,
  255, 0,   255,  128
};
CRGBPalette16 myPal_6 = cut_grass;

DEFINE_GRADIENT_PALETTE (mild_frustration){
  0,   180,   180,  180,
  64,  180,   180,  180,
  179, 128,   0,  255,
  255, 128,   0,  255
};
CRGBPalette16 myPal_7 = mild_frustration;

DEFINE_GRADIENT_PALETTE (sherbert){
  0,   0,   255,  0,
  100,  255,   128,  0,
  200,  255,   0,  128,
  255, 255,   0,  128
};
CRGBPalette16 myPal_8 = sherbert;

DEFINE_GRADIENT_PALETTE (blueberry){
  0,   155,   0,  255,
  64,  155,   0,  255,
  179, 0,   155,  255,
  255, 0,   155,  255
};
CRGBPalette16 myPal_9 = blueberry;

DEFINE_GRADIENT_PALETTE (mag_rose){
  0,   255,   0,  255,
  64,  255,   0,  255,
  179, 255,   0,  128,
  255, 255,   0,  128
};
CRGBPalette16 myPal_10 = mag_rose;

DEFINE_GRADIENT_PALETTE (azure_rose){
  0,   0,   128,  255,
  64,  0,   128,  255,
  179, 255,   0,  128,
  255, 255,   0,  128
};
CRGBPalette16 myPal_11 = azure_rose;

DEFINE_GRADIENT_PALETTE (red_blue){
  0,   255,   0,  0,
  64,  255,   0,  0,
  179, 0,   0,  255,
  255, 0,   0,  255
};
CRGBPalette16 myPal_12 = red_blue;

DEFINE_GRADIENT_PALETTE (azure_violet){
  0,   0,   128,  255,
  64,  0,   128,  255,
  179, 128,   0,  255,
  255, 128,   0,  255
};
CRGBPalette16 myPal_13 = azure_violet;

DEFINE_GRADIENT_PALETTE (cyan_yellow){
  0,   0,   255,  255,
  64,  0,   255,  255,
  179, 255,   255,  0,
  255, 255,   255,  0
};
CRGBPalette16 myPal_14 = cyan_yellow;

DEFINE_GRADIENT_PALETTE (white_rose){
  0,   180,   180,  180,
  64,  180,   180,  180,
  179, 255,   0,  128,
  255, 255,   0,  128
};
CRGBPalette16 myPal_15 = white_rose;

DEFINE_GRADIENT_PALETTE (azure_red){
  0,   0,   128,  255,
  64,  0,   128,  255,
  179, 255,   0,  0,
  255, 255,   0,  0
};
CRGBPalette16 myPal_16 = azure_red;

DEFINE_GRADIENT_PALETTE (rasta){
  0,   0,   255,  0,
  115,  255,   128,  0,
  190, 255,   0,  0,
  255, 255,   0,  0
};
CRGBPalette16 myPal_17 = rasta;

DEFINE_GRADIENT_PALETTE (springGreen_azure){
  0,   0,   255,  128,
  64,  0,   255,  128,
  179, 0,   128,  255,
  255, 0,   128,  255
};
CRGBPalette16 myPal_18 = springGreen_azure;

DEFINE_GRADIENT_PALETTE (candyfloss){
  0,   0,   128,  255,
  80,  255,   128,  0,
  200, 255,   0,  128,
  255, 255,   0,  128
};
CRGBPalette16 myPal_19 = candyfloss;

DEFINE_GRADIENT_PALETTE (blue_yellow){
  0,   0,   0,  255,
  64,  0,   0,  255,
  179, 255,   255,  0,
  255, 255,   255,  0
};
CRGBPalette16 myPal_20 = blue_yellow;

DEFINE_GRADIENT_PALETTE (yellow_violet){
  0,   255,   255,  0,
  64,  255,   255,  0,
  179, 128,   0,  255,
  255, 128,   0,  255
};
CRGBPalette16 myPal_21 = yellow_violet;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////  
////////////////////////////////////////////////////////////////  MAIN LOOP  /////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void loop() {
  webSocket.loop();
  server.handleClient();

  BouncingBallEffect balls(NUM_LEDS,5,48);
  for (int i=0; i<NUM_LEDS; i++){
    colorIndex[i]=i;
  }

  while(true)
  {
            // put your main code here, to run repeatedly:
            webSocket.loop();
            server.handleClient();
            //if(Serial.available()>0){
            //  char c[] = {(char)Serial.read()};
            //  webSocket.broadcastTXT(c, sizeof(c));
            //}
                         
            
            /*if(ID==0){
              brightness=0; 
              FastLED.setBrightness(brightness);
              CRGB color = CRGB(0,0,0);
              fill_solid(leds, NUM_LEDS, color);
              FastLED.show();
              
              FastLED[0].setLeds(leds, num_LEDs);
              FastLED.addLeds<LED_TYPE,DATA_PIN,COLOR_ORDER>(leds, num_LEDs);
              //.setCorrection(TypicalLEDStrip) // cpt-city palettes have different color balance
              brightness=25;
              FastLED.setDither(brightness < 255);

              // set master brightness control
              FastLED.setBrightness(brightness);
              ID=100;
            }*/
                      
            if(ID==100){
              switchOn(red,green,blue);
            }
          
            if(ID==101){
              switchOff();
            }
            
            if(ID==102){
             flashiddyFlash();
            }
            
            if(ID==103){
             stars();
            }
            
            if(ID==104){
              comet();
            }
            
            if(ID==105){
             rainbow1();
            }
            
            if(ID==106){
             rainbow2(10,10);
             FastLED.show();
            }
            
            if(ID==107){
             EVERY_N_MILLISECONDS(thisdelay) {                           // FastLED based non-blocking delay to update/display the sequence.
              ripple();
            }
             FastLED.show();
            }
            
            if(ID==108){
             comet2();
            }
            
            if(ID==109){
             rain();
            }
            
            if(ID==110){
             zigZag();
            }
            
            if(ID==111){
             flashiddyFlashRGB();
            }
            
            if(ID==112){
             balls.Draw(); 
             bounce();
            }
          
            if(ID==113){
             breathe();
            }
          
            if(ID==114){
             viveLaFrance();
            }
          
            if(ID==115){
             clouds();
            }
          
            if(ID==116){
             twinkle();
            }
          
            if(ID==117){
             lava();
             FastLED.show();
            }
          
            if(ID==118){
             applause();
            }
          
            if(ID==119){
             confetti();
            }
          
            if(ID==120){
             crayCray();
            }
          
            if(ID==121){
             
             fire_main();
             FastLED.show();
             FastLED.delay(20);
            }
  
            ////////////////////////////////////////////////////////// GRADIENTS ///////////////////////////////////////////
            if(ID==2001){
             displayGradient(myPal_1);
             if(ID!=2001){
             return;
             }
            }
            if(ID==2002){
             displayGradient(myPal_2);
             if(ID!=2002){
             return;
             }
            }
            if(ID==2003){
             displayGradient(myPal_3);
             if(ID!=2003){
             return;
             }
            }
            if(ID==2004){
             displayGradient(myPal_4);
             if(ID!=2004){
             return;
             }
            }
            if(ID==2005){
             displayGradient(myPal_5);
             if(ID!=2005){
             return;
             }
            }
            if(ID==2006){
             displayGradient(myPal_6);
             if(ID!=2006){
             return;
             }
            }
            if(ID==2007){
             displayGradient(myPal_7);
             if(ID!=2007){
             return;
             }
            }
            if(ID==2008){
             displayGradient(myPal_8);
             if(ID!=2008){
             return;
             }
            }
            if(ID==2009){
             displayGradient(myPal_9);
             if(ID!=2009){
             return;
             }
            }
            if(ID==2010){
             displayGradient(myPal_10);
             if(ID!=2010){
             return;
             }
            }
            if(ID==2011){
             displayGradient(myPal_11);
             if(ID!=2011){
             return;
             }
            }
            if(ID==2012){
             displayGradient(myPal_12);
             if(ID!=2012){
             return;
             }
            }
            if(ID==2013){
             displayGradient(myPal_13);
             if(ID!=2013){
             return;
             }
            }
            if(ID==2014){
             displayGradient(myPal_14);
             if(ID!=2014){
             return;
             }
            }
            if(ID==2015){
             displayGradient(myPal_15);
             if(ID!=2015){
             return;
             }
            }
            if(ID==2016){
             displayGradient(myPal_16);
             if(ID!=2016){
             return;
             }
            }
            if(ID==2017){
             displayGradient(myPal_17);
             if(ID!=2017){
             return;
             }
            }
            if(ID==2018){
             displayGradient(myPal_18);
             if(ID!=2018){
             return;
             }
            }
            if(ID==2019){
             displayGradient(myPal_19);
             if(ID!=2019){
             return;
             }
            }
            if(ID==2020){
             displayGradient(myPal_20);
             if(ID!=2020){
             return;
             }
            }
            if(ID==2021){
             displayGradient(myPal_21);
             if(ID!=2021){
             return;
             }
            }
            ////// ADD MORE EXCLUSIONS HERE //////
            if(ID!=102 && ID!=103 && ID!=104 && ID!=105 && ID!=106 && ID!=107 && ID!=108 && ID!=109 && ID!=110 && ID!=111 && ID!=112 && ID!=113 && ID!=114 && ID!=115 && ID!=116 && ID!=117 && ID!=118 && ID!=119 && ID!=120 && ID!=121){
             setColorRGB(red,green,blue);
             Serial.println(red);
             Serial.println(green);
             Serial.println(blue);
            }
            
            //loop_counter++;
            //if(loop_counter == 20000)
            //{
            //  loop_counter = 0;
              
            //}
            
            Serial.print("ID= ");
            Serial.print(ID);
            Serial.print("; ");
            Serial.print("Brightness= ");
            Serial.println(brightness);
          
          }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////////////////////////////////////////  USER FUNCTIONS  ///////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length){
  
   
  if(type == WStype_TEXT){
    if(payload[0]=='#'){
      brightness = (uint16_t) strtol((const char *) &payload[1], NULL, 10);
      FastLED.setBrightness(brightness);
      //analogWrite(PIN_LED, brightness);
      Serial.print("brightness= ");
      Serial.print(brightness);
      Serial.print("; ");
      Serial.print("BRIGHTNESS= ");
      Serial.println(BRIGHTNESS);
    }
    else{

    red_last = red;
    green_last = green;
    blue_last = blue;
    
    Serial.printf("%s\n", payload);
    String message = String((char*)(payload));
    //Serial.println(message);
    //for(int i = 0; i< length; i++)
    //  Serial.print((char) payload[i]);
    //  Serial.println();

    DynamicJsonDocument doc(200);

    DeserializationError error = deserializeJson(doc, message);

    //if(error){
      //Serial.print("deserializeJson() failed: ");
      //Serial.println(error.c_str());
      //return;
    //}

    num_LEDs = doc["num_LEDs"];
    ID = doc["ID"];
    func_tion = doc["Function"];
    stat_us = doc["State"];
    
    if(func_tion>0){
      red = red_last;
      green = green_last;
      blue = blue_last;
    }
    else{
      red = doc["Red"];
      green = doc["Green"];
      blue = doc["Blue"];
    }

    //setColorRGB(red,green,blue);

    //Serial.print(BRIGHTNESS);
    //Serial.print(", ");
    //Serial.print(brightness);
    //Serial.print(", ");

    //Serial.print(red_last);
    //Serial.print(", ");
    //Serial.print(green_last);
    //Serial.print(", ");
    //Serial.println(blue_last);

    
      
    }
  }
  
}


///////////////////////////////// PASS WEB SERVER FILES TO USER DEVICE ////////////////////////////////

void serveIndexFile()
{
  File file = SPIFFS.open("/index.html","r");
  server.streamFile(file, "text/html");
  file.close();
}

void serveCssFile()
{
  File file = SPIFFS.open("/style.css","r");
  server.streamFile(file, "text/css");
  file.close();
}

void bootstrapmincss()
{
  File file = SPIFFS.open("/bootstrap.min.css", "r");
  server.streamFile(file, "text/css");
  file.close();
}

void bootstrapminjs()
{
  File file = SPIFFS.open("/bootstrap.min.js", "r");
  server.streamFile(file, "application/javascript");
  file.close();
}

////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////// ALL COLOUR & EFFECT OPTIONS //////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////// COLOR WAVES //////////////////////////////////////

void colorwaves( CRGB* ledarray, uint16_t numleds, CRGBPalette16& palette) 
{
  static uint16_t sPseudotime = 0;
  static uint16_t sLastMillis = 0;
  static uint16_t sHue16 = 0;
 
  uint8_t sat8 = beatsin88( 87, 220, 250);
  uint8_t brightdepth = beatsin88( 341, 96, 224);
  uint16_t brightnessthetainc16 = beatsin88( 203, (25 * 256), (40 * 256));
  uint8_t msmultiplier = beatsin88(147, 23, 60);

  uint16_t hue16 = sHue16;//gHue * 256;
  uint16_t hueinc16 = beatsin88(113, 300, 1500);
  
  uint16_t ms = millis();
  uint16_t deltams = ms - sLastMillis ;
  sLastMillis  = ms;
  sPseudotime += deltams * msmultiplier;
  sHue16 += deltams * beatsin88( 400, 5,9);
  uint16_t brightnesstheta16 = sPseudotime;
  
  for( uint16_t i = 0 ; i < numleds; i++) {
    hue16 += hueinc16;
    uint8_t hue8 = hue16 / 256;
    uint16_t h16_128 = hue16 >> 7;
    if( h16_128 & 0x100) {
      hue8 = 255 - (h16_128 >> 1);
    } else {
      hue8 = h16_128 >> 1;
    }

    brightnesstheta16  += brightnessthetainc16;
    uint16_t b16 = sin16( brightnesstheta16  ) + 32768;

    uint16_t bri16 = (uint32_t)((uint32_t)b16 * (uint32_t)b16) / 65536;
    uint8_t bri8 = (uint32_t)(((uint32_t)bri16) * brightdepth) / 65536;
    bri8 += (255 - brightdepth);
    
    uint8_t index = hue8;
    //index = triwave8( index);
    index = scale8( index, 240);

    CRGB newcolor = ColorFromPalette( palette, index, bri8);

    uint16_t pixelnumber = i;
    pixelnumber = (numleds-1) - pixelnumber;
    
    nblend( ledarray[pixelnumber], newcolor, 128);
  }
}

// Alternate rendering function just scrolls the current palette 
// across the defined LED strip.
void palettetest( CRGB* ledarray, uint16_t numleds, const CRGBPalette16& gCurrentPalette)
{
  static uint8_t startindex = 0;
  startindex--;
  fill_palette( ledarray, numleds, startindex, (256 / NUM_LEDS) + 1, gCurrentPalette, 255, LINEARBLEND);
}

///////////////////////////////////////// SWITCH ON ////////////////////////////////////// ID=100 ////
void switchOn(byte r,byte g,byte b){ 
  if(r==0){
    r=10; 
  }
  if(g==0){
    g=10; 
  }
  if(b==0){
    b=10; 
  }
  if(brightness==0){
    brightness=50;
  }
  FastLED.setBrightness(brightness);
  CRGB color = CRGB(r,g,b);
  fill_solid(leds, NUM_LEDS, color);
  FastLED.show();
}

//////////////////////////////////////// SWITCH OFF ////////////////////////////////////// ID=101 ////
void switchOff(void){ 
  brightness=0; 
  FastLED.setBrightness(brightness);
  CRGB color = CRGB(0,0,0);
  fill_solid(leds, NUM_LEDS, color);
  FastLED.show();
  FastLED.clear();
  
  ID=1;
  red=red_last;
  green=green_last;
  blue=blue_last;
}

/////////////////////////////////// PICK A STATIC COLOUR /////////////////////////////////
void setColorRGB(byte r, byte g, byte b){
  CRGB color = CRGB(r,g,b);
  FastLED.setBrightness(brightness);
  fill_solid(leds, NUM_LEDS, color);
  FastLED.show();
}


//////////////////////////////////// ADD A BASIC EFFECT //////////////////////////////////

//// FLASH //// ID=102 ////
void flashiddyFlash(void){
  CRGB color = CRGB(0,0,0);
  fill_solid(leds, NUM_LEDS, color);
  FastLED.show();

  fill_solid(leds, NUM_LEDS, CRGB(red,green,blue));
  FastLED.show();
  FastLED.delay(500);
  
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();
  FastLED.delay(500);
  
}

//// STARS //// ID=103 ////
void stars(void){
  // built-in FastLED rainbow, plus some random sparkly glitter
   fadeToBlackBy( leds, NUM_LEDS, 10);
   addGlitter(80);
   FastLED.show();

   FastLED.delay(50);
   webSocket.loop();
   server.handleClient();
   //FastLED.setBrightness(brightness);
   if(ID!=103){
   return;
   }
}

//// COMET //// ID=104 ////
void comet(void){
  //CRGB color = CRGB(0,0,0);
  //fill_solid(leds, NUM_LEDS, color);
  //FastLED.show();
  
  for(int i=NUM_LEDS-1; i>=0; i--){
    
   leds[i]=CRGB(red, green, blue);
   FastLED.show();
   FastLED.delay(10);
   leds[i]=CRGB(0,0,0); 
   webSocket.loop();
   server.handleClient();
   FastLED.setBrightness(brightness);
   if(ID!=104){
    return;
   }
  }
}

//// RAINBOW 1 //// ID=105 ////
void rainbow1(void){
            
          EVERY_N_SECONDS( SECONDS_PER_PALETTE ) {
            gCurrentPaletteNumber = addmod8( gCurrentPaletteNumber, 1, gGradientPaletteCount);
            gTargetPalette = gGradientPalettes[ gCurrentPaletteNumber ];
          }
        
          EVERY_N_MILLISECONDS(40) {
            nblendPaletteTowardPalette( gCurrentPalette, gTargetPalette, 16);
          }
          
          colorwaves( leds, NUM_LEDS, gCurrentPalette);
        
          FastLED.show();
          FastLED.delay(20);
}

//// RAINBOW 2 //// ID=106 ////
void rainbow2(uint8_t thisSpeed, uint8_t deltaHue){

        for(int i=0; i<NUM_LEDS; i++){
           leds[i]=CRGB(0, 0, 0);
        }
        // uint8_t thisHue = beatsin8(thisSpeed,0,255);                // A simple rainbow wave.
        uint8_t thisHue = beat8(thisSpeed,255);                     // A simple rainbow march.
  
        fill_rainbow(leds, NUM_LEDS, thisHue, deltaHue);  
}

//// RIPPLE //// ID=107 ////
void ripple(void){
  for (int i = 0; i < NUM_LEDS; i++) leds[i] = CHSV(bgcol++, 255, 50);  // Rotate background colour.

  switch (step) {

    case -1:                                                          // Initialize ripple variables.
      center = random(NUM_LEDS);
      colour = random8();
      step = 0;
      break;

    case 0:
      leds[center] = CHSV(colour, 255, 255);                          // Display the first pixel of the ripple.
      step ++;
      break;

    case maxsteps:                                                    // At the end of the ripples.
      step = -1;
      break;

    default:                                                             // Middle of the ripples.
      leds[(center + step + NUM_LEDS) % NUM_LEDS] += CHSV(colour, 255, myfade/step);       // Simple wrap from Marc Miller
      leds[(center - step + NUM_LEDS) % NUM_LEDS] += CHSV(colour, 255, myfade/step);
      step ++;                                                         // Next step.
      break;  
  } // switch step
  
   webSocket.loop();
   server.handleClient();
   FastLED.setBrightness(brightness);
   if(ID!=107){
   return;
   }
} // ripple()

//// Comet2 //// ID=108 ////
void comet2(void){

   const byte fadeAmt = 80;
   const int cometSize = 3;
   const int deltaHue = 4;

   static byte hue = HUE_RED;       //Current colour
   static int iDirection = 1;       //Current direction (-1 or +1)
   static int iPos = 0;             //Current comet position on strip
   
   hue += deltaHue;                 //Update the comet colour
   iPos += iDirection;              //Update the comet position
   //Flip comet at either end
   if(iPos == (NUM_LEDS - cometSize) || iPos == 0) {iDirection *= -1;}
   
   //Draw comet at current position
   for (int i = 0; i < cometSize; i++) {leds[iPos + i].setHue(hue);}

   for (int j = 0; j<NUM_LEDS; j++) {
    if(random(2)==1){
      leds[j].fadeToBlackBy(fadeAmt);
    }
   }

   FastLED.setBrightness(brightness);
   FastLED.show();
   FastLED.delay(50);
   webSocket.loop();
   server.handleClient();
   FastLED.setBrightness(brightness);
   if(ID!=108){
   return;
   }
}

//// RAIN //// ID=109 ////
void rain(void){

   const byte fadeAmt = 150;
   const int dropletSize1 = 3;
   const int dropletSize2 = 1;
   const int deltaHue = 4;

   static byte hue = HUE_RED;       //Current colour
   static int iDirection = -1;
   static int iPos1 = random8(NUM_LEDS);             //Current comet position on strip
   static int iPos2 = random8(NUM_LEDS);

   //hue += deltaHue;                 //Update the comet colour
   iPos1 += iDirection;
   iPos2 += iDirection;
   if(iPos1 == 0) {iPos1 = NUM_LEDS - dropletSize1;}
   if(iPos2 == 0) {iPos2 = NUM_LEDS - dropletSize2;}
   //Draw comet at current position
   for (int i = dropletSize1; i >=0; i--) {leds[iPos1 - i] = CRGB(180,180,180);}
   for (int j = dropletSize2; j >=0; j--) {leds[iPos2 - j] = CRGB(180,180,180);}

   for (int k = 0; k<NUM_LEDS; k++) {
    if(random(2)==1){
      leds[k].fadeToBlackBy(fadeAmt);
    }
   }

   FastLED.setBrightness(brightness);
   FastLED.show();
   FastLED.delay(50);
   webSocket.loop();
   server.handleClient();
   FastLED.setBrightness(brightness);
   if(ID!=109){
   return;
   }
  
}


//// ZIG-ZAG //// ID=110 ////
void zigZag(void){

  //CRGB color = CRGB(0,0,0);
  //fill_solid(leds, NUM_LEDS, color);
  //FastLED.show();

  for(int i=0; i<NUM_LEDS; i++){
   
   leds[i]=CRGB(red, green, blue);
   FastLED.show();
   FastLED.delay(20);
   leds[i]=CRGB(0,0,0);
   webSocket.loop();
   server.handleClient();
   FastLED.setBrightness(brightness);
   if(ID!=110){
    return;
   } 
  }
  for(int i=NUM_LEDS-1; i>=0; i--){
   
   leds[i]=CRGB(red, green, blue);
   FastLED.show();
   FastLED.delay(20);
   leds[i]=CRGB(0,0,0);
   webSocket.loop();
   server.handleClient();
   FastLED.setBrightness(brightness);
   if(ID!=110){
    return;
   }
  }
}

//// FLASH RGB //// ID=111 ////
void flashiddyFlashRGB(void){
  CRGB color = CRGB(0,0,0);
  fill_solid(leds, NUM_LEDS, color);
  FastLED.show();

  fill_solid(leds, NUM_LEDS, CRGB::Red);
  FastLED.show();
  FastLED.delay(500);
  fill_solid(leds, NUM_LEDS, CRGB::Green);
  FastLED.show();
  FastLED.delay(500);
  fill_solid(leds, NUM_LEDS, CRGB::Blue);
  FastLED.show();
  FastLED.delay(500);
}
  
//// BOUNCE //// ID=112 ////
void bounce(void){
  //CRGB color = CRGB(0,0,0);
  //fill_solid(leds, NUM_LEDS, color);
  //FastLED.show();
  
   FastLED.show();
   //FastLED.delay(1);
   
   webSocket.loop();
   server.handleClient();
   FastLED.setBrightness(brightness);
   if(ID!=112){
   return;
   }
}


//// BREATHE //// ID=113 ////
void breathe(void){
  CRGB color = CRGB(0,0,0);
  fill_solid(leds, NUM_LEDS, color);
  FastLED.show();

  float breath = (exp(sin(millis()/5000.0*PI)) - 0.36787944)*108.0;
  breath = map(breath, 0, 255, 10, 100);
  FastLED.setBrightness(breath);
  fill_solid(leds, NUM_LEDS, CRGB(red,green,blue));
  
  FastLED.show();
  //FastLED.delay(1);
   
   webSocket.loop();
   server.handleClient();
   //FastLED.setBrightness(brightness);
   if(ID!=113){
   return;
   }
}

//// VIVELAFRANCE //// ID=114 ////
void viveLaFrance(void){
  static uint8_t startIndex = 0;
  startIndex = startIndex + 1; /* motion speed */
  currentPalette = myRedWhiteBluePalette_p;
  FillLEDsFromPaletteColors(startIndex);

  FastLED.show();
  FastLED.delay(1000 / UPDATES_PER_SECOND);

   webSocket.loop();
   server.handleClient();
   FastLED.setBrightness(brightness);
   if(ID!=114){
   return;
   }
}

//// CLOUDS //// ID=115 ////
void clouds(void){
  static uint8_t startIndex = 0;
  startIndex = startIndex + 1; /* motion speed */
  currentPalette = CloudColors_p;
  FillLEDsFromPaletteColors(startIndex);

  FastLED.show();
  FastLED.delay(1000 / UPDATES_PER_SECOND);

   webSocket.loop();
   server.handleClient();
   FastLED.setBrightness(brightness);
   if(ID!=115){
   return;
   }
}


//// TWINKLE //// ID=116 ////
void twinkle(void){
   int i = random(NUM_LEDS);                                           // A random number. Higher number => fewer twinkles. Use random16() for values >255.
   if (i < NUM_LEDS) leds[i] = CHSV(random(255), 255, 255);              // Only the lowest probability twinkles will do. You could even randomize the hue/saturation. .
   for (int j = 0; j < NUM_LEDS; j++) leds[j].fadeToBlackBy(8);
  
   LEDS.show();                                                // Standard FastLED display
   //show_at_max_brightness_for_power();                          // Power managed FastLED display

   //delay(10);                                            // Standard delay
   LEDS.delay(50); 

   webSocket.loop();
   server.handleClient();
   FastLED.setBrightness(brightness);
   if(ID!=116){
   return;
   }
}

//// LAVA //// ID=117 ////
void lava(void){
  // eight colored dots, weaving in and out of sync with each other
   for(int i = 0; i < NUM_LEDS; i++){
    uint8_t brightness2 = inoise8(i * brightnessScale, millis() / 5);
    uint8_t index = inoise8(i*indexScale, millis());
    
    leds[i] = ColorFromPalette(lavaPalette, index, brightness2);
   }
  
   webSocket.loop();
   server.handleClient();
   //FastLED.setBrightness(brightness2);
   if(ID!=117){
   return;
   }
}

//// APPLAUSE //// ID=118 ////
void applause(void){

   static uint16_t lastPixel = 0;
   fadeToBlackBy( leds, NUM_LEDS, 32);
   leds[lastPixel] = CHSV(random8(HUE_BLUE, HUE_PURPLE), 255, 255);
   lastPixel = random16(NUM_LEDS);
   leds[lastPixel] = CRGB::White;
   FastLED.show();
   
   webSocket.loop();
   server.handleClient();
   FastLED.setBrightness(brightness);
   if(ID!=118){
   return;
   }
}  

//// CONFETTI //// ID=119 //// 
void confetti(void){
   // random colored speckles that blink in and fade smoothly
   fadeToBlackBy( leds, NUM_LEDS, 10);
   int pos = random16(NUM_LEDS);
   leds[pos] += CHSV( thishue + random8(255), 200, 255);
   FastLED.show();
  
   webSocket.loop();
   server.handleClient();
   FastLED.setBrightness(brightness);
   if(ID!=119){
   return;
   }
}  

//// CRAY CRAY //// ID=120 ////
void crayCray(void){

   // built-in FastLED rainbow, plus some random sparkly glitter
   fadeToBlackBy( leds, NUM_LEDS, 10);
   addGlitter(80);
   FastLED.show();
  
   webSocket.loop();
   server.handleClient();
   //FastLED.setBrightness(brightness);
   if(ID!=120){
   return;
   }
}  

//// FIRE //// ID=121 ////
void fire_main(void){
  
    // Array of temperature readings at each simulation cell
    static byte heat[NUM_LEDS];

    // Step 1.  Cool down every cell a little
    for( int i = 0; i < NUM_LEDS; i++) {
      heat[i] = qsub8( heat[i],  random8(0, ((COOLING * 10) / NUM_LEDS) + 2));
    }
  
    // Step 2.  Heat from each cell drifts 'up' and diffuses a little
    for( int k= NUM_LEDS - 1; k >= 2; k--) {
      heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2] ) / 3;
    }
    
    // Step 3.  Randomly ignite new 'sparks' of heat near the bottom
    if( random8() < SPARKING ) {
      int y = random8(7);
      heat[y] = qadd8( heat[y], random8(160,255) );
    }

    // Step 4.  Map from heat cells to LED colors
    for( int j = 0; j < NUM_LEDS; j++) {
        leds[j] = HeatColor( heat[j]);
    }
   
   
   webSocket.loop();
   server.handleClient();
   FastLED.setBrightness(brightness);
   if(ID!=121){
   return;
   }
}  

//// RGB RAIN //// ID=109 ////
void rgb_rain(void){

   const byte fadeAmt = 150;
   const int dropletSize1 = 3;
   const int dropletSize2 = 1;
   const int deltaHue = 4;

   static byte hue = HUE_RED;       //Current colour
   static int iDirection = -1;
   static int iPos1 = random8(NUM_LEDS);             //Current comet position on strip
   static int iPos2 = random8(NUM_LEDS);

   hue += deltaHue;                 //Update the comet colour
   iPos1 += iDirection;
   iPos2 += iDirection;
   if(iPos1 == 0) {iPos1 = NUM_LEDS - dropletSize1;}
   if(iPos2 == 0) {iPos2 = NUM_LEDS - dropletSize2;}
   //Draw comet at current position
   for (int i = dropletSize1; i >=0; i--) {leds[iPos1 - i].setHue(hue);}
   for (int j = dropletSize2; j >=0; j--) {leds[iPos2 - j].setHue(hue);}

   for (int k = 0; k<NUM_LEDS; k++) {
    if(random(2)==1){
      leds[k].fadeToBlackBy(fadeAmt);
    }
   }

   FastLED.setBrightness(brightness);
   FastLED.show();
   FastLED.delay(50);
   webSocket.loop();
   server.handleClient();
   FastLED.setBrightness(brightness);
   if(ID!=109){
   return;
   }
  
}




////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////// PALETTE-KNIFE GRADIENTS /////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_GRADIENT_PALETTE( es_rivendell_15_gp ) {
    0,   1, 14,  5,
  101,  16, 36, 14,
  165,  56, 68, 30,
  242, 150,156, 99,
  255, 150,156, 99};

DEFINE_GRADIENT_PALETTE( rgi_15_gp ) {
    0,   4,  1, 31,
   31,  55,  1, 16,
   63, 197,  3,  7,
   95,  59,  2, 17,
  127,   6,  2, 34,
  159,  39,  6, 33,
  191, 112, 13, 32,
  223,  56,  9, 35,
  255,  22,  6, 38};

DEFINE_GRADIENT_PALETTE( retro2_16_gp ) {
    0, 188,135,  1,
  255,  46,  7,  1};

DEFINE_GRADIENT_PALETTE( Analogous_1_gp ) {
    0,   3,  0,255,
   63,  23,  0,255,
  127,  67,  0,255,
  191, 142,  0, 45,
  255, 255,  0,  0};

DEFINE_GRADIENT_PALETTE( es_pinksplash_08_gp ) {
    0, 126, 11,255,
  127, 197,  1, 22,
  175, 210,157,172,
  221, 157,  3,112,
  255, 157,  3,112};

DEFINE_GRADIENT_PALETTE( es_pinksplash_07_gp ) {
    0, 229,  1,  1,
   61, 242,  4, 63,
  101, 255, 12,255,
  127, 249, 81,252,
  153, 255, 11,235,
  193, 244,  5, 68,
  255, 232,  1,  5};

DEFINE_GRADIENT_PALETTE( Coral_reef_gp ) {
    0,  40,199,197,
   50,  10,152,155,
   96,   1,111,120,
   96,  43,127,162,
  139,  10, 73,111,
  255,   1, 34, 71};

DEFINE_GRADIENT_PALETTE( es_ocean_breeze_068_gp ) {
    0, 100,156,153,
   51,   1, 99,137,
  101,   1, 68, 84,
  104,  35,142,168,
  178,   0, 63,117,
  255,   1, 10, 10};

DEFINE_GRADIENT_PALETTE( es_ocean_breeze_036_gp ) {
    0,   1,  6,  7,
   89,   1, 99,111,
  153, 144,209,255,
  255,   0, 73, 82};

DEFINE_GRADIENT_PALETTE( es_landscape_33_gp ) {
    0,   1,  5,  0,
   19,  32, 23,  1,
   38, 161, 55,  1,
   63, 229,144,  1,
   66,  39,142, 74,
  255,   1,  4,  1};

DEFINE_GRADIENT_PALETTE( rainbowsherbet_gp ) {
    0, 255, 33,  4,
   43, 255, 68, 25,
   86, 255,  7, 25,
  127, 255, 82,103,
  170, 255,255,242,
  209,  42,255, 22,
  255,  87,255, 65};

DEFINE_GRADIENT_PALETTE( gr65_hult_gp ) {
    0, 247,176,247,
   48, 255,136,255,
   89, 220, 29,226,
  160,   7, 82,178,
  216,   1,124,109,
  255,   1,124,109};

DEFINE_GRADIENT_PALETTE( gr64_hult_gp ) {
    0,   1,124,109,
   66,   1, 93, 79,
  104,  52, 65,  1,
  130, 115,127,  1,
  150,  52, 65,  1,
  201,   1, 86, 72,
  239,   0, 55, 45,
  255,   0, 55, 45};

DEFINE_GRADIENT_PALETTE( GMT_drywet_gp ) {
    0,  47, 30,  2,
   42, 213,147, 24,
   84, 103,219, 52,
  127,   3,219,207,
  170,   1, 48,214,
  212,   1,  1,111,
  255,   1,  7, 33};

DEFINE_GRADIENT_PALETTE( ib15_gp ) {
    0, 113, 91,147,
   72, 157, 88, 78,
   89, 208, 85, 33,
  107, 255, 29, 11,
  141, 137, 31, 39,
  255,  59, 33, 89};

DEFINE_GRADIENT_PALETTE( Fuschia_7_gp ) {
    0,  43,  3,153,
   63, 100,  4,103,
  127, 188,  5, 66,
  191, 161, 11,115,
  255, 135, 20,182};

DEFINE_GRADIENT_PALETTE( Colorfull_gp ) {
    0,  10, 85,  5,
   25,  29,109, 18,
   60,  59,138, 42,
   93,  83, 99, 52,
  106, 110, 66, 64,
  109, 123, 49, 65,
  113, 139, 35, 66,
  116, 192,117, 98,
  124, 255,255,137,
  168, 100,180,155,
  255,  22,121,174};

DEFINE_GRADIENT_PALETTE( Magenta_Evening_gp ) {
    0,  71, 27, 39,
   31, 130, 11, 51,
   63, 213,  2, 64,
   70, 232,  1, 66,
   76, 252,  1, 69,
  108, 123,  2, 51,
  255,  46,  9, 35};

DEFINE_GRADIENT_PALETTE( Pink_Purple_gp ) {
    0,  19,  2, 39,
   25,  26,  4, 45,
   51,  33,  6, 52,
   76,  68, 62,125,
  102, 118,187,240,
  109, 163,215,247,
  114, 217,244,255,
  122, 159,149,221,
  149, 113, 78,188,
  183, 128, 57,155,
  255, 146, 40,123};

DEFINE_GRADIENT_PALETTE( Sunset_Real_gp ) {
    0, 120,  0,  0,
   22, 179, 22,  0,
   51, 255,104,  0,
   85, 167, 22, 18,
  135, 100,  0,103,
  198,  16,  0,130,
  255,   0,  0,160};

DEFINE_GRADIENT_PALETTE( BlacK_Blue_Magenta_White_gp ) {
    0,   0,  0,  0,
   42,   0,  0, 45,
   84,   0,  0,255,
  127,  42,  0,255,
  170, 255,  0,255,
  212, 255, 55,255,
  255, 255,255,255};

DEFINE_GRADIENT_PALETTE( Blue_Cyan_Yellow_gp ) {
    0,   0,  0,255,
   63,   0, 55,255,
  127,   0,255,255,
  191,  42,255, 45,
  255, 255,255,  0};


// Single array of defined cpt-city color palettes.
// This will let us programmatically choose one based on
// a number, rather than having to activate each explicitly 
// by name every time.
// Since it is const, this array could also be moved 
// into PROGMEM to save SRAM, but for simplicity of illustration
// we'll keep it in a regular SRAM array.
//
// This list of color palettes acts as a "playlist"; you can
// add or delete, or re-arrange as you wish.
const TProgmemRGBGradientPalettePtr gGradientPalettes[] = {
  Sunset_Real_gp,
  es_rivendell_15_gp,
  es_ocean_breeze_036_gp,
  rgi_15_gp,
  retro2_16_gp,
  Analogous_1_gp,
  es_pinksplash_08_gp,
  Coral_reef_gp,
  es_ocean_breeze_068_gp,
  es_pinksplash_07_gp,
  es_landscape_33_gp,
  rainbowsherbet_gp,
  gr65_hult_gp,
  gr64_hult_gp,
  GMT_drywet_gp,
  ib15_gp,
  Fuschia_7_gp,
  Colorfull_gp,
  Magenta_Evening_gp,
  Pink_Purple_gp,
  BlacK_Blue_Magenta_White_gp,
  Blue_Cyan_Yellow_gp };


// Count of how many cpt-city gradients are defined:
const uint8_t gGradientPaletteCount = 
  sizeof( gGradientPalettes) / sizeof( TProgmemRGBGradientPalettePtr );




/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FillLEDsFromPaletteColors( uint8_t colorIndex)
{
  uint8_t brightness = 255;
  
  for( int i = 0; i < NUM_LEDS; i++) {
    leds[i] = ColorFromPalette( currentPalette, colorIndex, brightness, currentBlending);
    colorIndex += 1;
  }
}


// There are several different palettes of colors demonstrated here.
//
// FastLED provides several 'preset' palettes: RainbowColors_p, RainbowStripeColors_p,
// OceanColors_p, CloudColors_p, LavaColors_p, ForestColors_p, and PartyColors_p.
//
// Additionally, you can manually define your own color palettes, or you can write
// code that creates color palettes on the fly.  All are shown here.


void ChangePalettePeriodically()
{
  uint8_t secondHand = (millis() / 1000) % 60;
  static uint8_t lastSecond = 99;
  
  if( lastSecond != secondHand) {
    lastSecond = secondHand;
    if( secondHand == 25)  { SetupBlackAndWhiteStripedPalette();       currentBlending = LINEARBLEND; }
    if( secondHand == 35)  { currentPalette = CloudColors_p;           currentBlending = LINEARBLEND; }
    if( secondHand == 55)  { currentPalette = myRedWhiteBluePalette_p; currentBlending = LINEARBLEND; }
  }
}

// This function fills the palette with totally random colors.
void SetupTotallyRandomPalette()
{
  for( int i = 0; i < 16; i++) {
    currentPalette[i] = CHSV( random8(), 255, random8());
  }
}

// This function sets up a palette of black and white stripes,
// using code.  Since the palette is effectively an array of
// sixteen CRGB colors, the various fill_* functions can be used
// to set them up.
void SetupBlackAndWhiteStripedPalette()
{
  // 'black out' all 16 palette entries...
  fill_solid( currentPalette, 16, CRGB::Black);
  // and set every fourth one to white.
  currentPalette[0] = CRGB::White;
  
  currentPalette[4] = CRGB::White;
  
  currentPalette[8] = CRGB::White;
  
  currentPalette[12] = CRGB::White;
  

}

// This function sets up a palette of purple and green stripes.
void SetupPurpleAndGreenPalette()
{
  CRGB purple = CHSV( HUE_PURPLE, 255, 255);
  CRGB green  = CHSV( HUE_GREEN, 255, 255);
  CRGB black  = CRGB::Black;
  
  currentPalette = CRGBPalette16( 
    green,  green,  black,  black,
    purple, purple, black,  black,
    green,  green,  black,  black,
    purple, purple, black,  black );
}


// This example shows how to set up a static color palette
// which is stored in PROGMEM (flash), which is almost always more 
// plentiful than RAM.  A static PROGMEM palette like this
// takes up 64 bytes of flash.
const TProgmemPalette16 myRedWhiteBluePalette_p PROGMEM =
{
  CRGB::Red,
  CRGB::Gray, // 'white' is too bright compared to red and blue
  CRGB::Blue,
  CRGB::Black,
  CRGB::Red,
  CRGB::Gray, // 'white' is too bright compared to red and blue
  CRGB::Blue,
  CRGB::Black,
  CRGB::Red,
  CRGB::Gray, // 'white' is too bright compared to red and blue
  CRGB::Blue,
  CRGB::Black,
  CRGB::Red,
  CRGB::Gray, // 'white' is too bright compared to red and blue
  CRGB::Blue,
  CRGB::Black,
};

/*const TProgmemPalette16 myRainPalette_p PROGMEM =
{
  CRGB::White,
  CRGB(150,150,150), // 'white' is too bright compared to red and blue
  CRGB(100,100,100),
  CRGB(0,0,0),
  CRGB(25,25,25),
  CRGB(0,0,0), // 'white' is too bright compared to red and blue
  CRGB(0,0,0),
  CRGB(0,0,0),
  CRGB::White,
  CRGB(150,150,150), // 'white' is too bright compared to red and blue
  CRGB(100,100,100),
  CRGB(0,0,0),
  CRGB(25,25,25),
  CRGB(0,0,0), // 'white' is too bright compared to red and blue
  CRGB(0,0,0),
  CRGB(0,0,0),
};*/
void displayGradient(CRGBPalette16 passInPalette){
  currentPalette = passInPalette;

  for (int i=0; i<NUM_LEDS; i++){
    leds[i] = ColorFromPalette(currentPalette,colorIndex[i]);
  }
    if(true){
      EVERY_N_MILLISECONDS(5){
        for (int i=0; i<NUM_LEDS; i++)
        {
          colorIndex[i]++;
        }
      }
    }

    FastLED.show();
    FastLED.delay(1000 / UPDATES_PER_SECOND);

    webSocket.loop();
    server.handleClient();
    FastLED.setBrightness(brightness);

}

void rainbow()
{
  // FastLED's built-in rainbow generator
  fill_rainbow( leds, NUM_LEDS, thishue, 7);
}

void addGlitter( fract8 chanceOfGlitter)
{
  if( random8() < chanceOfGlitter)
  {
    leds[ random16(NUM_LEDS) ] += CRGB(red,green,blue);
  }
}

// Additionl notes on FastLED compact palettes:
//
// Normally, in computer graphics, the palette (or "color lookup table")
// has 256 entries, each containing a specific 24-bit RGB color.  You can then
// index into the color palette using a simple 8-bit (one byte) value.
// A 256-entry color palette takes up 768 bytes of RAM, which on Arduino
// is quite possibly "too many" bytes. 
//
// FastLED does offer traditional 256-element palettes, for setups that
// can afford the 768-byte cost in RAM.
//
// However, FastLED also offers a compact alternative.  FastLED offers
// palettes that store 16 distinct entries, but can be accessed AS IF
// they actually have 256 entries; this is accomplished by interpolating
// between the 16 explicit entries to create fifteen intermediate palette 
// entries between each pair.
//
// So for example, if you set the first two explicit entries of a compact 
// palette to Green (0,255,0) and Blue (0,0,255), and then retrieved 
// the first sixteen entries from the virtual palette (of 256), you'd get
// Green, followed by a smooth gradient from green-to-blue, and then Blue.
