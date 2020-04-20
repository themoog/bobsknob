
#include <MD_REncoder.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>          // For WiFi
#include <OSCMessage.h>       // For OSC support
#include <Button2.h> //  https://github.com/LennartHennigs/Button2

#ifndef STASSID
#define STASSID "XR18-23-FC-CB"
#define STAPSK  ""
#endif

WiFiUDP Udp;

const IPAddress outIp(192, 168, 1, 1);    // IP of the XR18 in Comma Separated Octets, NOT dots!
const unsigned int outPort = 10024;         // remote port to receive OSC X-AIR is 10024, X32 is 10023
const unsigned int localPort = 8888;        // local port to listen for OSC packets (actually not used for sending)

uint8_t LED_CONNECTED = D7;
uint8_t LED_MUTE = D8;

#define volButton 2

// set up encoder object
MD_REncoder vol = MD_REncoder(14, 12);
MD_REncoder rev = MD_REncoder(4, 5);
Button2 volButtonObj = Button2(volButton);


int volume = 0;
int reverb = 0;

String bobsChannel = "01";

boolean muteToggle = false;
boolean revToggle = false;

void setup()

{

  pinMode(LED_CONNECTED, OUTPUT);   // Initialize the LED pin as an output
  pinMode(LED_MUTE, OUTPUT);   // Initialize the LED pin as an output

  delay(500);


  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(STASSID, STAPSK);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(500);
  }
  Serial.print("\n\nConnected! IP address: ");
  Serial.println(WiFi.localIP());

  ledStatusBlue(1023);
  Serial.printf("\n\nUDP server on port %d\n", localPort);
  Udp.begin(localPort);

  Serial.println("\n\nBobs Knob");

  volButtonObj.setLongClickHandler(muteButton);

  vol.begin();
  rev.begin();

  setFader(0, bobsChannel);
  setLevel(0, bobsChannel, "07"); // 07 is FX1 send on XR18


}

void loop()
{
  uint8_t x = vol.read();
  uint8_t y = rev.read();

  volButtonObj.loop();

  /////////// Volume //////////////

  if (x)
  {


    if (x == DIR_CW == 0 and volume <= 900 and muteToggle == false) {
      volume = volume + 30;
      Serial.println("VOLUME UP " + String(volume));
      setFader(volume, "01");
    }
    if (x == DIR_CW == 1 and volume >= 100) {
      volume = volume - 30;
      Serial.println("VOLUME DOWN " + String(volume));
      setFader(volume, "01");
    }


  }

  /////////// Reverb //////////////

  if (y)
  {
    if (y == DIR_CW == 0 and reverb <= 150) {
      reverb = reverb + 10;
      Serial.println("REVERB UP " + String(reverb));
      setLevel(reverb, "01", "07");
    }
    if (y == DIR_CW == 1 and reverb >= 20) {
      reverb = reverb - 10;
      Serial.println("REVERB DOWN " + String(reverb));
      setLevel(reverb, "01", "07");
    }
  }



}

void ledStatusBlue (int value) {

  analogWrite(LED_CONNECTED, map(value, 10, 150, 0, 1023));

}

void setFader (int level, String channel) {

  String url = "/ch/" + channel + "/mix/fader";

  const char * url_complete = url.c_str();   // this converts string to a const char which OSC needs.

  OSCMessage msg(url_complete);
  msg.add(level);
  Udp.beginPacket(outIp, outPort);
  msg.send(Udp);
  Udp.endPacket();
  msg.empty();

}

void setLevel (int level, String channel, String bus) {

  String url = "/ch/" + channel + "/mix/" + bus + "/level";

  const char * url_complete = url.c_str();   // this converts string to a const char which OSC needs.

  OSCMessage msg(url_complete);
  msg.add(level);
  Udp.beginPacket(outIp, outPort);
  msg.send(Udp);
  Udp.endPacket();
  msg.empty();

}

void muteOn(boolean mute, String channel) {

  if (mute == true) {

    String url = "/ch/" + channel + "/mix/on";

    const char * url_complete = url.c_str();   // this converts string to a const char which OSC needs.

    OSCMessage msg(url_complete);
    msg.add("OFF");
    Udp.beginPacket(outIp, outPort);
    msg.send(Udp);
    Udp.endPacket();
    msg.empty();
    digitalWrite(LED_MUTE, HIGH); // Turn the LED on
    Serial.println("Mute ON");

    muteToggle = false;

  }
  else if (mute == false) {

    String url = "/ch/" + channel + "/mix/on";

    const char * url_complete = url.c_str();   // this converts string to a const char which OSC needs.

    OSCMessage msg(url_complete);
    msg.add("ON");
    Udp.beginPacket(outIp, outPort);
    msg.send(Udp);
    Udp.endPacket();
    msg.empty();
    digitalWrite(LED_MUTE, LOW); // Turn the LED off
    Serial.println("Mute OFF");

    muteToggle = true;
  }
}

void muteButton(Button2 & btn) {

  unsigned int time = btn.wasPressedFor();

  if (time > 4000) {
    setFader(60, "01");

    Serial.println("Reset!");
  }
  else if (time > 500) {

    if (muteToggle == true) {

      muteOn(false, "01");
      //    muteToggle = false;

    }
    else if (muteToggle == false) {

      muteOn(true, "01");
      //    muteToggle = true;

    }
  }
}
