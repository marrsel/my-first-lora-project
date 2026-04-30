#include <SPI.h>
#include <RadioLib.h>

#define MOSI_PIN 13
#define MISO_PIN 12
#define SCK_PIN 14
#define NSS_PIN 15
#define DIO1_PIN 2
#define NRST_PIN 4

SPIClass hspi(HSPI);

SX1262 radio = new Module(NSS_PIN, DIO1_PIN, NRST_PIN, RADIOLIB_NC, hspi);

int transmissionState = RADIOLIB_ERR_NONE;

int count = 1;

volatile bool transmittedFlag = false;

ICACHE_RAM_ATTR void setFlag(void) {
  transmittedFlag = true;
}

void setup() {
  Serial.begin(9600);
  hspi.begin(SCK_PIN, MISO_PIN, MOSI_PIN, NSS_PIN);

  int state = radio.begin(868.0, 125.0, 12, 5, 0xA4);
  if (state != RADIOLIB_ERR_NONE) {
    while(true) {
      Serial.print("Failed to start radio, code: ");
      Serial.println(state);
      delay(2000);
    }
  }
  Serial.println("Radio initialized successfully");

  radio.setPacketSentAction(setFlag);

  Serial.print("Sending packet: ");
  Serial.println(count);

  String str = String(count);
  transmissionState = radio.startTransmit(str);
}

void loop() {
  if (transmittedFlag) {
    transmittedFlag = false;

    if (transmissionState != RADIOLIB_ERR_NONE) {
        Serial.print("Transmission failed, code: ");
        Serial.println(transmissionState);
    }

    radio.finishTransmit();

    delay(1000);

    count++;

    Serial.print("Sending packet: ");
    Serial.println(count);

    String str = String(count);
    transmissionState = radio.startTransmit(str);
  }
}