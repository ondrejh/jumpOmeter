/*
  Jump-O-Meter
*/

#define ECHO_TRIG_PIN D6
#define ECHO_RESP_PIN D7

#define MEASUREMENT_PERIOD 20 //ms
#define MESS_LONG 512 // long averaging
#define MESS_SHORT 8 // short averaging

#define BOUNCE_THR 0.9 // bounce threshold
#define BOUNCE_CNT_MIN 3
#define BOUNCE_CNT_MAX 30

uint8_t data[MESS_LONG];

// function to measure ultrasonic echo
uint8_t get_echo(void) {
  digitalWrite(ECHO_TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(ECHO_TRIG_PIN, LOW);
  uint32_t e = pulseIn(ECHO_RESP_PIN, HIGH, 0x3FFF);
  if (e == 0) return (0x3FFF >> 5);
  return (e >> 5);
}

// the setup function runs once when you press reset or power the board
void setup() {
  // init HC-SR04 pins
  pinMode(ECHO_TRIG_PIN, OUTPUT);
  digitalWrite(ECHO_TRIG_PIN, LOW);
  pinMode(ECHO_RESP_PIN, INPUT);

  // init data
  memset(data, 0, MESS_LONG);
  
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);

  // init serial
  Serial.begin(115200);
}

// the loop function runs over and over again forever
void loop() {
  static uint32_t et = 0;
  if ((millis() - et) > MEASUREMENT_PERIOD) {
    et += MEASUREMENT_PERIOD;
    uint8_t e = get_echo();
    static int p = 0;
    static uint32_t longavg = 0;
    longavg = longavg - data[p] + e;
    int pp = p - MESS_SHORT;
    if (pp < 0) pp += MESS_LONG;
    static uint32_t shortavg = 0;
    shortavg = shortavg - data[pp] + e;
    data[p++] = e;
    if (p >= MESS_LONG) p = 0;
    float lavg = (float)longavg / MESS_LONG;
    float savg = (float)shortavg / MESS_SHORT;

    static int bnc = 0;
    bool b = false;
    if (savg < (lavg * BOUNCE_THR)) {
      bnc ++;
    }
    if (savg > lavg) {
      if ((bnc >= BOUNCE_CNT_MIN) && (bnc < BOUNCE_CNT_MAX)) b = true;
      bnc = 0;
    }
    Serial.print(lavg);
    Serial.print(" ");
    Serial.print(savg);
    Serial.print(" ");
    Serial.print(e);
    if (b) Serial.print(" bounce");
    Serial.println();
  }
}
