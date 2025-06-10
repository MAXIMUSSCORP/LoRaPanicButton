#include <SPI.h>
#include <LoRa.h>

// LoRa settings, Set frequency to 433 MHz, check local law regulations for allowed frequencies
#define LORA_BAND 433E6

const int buttonPin = 4;
const uint8_t DEVICE_ID = 1; // Change for each sender node (1‑15)

// Triple press detection variables
int pressCount = 0;
unsigned long lastPressTime = 0;
unsigned long lastDebounceTime = 0;
bool lastButtonState = HIGH;
bool buttonState = HIGH;

// Timing constants
const unsigned long debounceDelay = 50; // 50ms debounce
const unsigned long tripleClickWindow = 1200; // 1.2 seconds for triple click

void setup() {
  Serial.begin(9600);
  // Initialize the button pin
  pinMode(buttonPin, INPUT_PULLUP);

  if (!LoRa.begin(LORA_BAND)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }

  // Configure for NLOS
  LoRa.setTxPower(20);
  LoRa.setSpreadingFactor(12);
  LoRa.setSignalBandwidth(41.7E3);
  LoRa.setCodingRate4(6);
  LoRa.setOCP(240);
  LoRa.enableCrc();
  
  Serial.println("LoRa Transmitter Ready");
  Serial.println("Single press = Help, Triple press = Emergency");
}

// Add even parity bit for error detection
uint8_t addEvenParity(uint8_t v) {
  uint8_t ones = 0;
  // Count 1-bits in lower 7 bits
  for (uint8_t i = 0; i < 7; i++) if (v & (1 << i)) ones ^= 1;
  if (ones) v |= 0x80;             // Set bit7 so total number of 1‑bits is even
  return v;
}

// Function to Encode device ID and emergency flag into message byte
uint8_t encodeMessage(uint8_t id, bool emergency) {
  uint8_t msg = id & 0x0F; // bits0‑3: device ID
  if (emergency) msg |= 0x10; // bit4: emergency flag
  return addEvenParity(msg);       // bit7: parity bit
}

// Function to check for triple press
bool checkTriplePress() {
  int reading = digitalRead(buttonPin);
  
  // Debouncing
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }
  
  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != buttonState) {
      buttonState = reading;
      
      // Button pressed
      if (buttonState == LOW) {
        unsigned long currentTime = millis();
        
        // Check if within triple-click window
        if (currentTime - lastPressTime <= tripleClickWindow) {
          pressCount++;
        } else {
          pressCount = 1; // Reset count
        }
        
        lastPressTime = currentTime;
        
        // Triple press detected
        if (pressCount >= 3) {
          pressCount = 0; // Reset
          lastButtonState = reading;
          return true;
        }
      }
    }
  }
  
  // Reset count if window expired
  if (millis() - lastPressTime > tripleClickWindow && pressCount > 0) {
    pressCount = 0;
  }
  
  lastButtonState = reading;
  return false;
}

// Function to check for single press
bool checkSinglePress() {
  static bool lastState = HIGH;
  static unsigned long lastDebounce = 0;
  
  int reading = digitalRead(buttonPin);
  
  if (reading != lastState) {
    lastDebounce = millis();
  }
  
  if ((millis() - lastDebounce) > debounceDelay) {
    if (reading == LOW && lastState == HIGH) {
      lastState = reading;
      return true;
    }
  }
  
  lastState = reading;
  return false;
}

void loop() {
  // Check for triple press first (higher priority)
  if (checkTriplePress()) {
    Serial.println("TRIPLE PRESS - Emergency!");

    uint8_t m = encodeMessage(DEVICE_ID, true);
    
    LoRa.beginPacket();
    LoRa.write(m);
    LoRa.endPacket();
    
    delay(500); // Longer delay for emergency transmission
  }
  
  // Check for single press
  else if (checkSinglePress() && pressCount == 0) {
    Serial.println("Single press - Help");
    
    uint8_t m = encodeMessage(DEVICE_ID, false);

    LoRa.beginPacket();
    LoRa.write(m);
    LoRa.endPacket();
    
    delay(500);
  }
  
  delay(10); // Small delay to prevent excessive polling
}