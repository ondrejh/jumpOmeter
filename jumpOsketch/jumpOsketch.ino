/*
  Jump-O-Meter
*/

#define ECHO_TRIG_PIN D6
#define ECHO_RESP_PIN D7

#define MEASUREMENT_PERIOD 20 //ms
#define MESS_MAX 500
#define MESS_LONG 100 // long averaging
#define MESS_SHORT 10 // short averaging

#define BOUNCE_THR 0.9 // bounce threshold
#define BOUNCE_CNT_MIN 3
#define BOUNCE_CNT_MAX 30

#define BEEP false
#define BEEP_PIN D5
#define BEEP_FREQV 880
#define BEEP_DUR 100

uint8_t data[MESS_MAX];
float l_avg[MESS_MAX];
float s_avg[MESS_MAX];

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
  memset(data, 0, MESS_MAX);
  memset(l_avg, 0, MESS_MAX * sizeof(float));
  memset(s_avg, 0, MESS_MAX * sizeof(float));
  
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
      if (BEEP) tone(BEEP_PIN, BEEP_FREQV, BEEP_DUR);
    }
    Serial.println();
    
    p++;
    if (p >= MESS_MAX) p = 0;
  }
}
