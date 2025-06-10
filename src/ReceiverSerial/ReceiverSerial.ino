/*
 * LoRa Receiver - Serial Output Version
 * 
 * This script is for connecting with a serial Python reader if the DFPlayer 
 * doesn't work properly with the radios using a Raspberry Pi gateway
 * for audio processing. More details explained in the repository README.
 */
#include <SPI.h>
#include <LoRa.h>

// LoRa settings, Set frequency to 433 MHz, check local law regulations for allowed frequencies
#define LORA_BAND 433E6

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

// Output device ID and message type to serial for external processing
void outputMessage(uint8_t id, bool emergency) {
  // Format: DEVICE_ID:MESSAGE_TYPE (e.g., "3:EMERGENCY" or "1:HELP")
  Serial.print(id);
  Serial.print(":");
  Serial.println(emergency ? "EMERGENCY" : "HELP");
}


void setup() {
  Serial.begin(9600);
  while (!Serial);

  Serial.println("LoRa Receiver - Serial Output Mode");

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
  Serial.println("Ready to receive messages...");
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
        // Output formatted message for external processing
        outputMessage(deviceId, emergency);
      } else {
        Serial.println("Received message with parity error - ignored");
      }
    }
  }
}