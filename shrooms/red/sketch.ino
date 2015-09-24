#include <Ethernet.h>
#include <EthernetUdp.h>
#include <SPI.h>    
#include <Adafruit_NeoPixel.h>

#include <OSCMessage.h>
#include <OSCBoards.h>

#define BAUD          115200

#define P(x)    ;//Serial.print(x)
#define PL(x)   ;//Serial.println(x)

#define RMAX 200
#define GMAX 200
#define BMAX 200

#define RMIN 50
#define GMIN 50
#define BMIN 50

#define N_PIEZO 6
#define N_SHROOMS N_PIEZO
#define T_CUTOFF 350
#define THRESHOLD 20

// this is ~ 8 * AVG( xMAX, xMIN)
#define AFTER_PLAY 1200

#define LIGHTS_PIN  3
#define N_LIGHTS    52

const int lights_per_piezo[] = { 12,  10, 11, 10, 10,  10 };
const int index_per_shroom[] = {  0,  12, 22, 33, 43, 53, 63 };

Adafruit_NeoPixel strip = Adafruit_NeoPixel(N_LIGHTS, LIGHTS_PIN, NEO_RGB + NEO_KHZ800);

/*  network stuff */
#ifndef MY_IP
#define MY_IP         10, 42, 16, 161
#define MY_MAC        0x02, 0x00, 0x00, 0x00, 0x00, 0x23

#define SERVER_IP     10, 42, 16, 7
#define SEND_PORT     9998
#define IN_PORT       8888
#endif

IPAddress ip(MY_IP);
byte mac[] = {MY_MAC};

EthernetUDP Udp;
IPAddress serverIP(SERVER_IP);
const int outPort = SEND_PORT;
const int inPort = IN_PORT;

/* these are small */
bool active   [N_PIEZO];
int timePlayed [N_PIEZO];
int afterPlayed [N_PIEZO];

byte R,G,B;

byte rdirection = 1;
byte gdirection = 1;
byte bdirection = 1;

void setup() 
{
  /* try to get a random seed */
  pinMode(A0, INPUT_PULLUP);
  randomSeed( analogRead(0) );

  strip.begin();
  strip.show(); 

  R = random(255);
  G = random(255);
  B = random(255);

  Ethernet.begin(mac,ip);
  Udp.begin(inPort);
  
  /* assume we're using all the analog pins */
  pinMode(A0, INPUT);
  pinMode(A1, INPUT);
  pinMode(A2, INPUT);
  pinMode(A3, INPUT);
  pinMode(A4, INPUT);
  pinMode(A5, INPUT);
   
  for( int i=0; i < N_PIEZO; ++i)
  {
    active[i] = 0;
    timePlayed[i] = 0;
    afterPlayed[i] = AFTER_PLAY;
  }

#ifdef PL
  Serial.begin(BAUD);
#endif
}

#define FADE_TIME_MAX 10
byte fadetimer = 0;
void updateRGB()
{
  fadetimer++;
  if( fadetimer%FADE_TIME_MAX == 2 )
  {
    if( R==RMAX ) rdirection = 0;
    if( R==RMIN ) rdirection = 1;

    if( G==GMAX ) gdirection = 0;
    if( G==GMIN ) gdirection = 1;

    if( B==BMAX ) bdirection = 0;
    if( B==BMIN ) bdirection = 1;

    rdirection ? R ++ : R -- ;
    gdirection ? G ++ : G -- ;
    bdirection ? B ++ : B -- ;
  }
  if( fadetimer == FADE_TIME_MAX )
  {
    fadetimer = 0;
  }
}

void updateAllLights()
{
  for(int i=0; i < N_SHROOMS; ++i)
  {
    if( active[i] )
    {
      for( int j=index_per_shroom[i]; j < index_per_shroom[i] + lights_per_piezo[i]; ++j )
      {
        strip.setPixelColor(
          j, 
          strip.Color(
            R > timePlayed[i] ? R - timePlayed[i] : 0 ,
            G > timePlayed[i] ? G - timePlayed[i] : 0 , 
            B > timePlayed[i] ? B - timePlayed[i] : 0
          ) 
        );
      }
    }
    else
    {
      if( afterPlayed[i] <= AFTER_PLAY )
      {
        for(int j=index_per_shroom[i]; j < index_per_shroom[i] + lights_per_piezo[i]; ++j)
        {
          strip.setPixelColor(
            j, 
            strip.Color(
              afterPlayed[i] / 8, 
              afterPlayed[i] / 8, 
              afterPlayed[i] / 8 
            ) 
          );
        }
        afterPlayed[i] ++;
      }
      else
      {
        for(int j=index_per_shroom[i]; j < index_per_shroom[i] + lights_per_piezo[i]; ++j)
        {
          strip.setPixelColor(j, strip.Color(R , G, B) );
        }
      }
    }
  }
}

void getPiezoReadings()
{
  for(int i=0; i < N_PIEZO; ++i)
  {
    // read first
    int hit = analogRead(i);

    P(i); P("\t:\t"); P(hit); P("\t|\t");

    if( hit > THRESHOLD)
    {
      // new hit
      if( active[i] == false )
      {
        P("[hit] ");
        PL(i);
        //send(i, hit); 
        timePlayed[i] = 0;
        active[i] = true;
      }
      else
      {
        // still ringing from previous hit
        ++ (timePlayed[i]) ;
      }
    }
    else if( active[i] == true )
    {
      ++ ( timePlayed[i] ) ;
      
      if( timePlayed[i] > T_CUTOFF )
      {
        // outlasted T_CUTOFF
        active[i] = false;
        afterPlayed[i] = 0;
      }
    }
  }
}

byte ltimer = 0;
void loop() 
{
  /* add some randomness */
  ltimer++;
  if( ltimer == 127 )
  {
    ltimer = 0;
    int color = random(0,3);
    Serial.println(color);
    if( color == 0)
      rdirection = !rdirection;
    if( color == 1)
      gdirection = !gdirection;
    if( color == 2)
      bdirection = !bdirection;
  }

  updateRGB();
  updateAllLights();
  getPiezoReadings();

  PL(" ");
  strip.show();
}
