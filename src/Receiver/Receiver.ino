#include <SPI.h>
#include <LoRa.h>
#include <DFRobotDFPlayerMini.h>
#include <SoftwareSerial.h>

// LoRa settings, Set frequency to 433 MHz, check local law regulations for allowed frequencies
#define LORA_BAND 433E6

SoftwareSerial mySerial(3, 4);  // RX, TX for DFPlayer
DFRobotDFPlayerMini myDFPlayer;

// Decode message byte and verify parity
bool decodeMessage(uint8_t b, uint8_t &id, bool &emergency) {
  uint8_t ones = 0;
  // Check even parity across all 8 bits
  for (uint8_t i = 0; i < 8; i++) if (b & (1 << i)) ones ^= 1;
  if (ones) return false; // Parity error
  id = b & 0x0F; // bits0‑3: device ID
  emergency = b & 0x10; // bit4: emergency flag
  return true;
}

// Map device ID and message type to audio file number
void playAudio(uint8_t id, bool emergency) {
  uint8_t base = (id - 1) * 2 + 1;  // Each device gets 2 consecutive file numbers starting from 1
  uint8_t file = emergency ? base : base + 1;  // Emergency uses base, help uses base+1
  myDFPlayer.play(file);
}


void setup() {
  Serial.begin(9600);
  while (!Serial);

  Serial.println("LoRa Receiver - NLOS Configuration");

  // Initialize LoRa at 433 MHz
  if (!LoRa.begin(LORA_BAND)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }

  // Configure LoRa for NLOS
  LoRa.setSpreadingFactor(12);
  LoRa.setSignalBandwidth(41.7E3);
  LoRa.setCodingRate4(6);
  LoRa.enableCrc();
  Serial.println("LoRa setup complete for NLOS communication.");

  // Initialize DFPlayer Mini
  mySerial.begin(9600);
  if (!myDFPlayer.begin(mySerial)) {
    Serial.println("Unable to begin DFPlayer Mini");
    while (1);
  }
  myDFPlayer.volume(10);  // Set volume level (0-30)
}

void loop() {
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    // Read the single byte message
    if (LoRa.available()) {
      uint8_t receivedByte = LoRa.read();
      uint8_t deviceId;
      bool emergency;
      
      // Decode message and check parity
      if (decodeMessage(receivedByte, deviceId, emergency)) {
        Serial.print("Valid message from device ");
        Serial.print(deviceId);
        Serial.println(emergency ? " - EMERGENCY" : " - Help");
        
        // Play corresponding audio file
        playAudio(deviceId, emergency);
      } else {
        Serial.println("Received message with parity error - ignored");
      }
    }
  }
}