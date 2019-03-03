/*
loosely based on trippy ligtning's touchOSC arduino example
https://trippylighting.com/teensy-arduino-ect/touchosc-and-arduino-oscuino/
*/

#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <OSCBundle.h>

#define LED_PIN 5
#define LDR_PIN A0

bool led_on = false;
int led_val = 0;
int ldr_val = 0;

// mac address may be written on the ethernet shield somewhere, otherwise choose your own
byte mac[] = { 
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED }; 

int local_port = 8888;
int remote_port = 8888;

EthernetUDP udp;


void setup(){

  Serial.begin(9600);

  if (Ethernet.begin(mac) == 0) {
    Serial.println("connection failed");
    // stop here if failed to establish connection
    while(true);
  }

  // print your local IP address:
  Serial.print("Arduino IP address: ");
  Serial.println(Ethernet.localIP());
 
  udp.begin(local_port);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
}


void loop(){
  osc_receive();
  led_update();
  ldr_update();

  delay(10);
} 


void osc_receive(){
  
  // create empty message buffer
  OSCMessage msg_in;
  // get message length
  int size = udp.parsePacket();
  
  if(size > 0){
    // read the incoming packet into the buffer
    while(size > 0) {
      msg_in.fill(udp.read());
      size--;
    }
    if(!msg_in.hasError()) {
      // if the message is good, route it to the appropriate function
      msg_in.route("/led/onoff", led_onoff);
      msg_in.route("/led/fade", led_fade);
    }
  }
}


void led_onoff(OSCMessage &msg, int addrOffset){
  led_on = msg.getInt(0);

  if (led_on) {
    analogWrite(LED_PIN, led_val);
    Serial.println("LED on");
  } else {
    digitalWrite(LED_PIN, LOW);
    Serial.println("LED off");
  }

  // make a new osc message to reply to PD
  OSCMessage msg_out("/led/state");  
  msg_out.add(led_on);

  // send the message
  udp.beginPacket(udp.remoteIP(), remote_port);
  msg_out.send(udp);
  udp.endPacket(); // mark the end of the OSC Packet
  msg_out.empty(); // free space occupied by message
}


void led_fade(OSCMessage &msg, int addrOffset ){
  led_val = msg.getInt(0);

  Serial.print("LED val = ");
  Serial.println(led_val);
  
  OSCMessage msg_out("/led/val");  
  msg_out.add(led_val);
  
  udp.beginPacket(udp.remoteIP(), remote_port);
  msg_out.send(udp);
  udp.endPacket();
  msg_out.empty();
}


void led_update() {
  if (led_on) {
    analogWrite(LED_PIN, led_val);
  } else {
    digitalWrite(LED_PIN, LOW);
  }
}


void ldr_update() {
  ldr_val = analogRead(LDR_PIN);

  Serial.print("LDR val = ");
  Serial.println(ldr_val);

  OSCMessage msg_out("/ldr/val");  
  msg_out.add(ldr_val);
  
  udp.beginPacket(udp.remoteIP(), remote_port);
  msg_out.send(udp);
  udp.endPacket();
  msg_out.empty();
}