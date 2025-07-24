const int ledPin = 13;

// Morse timing
const int dotDuration = 200;
const int dashDuration = dotDuration * 3;
const int symbolSpace = dotDuration;
const int letterSpace = dotDuration * 3;
const int wordSpace = dotDuration * 7;

String morseCode(char c) {
  switch (toupper(c)) {
    case 'A': return ".-";
    case 'B': return "-...";
    case 'C': return "-.-.";
    case 'D': return "-..";
    case 'E': return ".";
    case 'F': return "..-.";
    case 'G': return "--.";
    case 'H': return "....";
    case 'I': return "..";
    case 'J': return ".---";
    case 'K': return "-.-";
    case 'L': return ".-..";
    case 'M': return "--";
    case 'N': return "-.";
    case 'O': return "---";
    case 'P': return ".--.";
    case 'Q': return "--.-";
    case 'R': return ".-.";
    case 'S': return "...";
    case 'T': return "-";
    case 'U': return "..-";
    case 'V': return "...-";
    case 'W': return ".--";
    case 'X': return "-..-";
    case 'Y': return "-.--";
    case 'Z': return "--..";
    case '1': return ".----";
    case '2': return "..---";
    case '3': return "...--";
    case '4': return "....-";
    case '5': return ".....";
    case '6': return "-....";
    case '7': return "--...";
    case '8': return "---..";
    case '9': return "----.";
    case '0': return "-----";
    default: return ""; // for space or unsupported characters
  }
}

void blinkDot() {
  digitalWrite(ledPin, HIGH);
  delay(dotDuration);
  digitalWrite(ledPin, LOW);
  delay(symbolSpace);
}

void blinkDash() {
  digitalWrite(ledPin, HIGH);
  delay(dashDuration);
  digitalWrite(ledPin, LOW);
  delay(symbolSpace);
}

void sendMorseChar(char c) {
  String code = morseCode(c);
  Serial.print(c);
  Serial.print(" : ");
  Serial.println(code); // Send Morse translation via Bluetooth

  for (int i = 0; i < code.length(); i++) {
    if (code[i] == '.') blinkDot();
    else if (code[i] == '-') blinkDash();
  }
  delay(letterSpace - symbolSpace);
}

void sendMorseMessage(String message) {
  for (int i = 0; i < message.length(); i++) {
    char c = message[i];
    if (c == ' ') {
      delay(wordSpace - letterSpace);
      Serial.println(" ");
    } else {
      sendMorseChar(c);
    }
  }
}

void setup() {
  pinMode(ledPin, OUTPUT);
  Serial.begin(9600); // HC-05 default baud rate
  delay(1000);
  Serial.println("Morse Code Transmission Starting...");
}

void loop() {
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');  // Read message until newline
    input.trim();  // Remove extra whitespace
    Serial.println("Received: " + input);
    sendMorseMessage(input);  // Send as Morse
  }
}