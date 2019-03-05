/*
based on the ESP8266ReceiveMessage example from OSC for Arduino library
https://github.com/CNMAT/OSC/tree/master/examples/ESP8266ReceiveMessage
*/

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#else
#include <WiFi.h>
#endif
#include <WiFiUdp.h>
#include <OSCMessage.h>

char ssid[] = "********";       // your network SSID (name)
char pass[] = "********";       // your network password

WiFiUDP udp;
IPAddress remote_ip(192,168,1,96);    // destination ip for messages (commas, not decimal points!)
int remote_port = 8888;               // where to send messages to at destination
int local_port = 8888;                // where to listen for messages

#define LED_PIN 3
#define LDR_PIN A0

bool led_on = false;
int led_val = 0;
int ldr_val = 0;


void setup() {

  Serial.begin(115200);

  // Connect to WiFi network
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
  }
  Serial.println("");

  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  Serial.println("Starting UDP");
  udp.begin(local_port);
  Serial.print("Local port: ");
#ifdef ESP32
  Serial.println(local_port);
#else
  Serial.println(udp.localPort());
#endif

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

}


void loop() {

  // listen for osc messages
  osc_receive();

  // update the led state
  led_update();

  // update the ldr readings and stream these over osc
  ldr_update();
  // slow down the loop to not overload the stream
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
    Serial.println("LED on");
  } else {
    Serial.println("LED off");
  }

  // make a new osc message to reply to PD
  OSCMessage msg_out("/led/state");  
  msg_out.add(led_on);

  // send the message
  udp.beginPacket(remote_ip, remote_port);
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
  
  udp.beginPacket(remote_ip, remote_port);
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
  
  udp.beginPacket(remote_ip, remote_port);
  msg_out.send(udp);
  udp.endPacket();
  msg_out.empty();
}
