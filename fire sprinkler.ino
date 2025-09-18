// FireSpray MQ-Only v1.0 (UNO)
// Sensors: MQ-2 on A0, MQ-135 on A1
// Actuator: 5V single-channel relay on D7 (default assumes active-LOW; set RELAY_ACTIVE_HIGH if needed)
// Status: onboard LED (D13); optional buzzer on D8

// ---------------- Pins ----------------
const int PIN_MQ2   = A0;
const int PIN_MQ135 = A1;
const int PIN_RELAY = 7;
const int PIN_LED   = 13;
const int PIN_BUZZ  = 8;   // optional; ignore if not connected

// ---------------- Relay polarity ----------------
// If your relay turns ON when you write HIGH, set this true.
const bool RELAY_ACTIVE_HIGH = false;

// ---------------- Timing & thresholds ----------------
const unsigned long WARMUP_MS          = 60000;   // quick demo warmup; longer is better
const float         RISE_FACTOR        = 1.25;    // normal trigger vs baseline
const float         STRONG_FACTOR      = 1.60;    // strong smoke failsafe
const unsigned long PERSIST_MS         = 8000;    // smoke/aq must persist this long
const unsigned long STRONG_PERSIST_MS  = 2000;    // strong smoke must persist this long
const unsigned long LATCH_ON_MS        = 180000;  // sprinkler ON duration (3 min)
const unsigned long SAMPLE_MS          = 200;     // sampling period

// ---------------- Baseline filters ----------------
// EMA alphas: small number -> slower baseline drift (more stable)
const float ALPHA_BASELINE = 0.01;  // baseline learning rate in clean air

// ---------------- State ----------------
unsigned long t0;
unsigned long lastSample = 0;

float base2   = 0.0f;  // baseline MQ-2
float base135 = 0.0f;  // baseline MQ-135

unsigned long mq2HighSince   = 0;
unsigned long mq135HighSince = 0;
unsigned long strongSince    = 0;

bool smokeActive   = false;
bool aqActive      = false;
bool strongSmoke   = false;

bool sprinklerOn   = false;
unsigned long sprinklerOnAt  = 0;

void setRelay(bool on) {
  // Normalize to the relay's active polarity
  bool level = RELAY_ACTIVE_HIGH ? on : !on;
  digitalWrite(PIN_RELAY, level ? HIGH : LOW);
}

void setAlarm(bool on) {
  digitalWrite(PIN_LED, on ? HIGH : LOW);
  digitalWrite(PIN_BUZZ, on ? HIGH : LOW);
}

void setup() {
  pinMode(PIN_RELAY, OUTPUT);
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_BUZZ, OUTPUT);

  setRelay(false);
  setAlarm(false);

  Serial.begin(9600);
  delay(100);

  // Quick warmup countdown
  t0 = millis();
  Serial.println(F("MQ warmup..."));
  while (millis() - t0 < WARMUP_MS) {
    // Read fast and build initial baseline as simple average
    int v2 = analogRead(PIN_MQ2);
    int v135 = analogRead(PIN_MQ135);
    if (base2 == 0.0f) base2 = v2;
    if (base135 == 0.0f) base135 = v135;
    base2   = 0.99f * base2   + 0.01f * v2;
    base135 = 0.99f * base135 + 0.01f * v135;

    // tiny heartbeat on LED during warmup
    digitalWrite(PIN_LED, (millis() / 250) % 2);
    delay(50);
  }
  digitalWrite(PIN_LED, LOW);

  Serial.println(F("Warmup done. Baselines:"));
  Serial.print(F("  MQ-2:   "));  Serial.println(base2, 1);
  Serial.print(F("  MQ-135: "));  Serial.println(base135, 1);
  Serial.println(F("Monitoring..."));
}

void loop() {
  unsigned long now = millis();

  // Sample at fixed interval
  if (now - lastSample < SAMPLE_MS) return;
  lastSample = now;

  // Read sensors
  int v2   = analogRead(PIN_MQ2);
  int v135 = analogRead(PIN_MQ135);

  // Update baseline slowly when not currently alarming
  bool systemQuiet = !sprinklerOn && !smokeActive && !aqActive && !strongSmoke;
  if (systemQuiet) {
    base2   = (1.0f - ALPHA_BASELINE) * base2   + ALPHA_BASELINE * v2;
    base135 = (1.0f - ALPHA_BASELINE) * base135 + ALPHA_BASELINE * v135;
  }

  // Compute thresholds
  float thr2   = base2   * RISE_FACTOR;
  float thr135 = base135 * RISE_FACTOR;
  float thr2S  = base2   * STRONG_FACTOR;

  // Persistence logic for each condition
  // MQ-2 regular smoke
  if (v2 >= thr2) {
    if (mq2HighSince == 0) mq2HighSince = now;
    smokeActive = (now - mq2HighSince >= PERSIST_MS);
  } else {
    mq2HighSince = 0;
    smokeActive = false;
  }

  // MQ-135 air quality rise
  if (v135 >= thr135) {
    if (mq135HighSince == 0) mq135HighSince = now;
    aqActive = (now - mq135HighSince >= PERSIST_MS);
  } else {
    mq135HighSince = 0;
    aqActive = false;
  }

  // Strong smoke (failsafe on MQ-2 alone)
  if (v2 >= thr2S) {
    if (strongSince == 0) strongSince = now;
    strongSmoke = (now - strongSince >= STRONG_PERSIST_MS);
  } else {
    strongSince = 0;
    strongSmoke = false;
  }

  // Decide to turn sprinkler ON
  bool shouldTrigger = (smokeActive && aqActive) || strongSmoke;

  if (shouldTrigger && !sprinklerOn) {
    sprinklerOn = true;
    sprinklerOnAt = now;
    setRelay(true);
    setAlarm(true);
    Serial.println(F("=== SPRINKLER: ON (latched) ==="));
  }

  // Latch timing & release
  if (sprinklerOn) {
    if (now - sprinklerOnAt >= LATCH_ON_MS) {
      // After latch, if conditions have cleared, turn off
      if (!shouldTrigger) {
        sprinklerOn = false;
        setRelay(false);
        setAlarm(false);
        Serial.println(F("=== SPRINKLER: OFF ==="));
      } else {
        // Extend latch if still bad
        sprinklerOnAt = now;
      }
    }
  } else {
    // idle heartbeat blink
    digitalWrite(PIN_LED, (now / 1000) % 2);
  }

  // Debug stream for tuning (open Serial Monitor @9600)
  Serial.print(F("MQ2="));   Serial.print(v2);
  Serial.print(F(" (base "));Serial.print(base2, 1);   Serial.print(F(")"));
  Serial.print(F(" | MQ135=")); Serial.print(v135);
  Serial.print(F(" (base "));Serial.print(base135, 1); Serial.print(F(")"));
  Serial.print(F(" | flags [S=")); Serial.print(smokeActive);
  Serial.print(F(", A="));         Serial.print(aqActive);
  Serial.print(F(", STR="));       Serial.print(strongSmoke);
  Serial.print(F("] | RELAY="));   Serial.println(sprinklerOn);
}
