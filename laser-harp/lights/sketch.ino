#include <Ethernet.h>
#include <EthernetUdp.h>
#include <SPI.h>    
#include <Adafruit_NeoPixel.h>

#include <OSCBundle.h>
#include <OSCBoards.h>

#define BAUD          115200
#define STRAND_MAX    150
#define LOG(x)        Serial.println(x)
#define BEGIN_SERIAL  Serial.begin(BAUD)

//#define PRINT_FREE_RAM  \
//{ \
//  extern int __heap_start, *__brkval;  \
//  int v; \
//  (int) &v - (__brkval == 0 ? LOG( (int) &__heap_start ) : LOG( (int) __brkval) );  \
//}
;

#define CHASE_OFFSET 127
#define N_CHASE 64

Adafruit_NeoPixel strip_1 = Adafruit_NeoPixel(2, 6, NEO_RGB + NEO_KHZ800);

EthernetUDP Udp;
static byte mac[] = { 0x02, 0x00, 0x00, 0x00, 0x00, 0xA0 };
const unsigned int inPort = 9999;
IPAddress ip(10, 42, 16, 167);

byte R, G, B;
unsigned int _height = 0;
bool _all_on = false;
void black()  { _height = STRAND_MAX; R=G=B=0; _all_on = false; }
void black(OSCMessage &msg, int addrOffset) { black();}

void white() { _height = STRAND_MAX; R=G=B=255;  _all_on = true; }
void white(OSCMessage &msg, int addrOffset) { white();}

void all_swap() { _all_on ? black() : white(); }

int _color;
void setHeight(OSCMessage &msg, int addrOffset)
{
  int tmp;
  LOG("in height");
  if( msg.isInt(0))
  {
    tmp = msg.getInt(0);
    if( tmp < 0 || tmp > STRAND_MAX)
      _height = 0;
    else
      _height = tmp;
  }
}

byte _or, _og, _ob; 
bool _fadeDown = false;
bool _fadeUp = false;
float _fadeMod = 0.99;
void updateFade()
{
  LOG("updating fade");
  if( _fadeDown)
  {
    if(R > _or)
      R*=_fadeMod;
    if(G > _og)
      G*=_fadeMod;
    if(B > _ob)
      B*=_fadeMod;
    
    if(R <= _or && 
      G <= _og &&
      B <= _ob)
      _fadeDown = false;
  }
  else if( _fadeUp)
  {
    if(R < _or)
      R/=_fadeMod;
    if(G < _og)
      G/=_fadeMod;
    if(B < _ob)
      B/=_fadeMod;
    
    if(R >= _or && 
      G >= _og &&
      B >= _ob)
      _fadeUp = false;
  }
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte w_pos, Adafruit_NeoPixel &strip )
{
  w_pos = 255 - w_pos;
  if(w_pos < 85) 
  {
    return strip.Color(255 - w_pos * 3, 0, w_pos * 3);
  }
  else if(w_pos < 170) 
  {
    w_pos -= 85;
   return strip.Color(0, w_pos * 3, 255 - w_pos * 3);
  } 
  else 
  {
    w_pos -= 170;
    return strip.Color(w_pos * 3, 255 - w_pos * 3, 0);
  }
}

void setColor( int c)
{
  c = map(c, 0, 1023, 0, 2040);
  R = G = B = 0;
  if(c >= 0 && c <=255)
    R=c;
  else if(c > 255 && c <= 510)
    { R=255; G=c-255; }
  else if(c > 510 && c <= 765)
    { R= 255 - (c - 510); G=255;  }
  else if(c >765 && c <= 1020)
    { R=0; G=255; B=c-765;  }
  else if(c >1020 && c <= 1275)
    { R=0; G= 255 - (c - 1020); B=255;  }
  else if(c >1275 && c <= 1530)
    { R=c-1275; B=255;  }
  else if(c >1530 && c <= 1785)
    { R=255; G=0; B= 255-(c-1530);  }
  else if(c >1785 && c <= 2040)
    { R=255; G= c-1785; B=c-1785; }
}

void addRed( int c)  { R +=c;  }
void addRed(OSCMessage &msg, int addrOffset ) {
  if( msg.isInt(0) )
    addRed( msg.getInt(0) );
}

void addGreen( int c)  { G += c; }
void addGreen(OSCMessage &msg, int addrOffset ) {
  if( msg.isInt(0) )
    addGreen( msg.getInt(0) );
}

void addBlue( int c) { B += c; }
void addBlue(OSCMessage &msg, int addrOffset )  {
  if( msg.isInt(0) )
    addBlue( msg.getInt(0) );
}

void setRed(OSCMessage &msg, int addrOffset )
{
  LOG("set red");
  if( msg.isInt(0) )
    R = msg.getInt(0);
}
void setGreen(OSCMessage &msg, int addrOffset )
{
  LOG("set green");
  if( msg.isInt(0) )
    G = msg.getInt(0);
}
void setBlue(OSCMessage &msg, int addrOffset )
{
  LOG("set blue");
  if( msg.isInt(0) )
    B = msg.getInt(0);
}

void setFadeMod(OSCMessage &msg, int addrOffset)
{
  if( msg.isFloat(0) )
  {
    _fadeMod = msg.getFloat(0);
  }
}

byte _pulseOffset = 128;
void pulseUp(OSCMessage &msg, int addrOffset)
{
  // starting a fade down
  if( !_fadeDown)
  {
    _fadeDown = true;
    _or = R;
    _og = G;
    _ob = B;
  }

  LOG("in pulse");
  if( msg.isInt(0))
    _pulseOffset = msg.getInt(0);
  else
    _pulseOffset = 2;

  if( msg.isFloat(1) )
    _fadeMod = msg.getFloat(1);

if( msg.isInt(2) && msg.isInt(3) && msg.isInt(4) )
  {
    addRed( msg.getInt(2));
    addGreen( msg.getInt(3));
    addBlue( msg.getInt(4));
  }

  ( R * _pulseOffset < 255 ) ?
    R *= _pulseOffset : R=255;
  ( G * _pulseOffset < 255 ) ?
    G *= _pulseOffset : G=255;
  ( B * _pulseOffset < 255 ) ?
    B *= _pulseOffset : B=255;
  LOG("R: ");
  LOG(R);
  LOG("G: ");
  LOG(G);
  LOG("B: ");
  LOG(B);
}

void setAll(OSCMessage &msg, int addrOffset )
{
  LOG("in color");
  if( msg.isInt(0) )
  {
    setColor(msg.getInt(0) );
  }
}

byte _current[N_CHASE];
byte _chase_color[N_CHASE*3];
void checkChase(byte i)
{
  unsigned int ii = i*2;
  for(int k = 0; k < N_CHASE; ++k)
  {
    if( i == _current[k] && i != 0)
    {
      byte r2 = _chase_color[k*3];
      byte g2 = _chase_color[k*3 + 1];
      byte b2 = _chase_color[k*3 + 2];
      strip_1.setPixelColor(i, strip_1.Color(r2, g2, b2) );
    }
  }
}

void startChase(OSCMessage &msg, int addrOffset)
{
  for(int k=0; k < N_CHASE; ++k)
  {
    if(_current[k] == 0)
    {
      // enable this chase
      _current[k] = 1;
      if( msg.isInt(0) && msg.isInt(1) && msg.isInt(2) )
      {
        _chase_color[k*3] = msg.getInt(0);
        _chase_color[(k*3)+ 1] = msg.getInt(1);
        _chase_color[(k*3)+ 2] = msg.getInt(2);
      }
      else
      {
        // give it default color
        _chase_color[k*3] = CHASE_OFFSET;
        _chase_color[(k*3)+1] = CHASE_OFFSET;
        _chase_color[(k*3)+2] = CHASE_OFFSET;
      }
      return;
    }
  }
}

void updateAll()
{
  for(unsigned int i=0, ii=0; i < _height; ++i)
  {
    ii = i*2;
    strip_1.setPixelColor(i, strip_1.Color(R,G,B) );
    checkChase(i);
  }
  for( unsigned int i=_height, ii=_height; i < STRAND_MAX; ++i)
  {
    ii = i*2;
    strip_1.setPixelColor(i, strip_1.Color(0,0,0) );
    checkChase(i);
  }
  strip_1.show();
}

bool _update_all = true;
byte _pos = 0;
void updateInd()
{
  int32_t tmp_c;
  _pos++;
  LOG("start");
  for(unsigned int i=0, ii=0; i<STRAND_MAX; i++) 
  {
    ii=i*2;
    tmp_c = Wheel(((i * 256 / STRAND_MAX) + _pos) & 255, strip_1);
    strip_1.setPixelColor((i)%STRAND_MAX, tmp_c);
    checkChase((i)%STRAND_MAX);
  }
  LOG("before show");
  strip_1.show();
}

void toggleInd (OSCMessage &msg, int addrOffset)
{
  _update_all  = (_update_all ) ? false : true;
  //if( _update_all)
  //  LOG("updating all");
  //else
  //  LOG("updating individually");
  updateInd();
}

void setup() 
{
  strip_1.begin();
  strip_1.show(); 

  Ethernet.begin(mac,ip);
  Udp.begin(inPort);
  
  BEGIN_SERIAL;
  
  R=G=B=0;
  _height = STRAND_MAX;
  for(int i=0; i < N_CHASE; ++i)
    _current[i] = 0;

  for(int i=0; i < N_CHASE*3; ++i)
    _chase_color[i] = 127;
  LOG("done setup");
}

void loop() 
{
  for(int k=0; k<N_CHASE; ++k)
  {
    if( _current[k] < STRAND_MAX )
    {
      if(_current[k] != 0)
      ++ _current[k];
    }
    else
      _current[k] = 0;
  }

  if( _fadeUp || _fadeDown )
    updateFade();
  
   _update_all ? updateAll() : updateInd();

  //LOG("[mem] ");
  //PRINT_FREE_RAM;
  OSCMessage msgIn;
  int s;
  if( (s = Udp.parsePacket() ) > 0)
  {
    LOG("got message!");
    while(s--)
      msgIn.fill(Udp.read());
    
    if(!msgIn.hasError())
    {
      //LOG("[ mem(2) ] ");
      //PRINT_FREE_RAM;
      msgIn.route("/flood/height", setHeight);
      msgIn.route("/flood/pulseUp", pulseUp);
      msgIn.route("/flood/setFM", setFadeMod);
      msgIn.route("/flood/chase", startChase);
      msgIn.route("/flood/ind", toggleInd);

      msgIn.route("/flood/addR", addRed);
      msgIn.route("/flood/addG", addGreen);
      msgIn.route("/flood/addB", addBlue);

      msgIn.route("/flood/setR", setRed);
      msgIn.route("/flood/setG", setGreen);
      msgIn.route("/flood/setB", setBlue);

   }
    else
    {
      LOG("[OSC ERROR] --- MSG_HAS_ERROR");
      all_swap();
    }
  }
}
