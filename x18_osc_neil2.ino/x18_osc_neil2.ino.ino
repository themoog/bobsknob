#include <ESP8266WiFi.h>
#include <WiFiUdp.h>          // For WiFi
#include <OSCMessage.h>       // For OSC support
#include <Button2.h> //  https://github.com/LennartHennigs/Button2
#include <ESPRotary.h>;

#ifndef STASSID
#define STASSID "XR18-23-FC-CB"
#define STAPSK  ""
#endif

WiFiUDP Udp;

const IPAddress outIp(192, 168, 1, 1);    // IP of the XR18 in Comma Separated Octets, NOT dots!
const unsigned int outPort = 10024;         // remote port to receive OSC X-AIR is 10024, X32 is 10023
const unsigned int localPort = 8888;        // local port to listen for OSC packets (actually not used for sending)

uint8_t LED_CONNECTED = D5; //14
uint8_t LED_MUTE = D6; //12
//uint8_t REV_POWER = D4; //2

#define ROTARY_PIN1 4 //d2
#define ROTARY_PIN2 5 //d1
#define BUTTON_PIN  0 // d3

#define ROTARY_REV_PIN1 10 // d7
#define ROTARY_REV_PIN2 9 // d8
//#define BUTTON_REV_PIN  0

//uint8_t LED_CONNECTED = SDD9;
//uint8_t LED_MUTE = SDD10;
////uint8_t REV_POWER = D4; 2
//
//#define ROTARY_PIN1 4 //d2
//#define ROTARY_PIN2 5 //d1
//#define BUTTON_PIN  14 // d3
//
//#define ROTARY_REV_PIN1 0 // d7
//#define ROTARY_REV_PIN2 2 // d8
////#define BUTTON_REV_PIN  0

boolean muteToggle = false;
boolean revToggle = false;


int faderLevel = 0;
int faderLevelRev = 0;

ESPRotary r = ESPRotary(ROTARY_PIN1, ROTARY_PIN2);
Button2 b = Button2(BUTTON_PIN);

ESPRotary r2 = ESPRotary(ROTARY_REV_PIN1, ROTARY_REV_PIN2);
//Button2 b2 = Button2(BUTTON_REV_PIN);

/////////////////////////////////////////////////////////////////

void setup() {
  pinMode(LED_CONNECTED, OUTPUT);   // Initialize the LED pin as an output
  pinMode(LED_MUTE, OUTPUT);   // Initialize the LED pin as an output
//  pinMode(REV_POWER, OUTPUT);   // Power up the second reverb pot
  delay(500);
//  digitalWrite(REV_POWER, HIGH);

  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(STASSID, STAPSK);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(500);
  }
  Serial.print("\n\nConnected! IP address: ");
  Serial.println(WiFi.localIP());
  //digitalWrite(LED_CONNECTED, HIGH); // Turn the LED on
  ledStatusBlue(1023);
  Serial.printf("\n\nUDP server on port %d\n", localPort);
  Udp.begin(localPort);

  Serial.println("\n\nBobs Knob");

  r.setLeftRotationHandler(directionL);
  r.setRightRotationHandler(directionR);

  r2.setLeftRotationHandler(directionL2);
  r2.setRightRotationHandler(directionR2);

  b.setLongClickHandler(muteButton);


  muteOn(true, "01");
  setLevel(20, "01");
  muteToggle = true;


}


/////////////////////////////////////////////////////////////////

void loop() {

  r.loop();
  r2.loop();
  b.loop();

}

/////////////////////////////////////////////////////////////////

// on left or right rotation (channel level)
void directionL(ESPRotary& r) {

  if (muteToggle == false and faderLevel <= 1000) {
    faderLevel = faderLevel + 20;
    Serial.println(faderLevel);

    OSCMessage msg("/ch/01/mix/fader");
    msg.add(faderLevel);
    Udp.beginPacket(outIp, outPort);
    msg.send(Udp);
    Udp.endPacket();
    msg.empty();
  }
}

void directionR(ESPRotary& r) {

  if (faderLevel >= 20) {
    faderLevel = faderLevel - 20;
    Serial.println(faderLevel);

    OSCMessage msg("/ch/01/mix/fader");
    msg.add(faderLevel);
    Udp.beginPacket(outIp, outPort);
    msg.send(Udp);
    Udp.endPacket();
    msg.empty();
  }
}


// on left or right rotation r2 (Reverb)
void directionL2(ESPRotary& r2) {

  if (faderLevelRev <= 150) {
    faderLevelRev = faderLevelRev + 2;
    Serial.println(faderLevelRev);
    ledStatusBlue(faderLevelRev);
    OSCMessage msg("/ch/01/mix/07/level");
    msg.add(faderLevelRev);
    Udp.beginPacket(outIp, outPort);
    msg.send(Udp);
    Udp.endPacket();
    msg.empty();
  }
}

void directionR2(ESPRotary& r2) {

  if (faderLevelRev >= 10) {
    faderLevelRev = faderLevelRev - 2;
    Serial.println(faderLevelRev);
    ledStatusBlue(faderLevelRev);
    OSCMessage msg("/ch/01/mix/07/level");
    msg.add(faderLevelRev);
    Udp.beginPacket(outIp, outPort);
    msg.send(Udp);
    Udp.endPacket();
    msg.empty();
  }
}


// long click volume
void muteButton(Button2 & btn) {

  unsigned int time = btn.wasPressedFor();

  if (time > 4000) {
    faderLevel = 0;

    OSCMessage msg("/ch/01/mix/fader");
    msg.add(faderLevel);
    Udp.beginPacket(outIp, outPort);
    msg.send(Udp);
    Udp.endPacket();
    msg.empty();

    Serial.println("Reset!");
  }
  else if (time > 500) {

    if (muteToggle == true) {

      muteOn(false, "01");
      muteToggle = false;

    }
    else if (muteToggle == false) {

      muteOn(true, "01");
      muteToggle = true;

    }
  }
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
  }
}

void setLevel (int level, String channel) {

    String url = "/ch/" + channel + "/mix/fader";

    const char * url_complete = url.c_str();   // this converts string to a const char which OSC needs.

    OSCMessage msg(url_complete);
    msg.add(level);
    Udp.beginPacket(outIp, outPort);
    msg.send(Udp);
    Udp.endPacket();
    msg.empty();
  
}

void ledStatusBlue (int value) {

  analogWrite(LED_CONNECTED,map(value,10,150,0,1023));

}
