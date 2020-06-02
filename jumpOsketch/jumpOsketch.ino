/*
  Jump-O-Meter
*/

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

#include <EEPROM.h>

#include "webi.h"

#define ECHO_TRIG_PIN D6
#define ECHO_RESP_PIN D7

#define WLED BUILTIN_LED

#define MEASUREMENT_PERIOD 20 //ms
#define MESS_MAX 501
#define MESS_LONG 100 // long averaging
#define MESS_SHORT 10 // short averaging

#define BOUNCE_THR 0.9 // bounce threshold
#define BOUNCE_CNT_MIN 3
#define BOUNCE_CNT_MAX 30

#define BEEP false
#define BEEP_PIN D5
#define BEEP_FREQV 880
#define BEEP_DUR 100

#define ON LOW
#define OFF HIGH

uint8_t data[MESS_MAX];
float l_avg[MESS_MAX];
float s_avg[MESS_MAX];
bool bncs[MESS_MAX];
int p = 0;

// wifi ap settings
const char *ssid = "JumpOMeter";
const char *password = "jump0meter";

ESP8266WebServer server(80);

// function to measure ultrasonic echo
uint8_t get_echo(void) {
  digitalWrite(ECHO_TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(ECHO_TRIG_PIN, LOW);
  uint32_t e = pulseIn(ECHO_RESP_PIN, HIGH, 0x3FFF);
  if (e == 0) return (0x3FFF >> 5);
  return (e >> 5);
}

// web server handlers
void handleRoot() {
  digitalWrite(WLED, ON);
  server.send(200, "text/html", index_html);
  digitalWrite(WLED, OFF);
}

void handleData() {
  digitalWrite(WLED, ON);
  server.sendHeader("Access-Control-Max-Age", "10000");
  server.sendHeader("Access-Control-Allow-Methods", "POST,GET,OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Origin, X-Requested-With, Content-Type, Accept");
  server.sendHeader("Access-Control-Allow-Origin", "*");
  String s;
  s += "{\"x\":[";
  bool first = true;
  for (int i=0; i<MESS_MAX; i++) {
    if (first) first = false; else s += ",";
    s += String(i * MEASUREMENT_PERIOD);
  }
  s += "],\"f\":[";
  first = true;
  int pp = p;
  for (int i=0; i<MESS_MAX; i++) {
    if (first) first = false; else s += ",";
    s += String(l_avg[pp++]);
    if (pp >= MESS_MAX) pp = 0;
  }
  s += "],\"s\":[";
  first = true;
  pp = p;
  for (int i=0; i<MESS_MAX; i++) {
    if (first) first = false; else s += ",";
    s += String(s_avg[pp++]);
    if (pp >= MESS_MAX) pp = 0;
  }
  s += "],\"b\":[";
  first = true;
  pp = p;
  for (int i=0; i<MESS_MAX; i++) {
    if (first) first = false; else s += ",";
    s += bncs[pp++]?"1":"0";
    if (pp >= MESS_MAX) pp = 0;
  }
  s += "]}";
  server.send(200, "application/json", s);
  digitalWrite(WLED, OFF);
}

// the setup function runs once when you press reset or power the board
void setup() {
  // init HC-SR04 pins
  pinMode(ECHO_TRIG_PIN, OUTPUT);
  digitalWrite(ECHO_TRIG_PIN, LOW);
  pinMode(ECHO_RESP_PIN, INPUT);

  // init data
  memset(data, 0, MESS_MAX);
  memset(l_avg, 0, MESS_MAX * sizeof(float));
  memset(s_avg, 0, MESS_MAX * sizeof(float));
  
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(WLED, OUTPUT);
  digitalWrite(WLED, OFF);

  // init serial
  Serial.begin(115200);

  // start the access point
  IPAddress ip(192, 168, 42, 1);
  IPAddress subnet(255, 255, 255, 0);
  WiFi.softAPConfig(ip, ip, subnet);
  WiFi.softAP(ssid, password);
  
  MDNS.begin("esp8266");

  server.on("/", handleRoot);
  server.on("/data.json", handleData);

  server.begin();
}

// the loop function runs over and over again forever
void loop() {
  server.handleClient();
  MDNS.update();
  
  static uint32_t et = 0;
  if ((millis() - et) > MEASUREMENT_PERIOD) {
    et += MEASUREMENT_PERIOD;
    uint8_t e = get_echo();
    static uint32_t longavg = 0;
    static uint32_t shortavg = 0;
    
    int pp = p - MESS_LONG;
    if (pp < 0) pp += MESS_MAX;
    longavg = longavg - data[pp] + e;
    
    pp = p - MESS_SHORT;
    if (pp < 0) pp += MESS_MAX;
    shortavg = shortavg - data[pp] + e;

    data[p] = e;
    l_avg[p] = (float)longavg / MESS_LONG;
    s_avg[p] = (float)shortavg / MESS_SHORT;

    static int bnc = 0;
    bool b = false;
    if (s_avg[p] < (l_avg[p] * BOUNCE_THR)) {
      bnc ++;
    }
    if (s_avg[p] > l_avg[p]) {
      if ((bnc >= BOUNCE_CNT_MIN) && (bnc < BOUNCE_CNT_MAX)) b = true;
      bnc = 0;
    }
    Serial.print(l_avg[p]);
    Serial.print(" ");
    Serial.print(s_avg[p]);
    if (b) {
      Serial.print(" bounce");
      bncs[p] = 1;
      if (BEEP) tone(BEEP_PIN, BEEP_FREQV, BEEP_DUR);
    }
    else
      bncs[p] = 0;
    Serial.println();
    
    p++;
    if (p >= MESS_MAX) p = 0;
  }
}
