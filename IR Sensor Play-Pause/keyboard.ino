#define IR_SENSOR_PIN 21  // GPIO where sensor OUT is connected



void setup() {

  Serial.begin(115200);

  pinMode(IR_SENSOR_PIN, INPUT);

  Serial.println("IR YouTube control ready...");

}



void loop() {

  int sensorValue = digitalRead(IR_SENSOR_PIN);



  if (sensorValue == LOW) {  

    // IR detected → trigger play/pause

    Serial.println("PLAY_PAUSE");

    delay(500);  // debounce, so it doesn’t spam

  }

}