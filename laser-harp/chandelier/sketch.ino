#include <OSCMessage.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <SPI.h>    

/* make debugging easy */
#define DEBUG(x) Serial.print(x)
#define DEBUGL(x) Serial.println(x)

/* there are only 3 relays for the demo 
*  but they're spaced out "every other one"
*/
#define N_RELAYS      24
#define RELAYS_BEGIN  26
#define RELAYS_END RELAYS_BEGIN + N_RELAYS

/* we'll give ourselves 1 bit of wiggle room */
#define POWER_SUPPLY_THRESHOLD 1022

#define MY_IP   10, 42, 16, 254
#define MY_MAC  0x02, 0x00, 0x00, 0xFF, 0xFF, 0x22
#define RECEIVE_PORT  9999

//#define SERVER_IP   10, 42, 16, 6
//#define SEND_PORT   7778

/* we're using an obscure mac address just in case */
byte mac[] = {MY_MAC};
/* this should be special because it's critical */
IPAddress ip(MY_IP);
/*recieve port */
const unsigned int inPort = RECEIVE_PORT;
/* ethernet object */
EthernetUDP Udp;
/* keep track of each relay HI/LO */
int relayStates[N_RELAYS];

#ifdef SERVER_IP
IPAddress serverIP(SERVER_IP);
const int outPort = SEND_PORT;
#endif

void routeHigh(OSCMessage &msg, int addrOffset )
{
  if( msg.isInt(0) )
  {
    int incoming = msg.getInt(0);
    /* relays are numbered 0 through N_RELAYS */
    if( incoming < N_RELAYS && incoming >= 0 )
    {
      relayStates[incoming] = HIGH;
      DEBUG("turning on relay "); DEBUG(incoming);
      DEBUG("at pin "); DEBUGL(incoming+RELAYS_BEGIN);
    }
  }
}

void routeLow(OSCMessage &msg, int addrOffset )
{
  if( msg.isInt(0) )
  {
    int incoming = msg.getInt(0);
    /* relays are numbered 0 through N_RELAYS */
    if( incoming < N_RELAYS && incoming >= 0 )
    {
      relayStates[incoming] = LOW;
      DEBUG("turning off relay "); DEBUG(incoming);
      DEBUGL("at pin "); DEBUGL(incoming+RELAYS_BEGIN);

    }
  }
}

void setup() 
{
  /* let's start out turning on the show */
  for(int i = RELAYS_BEGIN; i < RELAYS_END; ++i)
  {
    pinMode(i, OUTPUT);
    digitalWrite(i, HIGH);
    relayStates[ i - RELAYS_BEGIN ] = HIGH;
  }
  
  Ethernet.begin(mac,ip);
  Udp.begin(inPort);

#ifdef DEBUG
  Serial.begin(115200);
#endif
}

void loop() 
{
  /* update each relay */
  for( int i = RELAYS_BEGIN; i < RELAYS_END; ++i)
  {
    digitalWrite( i, relayStates[ i - RELAYS_BEGIN ] );
  }

  OSCMessage msgIn;
  /* pretend something is available */
  int packetSize = Udp.parsePacket();
  if( packetSize > 0)
  {
    DEBUGL("recieved");
    /* read in our entire message */
    while(packetSize--)  msgIn.fill(Udp.read());
    
    /* route our commands unless an error */
    if( !msgIn.hasError() )
    {
      msgIn.route("/relay/High", routeHigh);
      msgIn.route("/relay/Low", routeLow);
    }
  }
}
