#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

const char* ap_ssid = "ESP1_Alina";
const char* ap_pass = "12345678";

ESP8266WebServer server(80);

const uint8_t leds[] = {D3, D5, D7}; 
const uint8_t buttonPin = D1; 

volatile bool isPaused = false;
volatile unsigned long pauseStartTime = 0;
const unsigned long pauseDuration = 15000;

int currentLed = 0;
bool reverseOrder = false; 
unsigned long lastUpdate = 0;
const int interval = 300;

ICACHE_RAM_ATTR void handleButton() {
  static unsigned long lastPress = 0;
  if (millis() - lastPress > 300) {
    isPaused = !isPaused;
    if (isPaused) pauseStartTime = millis();
    Serial.print('S'); 
    lastPress = millis();
  }
}

void setup() {
  Serial.begin(115200); 
  delay(500);

  for (int i = 0; i < 3; i++) {
    pinMode(leds[i], OUTPUT);
    digitalWrite(leds[i], LOW);
  }
  digitalWrite(leds[currentLed], HIGH);

  pinMode(buttonPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(buttonPin), handleButton, FALLING);

  // --- ВИВІД ІР ЯК НА СКРІНШОТІ (ОДИН РАЗ) ---
  Serial.println("Setting AP (Access Point)...");
  WiFi.softAP(ap_ssid, ap_pass); 
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());

  // --- ПОВЕРНУВ ОБРОБКУ ВЕБКИ ---
  
  server.on("/status", []() {
    int timeLeft = isPaused ? (15000 - (millis() - pauseStartTime)) / 1000 : 0;
    String json = "{\"currentLed\":" + String(currentLed) + 
                  ",\"paused\":" + (isPaused ? "true" : "false") + 
                  ",\"rev\":" + (reverseOrder ? "true" : "false") +
                  ",\"timeLeft\":" + String(timeLeft < 0 ? 0 : timeLeft) + "}";
    server.send(200, "application/json", json);
  });

  // Обробка кнопок з веб-інтерфейсу ESP2
  server.on("/p_stop_only", []() {
    isPaused = !isPaused;
    if (isPaused) pauseStartTime = millis();
    server.send(200, "text/plain", "OK");
  });

  server.on("/pause_both", []() {
    isPaused = !isPaused;
    if (isPaused) pauseStartTime = millis();
    Serial.print('S'); 
    server.send(200, "text/plain", "OK");
  });

  server.begin();
}

void loop() {
  server.handleClient();
  unsigned long now = millis();

  if (Serial.available() > 0) {
    char incoming = Serial.read();
    if (incoming == 'S') { 
      isPaused = !isPaused; 
      if(isPaused) pauseStartTime = now; else lastUpdate = now;
    }
    if (incoming == 'R') reverseOrder = !reverseOrder;
  }

  if (isPaused) {
    if (now - pauseStartTime >= pauseDuration) { isPaused = false; lastUpdate = now; }
    for (int i = 0; i < 3; i++) digitalWrite(leds[i], (i == currentLed) ? HIGH : LOW);
  } 
  else {
    if (now - lastUpdate >= interval) {
      lastUpdate = now;
      digitalWrite(leds[currentLed], LOW);
      if (!reverseOrder) currentLed = (currentLed + 1) % 3;
      else currentLed = (currentLed - 1 + 3) % 3;
      digitalWrite(leds[currentLed], HIGH);
    }
  }
  yield();
}