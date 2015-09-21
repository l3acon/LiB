#include <OSCMessage.h>
#include <MuxShield.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <SPI.h>    

/* make debugging easy */
#define P(x) Serial.print(x)
#define PL(x) Serial.println(x)

#define N_LASERS  33
#define THRESHOLD 512
//#define LIGHT_THRESH_DOWN 100
//#define LIGHT_THRESH_UP   100

#ifndef MY_IP
#define MY_IP         10, 42, 16, 253
#define MY_MAC        0x02, 0x00, 0x00, 0xFF, 0xFF, 0x21
#define RECEIVE_PORT  9999

#define SERVER_IP     10, 42, 16, 7
#define SEND_PORT     9998
#endif

byte mac[] = {MY_MAC};
IPAddress ip(MY_IP);
const unsigned int inPort = RECEIVE_PORT;
EthernetUDP Udp;
IPAddress serverIP(SERVER_IP);
const int outPort = SEND_PORT;

MuxShield muxShield;

struct laser_s
{
  /*  last value from ADC */
  int previousValue;

  /*  track states */
  int state;
};

laser_s lasers[N_LASERS]; 

void sendNoteOn( int id )
{
  P("[ON] ");
  PL(id);
  OSCMessage msg("/harp/base/note/on");
  msg.add(id);
  Udp.beginPacket(serverIP, outPort);
  msg.send(Udp);   
  Udp.endPacket();
  msg.empty();
}

void sendNoteOff( int id)
{
  P("[OFF] ");
  PL(id);
  OSCMessage msg("/harp/base/note/off");
  msg.add(id);
  Udp.beginPacket(serverIP, outPort);
  msg.send(Udp);   
  Udp.endPacket();
  msg.empty();
}

void sendDebug( int id, int val)
{
  OSCMessage msg("/");
  msg.add(id);
  msg.add(val);
  Udp.beginPacket(serverIP, outPort);
  msg.send(Udp);   
  Udp.endPacket();
  msg.empty();
}

void setup()
{
#ifdef PL
  Serial.begin(115200);
#endif

  Ethernet.begin(mac,ip);
  Udp.begin(inPort);

  for(int i=0; i< N_LASERS; ++i)
  {
    lasers[i].previousValue = 0;
    lasers[i].state = 0;
  }
  
  //Set I/O 1, I/O 2, and I/O 3 as analog inputs
  muxShield.setMode(1,ANALOG_IN);
  muxShield.setMode(2,ANALOG_IN);
  muxShield.setMode(3,ANALOG_IN);

  PL("all setup");
}

void check(int begin, int end, int mux)
{
  for(int i=begin; i<end; ++i)
  {
    int m = muxShield.analogReadMS( mux, i-begin );

    if( lasers[i].state == 0 && m <= THRESHOLD )
    {
      sendNoteOff( i );
    }

    if( lasers[i].state == 1 && m >= THRESHOLD )
    {
      sendNoteOn( i );
    }

    m >= THRESHOLD ? lasers[i].state = 0 : lasers[i].state = 1;
  }
}

long previous = 0;
long current = 0;
long t1;

int k = 0;
void loop()
{
  previous = current;
  current = micros();
  t1 = current - previous;

  //P("delta = ");
  //PL( t1 );

  ///* check the first row of 16 */
  check(0, 16, 1);
  /* check the last row of 16 */
  check(16, 32, 3);
  /* get our 33rd sensor value */
  check(32, 33, 2);
}

