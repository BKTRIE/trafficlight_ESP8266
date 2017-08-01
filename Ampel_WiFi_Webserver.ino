#include <ESP8266WiFi.h>
#include <Adafruit_NeoPixel.h>

#define PIN 2                        // change if you use another port

const char* ssid     = "Ampel";      // change if you want another WiFi-Name
const char* password = "12345678";   // change or leave it empty "" 

unsigned long urlZugriff    = 0;
unsigned long timer         = 0;
unsigned long timerRainbow  = 0;
unsigned long timerSleep    = 0;

int lamps            = 8;            // change to the number of Lamps you are using (10 is maximum for now)
int pixPerLamps      = 3;            // change to the number of ws2812b pixels you are using per lamp
int pixels           = pixPerLamps * lamps;
int Leuchtdauer      = 500;
int basicTime        = 1000;


byte neueZeit        = 0;
byte phasenZeit      = 10;
byte farbe           = 127;
byte helligkeit      = 255;
byte ledMode         = 1;
byte runner          = 0;
byte rainbow         = 0;
byte sleeper         = 0;
byte phasen          = 6;
byte blaulicht       = 0;
byte gpio_pin        = 4;
byte oggmode         = 0;

boolean automatik    = true;
boolean police       = false;
boolean toNextGreen  = false;

// Create an instance of the server on Port 80
WiFiServer server(80);

Adafruit_NeoPixel strip = Adafruit_NeoPixel(pixels, PIN, NEO_GRB + NEO_KHZ800);


void setup()
{
  pinMode(gpio_pin, OUTPUT);
  digitalWrite(gpio_pin, LOW);
  urlZugriff = 0;
  Serial.begin(115200);
  delay(1);
  // AP mode
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);
  server.begin();
  strip.begin();
  strip.show(); 
}


void WiFiStart()
{
  Serial.print("Erstelle Netzwerk :");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("Netzwerk Erstellt");
  server.begin();
  Serial.println("Server gestartet");
  Serial.print("Erreichbar unter :");
  Serial.println(WiFi.localIP());
}

void loop() {
  webserver();
  long now = millis();
  if (now - timer > Leuchtdauer ) {
    timer = now;
    if (!police) {
      if (automatik || toNextGreen) {
        phase(runner);
        runnerIncr();
      }
    } else {
      Leuchtdauer = 100;
      blaulicht = (blaulicht + 1) % 4;
      blackOut();
      blaulichtleuchten();
      stripshow();
    }
  }
  if (now - timerRainbow > phasenZeit * 10 ) {
    timerRainbow = now;
    //Serial.println(rainbow++);
    rainbow++;
    if (oggmode > 1) {
      oggMode();
      //if (sleeper != 0) {
      //  Serial.print("Zeit bis Schlaf :");
      //  Serial.println((sleeper * 10  * 60) - ((now - timerSleep) / 1000));
      //}
    }
  }
  if ( sleeper > 0 && now - timerSleep > (sleeper * 10 * 1000 * 60) ) {
    if (helligkeit >= 1)
      helligkeit --;
    else
      sleeper = 0;
    timerSleep += 100;
    Serial.print("sleep : ");
    Serial.println(helligkeit);
    stripshow();
  }
}


void webserver() {
  // Check if a client has connected
  WiFiClient client = server.available();
  if (!client)
  {
    digitalWrite(gpio_pin, HIGH);
    return;
  }

  // Wait until the client sends some data
  Serial.println("new client");
  digitalWrite(gpio_pin, LOW);
  unsigned long ultimeout = millis() + 250;
  while (!client.available() && (millis() < ultimeout) )
  {
    delay(1);
  }
  if (millis() > ultimeout)
  {
    Serial.println("client connection time-out!");
    return;
  }
  String sRequest = client.readStringUntil('\r');
  Serial.println(sRequest);
  client.flush();
  if (sRequest == "")
  {
    Serial.println("empty request! - stopping client");
    client.stop();
    return;
  }
  String sStart = "GET /?";
  String sEnde  = "HTTP/1.1";
  String word1    = "pin=";
  String word2    = "time=";
  String word3    = "farbe=";
  String word4    = "hellig=";
  String Param  = sRequest.substring(sRequest.indexOf(sStart) + sStart.length(), sRequest.indexOf(sEnde));
  if (Param.indexOf(word1) == 0) {
    Param = Param.substring(word1.length(), Param.length() - 1);
    command(Param);
  }
  if (Param.indexOf(word2) > 0) {
    int i = Param.indexOf(word2) + word2.length();
    phasenZeit = Param.substring(i, i + 3).toInt() - 100;
    i = Param.indexOf(word3) + word3.length();
    farbe = Param.substring(i, i + 3).toInt() - 100;
    i = Param.indexOf(word4) + word4.length();
    helligkeit = Param.substring(i, i + 3).toInt() - 100;
    if (oggmode != 0)
      oggMode();
  }

  String website, sHeader;

  // build the html page //

  urlZugriff++;
  timerSleep = millis();
  website  = "<html><head><title>Ampelsteuerung</title></head><body>";
  website += "<style>     .bunt {  background-image: linear-gradient(to right, LawnGreen, yellow , red, Magenta ,RoyalBlue , Aqua , LawnGreen );} ";
  website += "            .sw {  background-image: linear-gradient(to right, black, white );}</style>";
  website += "<font color=\"#FFFFF0\"><body bgcolor=\"#151B54\">";
  website += "<FONT SIZE=-1>";
  website += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=yes\">";
  website += "<center><h1>Ampel UI</h1></center>";
  //website += "<br>";
  website += "<FONT SIZE=+1>";
  website += "<table style=\"width:100%; color: #fff\">";
  website += "<tr>";
  website += "<td style=\"width:30%\">Automatik</td>";
  if (!automatik)
    website += "<td style=\"width:30%\" ><a href=\"?pin=AutoON\"><button style=\"width:100%; height:50px;  background-color: #AF4C50 \" ><font farbe=\"#FFF\">AUS</font></button></a></td>";
  else
    website += "<td style=\"width:30%\" ><a href=\"?pin=AutoOFF\"><button style=\"width:100%; height:50px; background-color: #4CAF50\" ><font farbe=\"#000\"><b>AN</b></font></button></a></td>";
  website += "<td style=\"width:30%\" ><a href=\"?pin=nextGreen\"><button style=\"width:100%; height:50px\">Gruen</button></a></td>";
  website += "</tr><tr>";
  website += "<td>Schritt</td> <td><a href=\"?pin=stepBack\"><button style=\"width:100%; height:50px\">(-)</button></a></td><td><a href=\"?pin=stepForw\"><button style=\"width:100%; height:50px\">(+)</button></a></td>";
  website += "</tr><tr><td></td><td><br><center>Modi</center></td><td></td></tr><tr>";
  website += "<td><a href=\"?pin=oggMode\"><button style=\"width:100%; height:50px\">Ogg</button></a></td> <td><a href=\"?pin=ampelOn\"><button style=\"width:100%; height:50px\">Ampel</button></a></td><td><a href=\"?pin=blauOn\"><button style=\"width:100%; height:50px\">Polizei</button></a></td>";
  website += "</tr><tr>";
  website += "<td><a href=\"?pin=sleep\"><button style=\"width:100%; height:50px\">Sleep(";
  website += sleeper * 10;
  website += ")</button></a></td> <td><a href=\"?pin=rain1\"><button style=\"width:100%; height:50px\">Rainbow I</button></a></td><td><a href=\"?pin=rain2\"><button style=\"width:100%; height:50px\">Rainbow II</button></a></td>";
  website += "</tr>";
  website += "</table>";
  website += "</p>";
  website += "<form action=\"?sCmd\" >";    // ?sCmd forced the '?' at the right spot
  website += "<BR>Schaltzeit &nbsp;&nbsp";  // perhaps we can show here the current value
  website += phasenZeit;   // this is just a scale depending on the max value; round for better readability
  website += " sekunden";
  website += "<BR>";
  website += "<input style=\"width:100%; height:50px\" type=\"range\" name=\"=time\" id=\"cmd\" value=\"";   // '=' in front of FUNCTION_200 forced the = at the right spot
  website += phasenZeit + 100;
  website += "\" min=105 max=160 step=5 onchange='this.form.submit()' />";
  //website += "<BR>";
  website += "<div class=\"bunt\" style=\"width:100%\"><input style=\"width:100%; height:50px\" type=\"range\" name=\"=farbe\" id=\"cmd\" value=\"";   // '=' in front of FUNCTION_200 forced the = at the right spot
  website += farbe + 100;
  website += "\" min=100 max=355 step=5 onchange='this.form.submit()' /></div>";
  //website += "<BR>";
  website += "<div class=\"sw\" style=\"width:100%\"><input style=\"width:100%; height:50px\" type=\"range\" name=\"=hellig\" id=\"cmd\" value=\"";   // '=' in front of FUNCTION_200 forced the = at the right spot
  website += helligkeit + 100;
  website += "\" min=100 max=355 step=5 onchange='this.form.submit()' /></div>";
  website += "<noscript<input type=\"submit\"></noscript";
  website += "</form>";
  website += "<p>";
  website += "<FONT SIZE=-1>";
  website += "<BR>";
  website += "<FONT SIZE=-2>";
  website += "<font color=\"#FFDE00\">";
  website += "Assambled by Christian Rietzscher ;-)<BR>";
  website += "thanks to Mark Kriegsman<BR>";
  website += "Webserver by Stefan Thesen<BR>";
  website += "<font color=\"#FFFFF0\">";
  website += "</body></html>";
  sHeader  = "HTTP/1.1 200 OK\r\n";
  sHeader += "Content-Length: ";
  sHeader += website.length();
  sHeader += "\r\n";
  sHeader += "Content-Type: text/html\r\n";
  sHeader += "Connection: close\r\n";
  sHeader += "\r\n";

  client.print(sHeader);
  client.print(website);
  client.stop();
  Serial.println("Client disonnected");
}

void sleeperIncr() {
  sleeper = (sleeper + 1) % 7;
}

void runnerIncr() {
  runner = (runner + 1) % phasen;
}

void runnerDecr() {
  runner = (runner + phasen - 1) % phasen;
}

void gruen1() {
  setLampfarbe(0, 0, 255, 0);
  setLampfarbe(5, 0, 255, 0);
}

void gelb1() {
  setLampfarbe(1, 200, 150, 0);
  setLampfarbe(6, 200, 150, 0);
}

void rot1() {
  setLampfarbe(2, 255, 0, 0);
  setLampfarbe(7, 255, 0, 0);
}

void gruen2() {
  setLampfarbe(4, 0, 255, 0);
  setLampfarbe(9, 0, 255, 0);

}

void rot2() {
  setLampfarbe(3,  255, 0, 0);
  setLampfarbe(8, 255, 0, 0);
}

void setLampfarbe(int _lamp, byte _red, byte _green, byte _blue) {
  for (int i = 0; i < pixPerLamps; i++) {
    strip.setPixelColor((_lamp * pixPerLamps) + i,  _red, _green, _blue);
  }
}

void phase(byte input) {
  blackOut();
  switch (input) {
    case 0:
      gruen1();
      rot2();
      Leuchtdauer = basicTime * phasenZeit;
      toNextGreen = false;
      break;
    case 1:
      gelb1();
      rot2();
      Leuchtdauer = basicTime;
      break;
    case 2:
      rot1();
      rot2();
      Leuchtdauer = basicTime;
      break;
    case 3:
      rot1();
      gruen2();
      Leuchtdauer = basicTime * phasenZeit;
      toNextGreen = false;
      break;
    case 4:
      rot1();
      rot2();
      Leuchtdauer = basicTime;
      break;
    case 5:
      rot1();
      gelb1();
      rot2();
      Leuchtdauer = basicTime;
      break;
    default:
      // if nothing else matches, do the default
      // default is optional
      break;
  }
  stripshow();
}

void blackOut() {
  uint8_t pixel, runden;
  for (pixel = 0; pixel < strip.numPixels(); pixel++) {
    strip.setPixelColor(pixel, 0, 0, 0);
  }
}

void blaulichtleuchten() {
  switch (blaulicht) {
    case 0:
      setLampfarbe(0, 0, 0, 255);
      setLampfarbe(1, 0, 0, 255);
      setLampfarbe(9, 0, 0, 100);
      setLampfarbe(8, 0, 0, 100);
      break;
    case 1:
      setLampfarbe(0, 0, 0, 100);
      setLampfarbe(1, 0, 0, 100);
      setLampfarbe(6, 0, 0, 255);
      setLampfarbe(7, 0, 0, 255);
      break;
    case 2:
      setLampfarbe(6, 0, 0, 100);
      setLampfarbe(7, 0, 0, 100);
      setLampfarbe(3, 0, 0, 255);
      setLampfarbe(4, 0, 0, 255);
      break;
    default:
      setLampfarbe(3, 0, 0, 100);
      setLampfarbe(4, 0, 0, 100);
      setLampfarbe(9, 0, 0, 255);
      setLampfarbe(8, 0, 0, 255);
      break;
  }
}

void command(String _test) {
  // switch the animiation (based on your choice in the case statement (main loop)
  if ( _test == "AutoON" )
  {
    Serial.println("Auto ein");
    automatik = true;
  }
  if ( _test == "AutoOFF" )
  {
    Serial.println("Auto aus");
    automatik = false;
  }
  if ( _test == "stepBack" )
  {
    Serial.println("stepBack");
    runnerDecr();
    phase(runner);
  }
  if ( _test == "stepForw" )
  {
    Serial.println("stepForw");
    runnerIncr();
    phase(runner);
  }
  if ( _test == "sleep" )
  {
    Serial.println("sleep");
    sleeperIncr();
  }

  if ( _test == "blauOn" )
  {
    Serial.println("blauOn");
    police = true;
    oggmode = 0;
    Leuchtdauer = 100;
  }
  if ( _test == "ampelOn" )
  {
    Serial.println("ampelOn");
    police = false;
    oggmode = 0;
    Leuchtdauer = 100;
    automatik = true;
    runner = 4;
  }
  if ( _test == "nextGreen" )
  {
    Serial.println("nextGreen");
    oggmode = 0;
    police = false;
    automatik = false;
    toNextGreen = true;
    Leuchtdauer = 100;
  }
  if ( _test == "oggMode" )
  {
    Serial.println("Ogg Mode");
    police = false;
    automatik = false;
    toNextGreen = false;
    oggmode = 1;
    Leuchtdauer = 100;
    oggMode() ;
  }
  if ( _test == "rain1" )
  {
    Serial.println("Rainbow I");
    police = false;
    automatik = false;
    toNextGreen = false;
    oggmode = 2;
    Leuchtdauer = 100;
    //oggMode() ;
  }
  if ( _test == "rain2" )
  {
    Serial.println("Rainbow II");
    police = false;
    automatik = false;
    toNextGreen = false;
    oggmode = 3;
    Leuchtdauer = 100;
    //oggMode() ;
  }
}

void oggMode() {
  uint16_t i;
  byte temp = farbe;
  if (oggmode == 2)
    temp = rainbow;
  for (i = 0; i < strip.numPixels(); i++) {
    if (oggmode == 3)
      temp = rainbow + (i * 255 / strip.numPixels());
    strip.setPixelColor(i, Wheel(temp));
  }
  stripshow();
}

uint32_t Wheel(byte WheelPos) {
  if (WheelPos < 85) {
    return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  }
  else if (WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  else {
    WheelPos -= 170;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
}

void stripshow() {
  strip.setBrightness(helligkeit);
  strip.show();
}
