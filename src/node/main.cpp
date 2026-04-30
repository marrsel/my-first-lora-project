#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <RadioLib.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define SCREEN_ADDRESS 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire);

#define MOSI_PIN 13
#define MISO_PIN 12
#define SCK_PIN 14
#define NSS_PIN 15
#define DIO1_PIN 2
#define NRST_PIN 4

SPIClass hspi(HSPI);

SX1262 radio = new Module(NSS_PIN, DIO1_PIN, NRST_PIN, RADIOLIB_NC, hspi);

volatile bool receivedFlag = false;

void display_data(float rssi, float snr, int packet, int overall_packets, int packets_lost);

ICACHE_RAM_ATTR void setFlag(void) {
  receivedFlag = true;
}

int overall_received = 0;
int last_received = 0;
int lost_packets = 0;
bool first_packet_received = false;

void setup() {
  Serial.begin(9600);
  hspi.begin(SCK_PIN, MISO_PIN, MOSI_PIN, NSS_PIN);

  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    while(1) {
      Serial.println(F("SSD1306 allocation failed"));
      delay(2000);
    }
  }
  Serial.println("Display started successfully");

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.cp437(true);
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.print("Starting..");
  display.display();


  int state = radio.begin(868.0, 125.0, 12, 5, 0xA4);
  if (state != RADIOLIB_ERR_NONE) {
    while(true) {
      Serial.print("Failed to start radio, code: ");
      Serial.println(state);
      delay(2000);
    }
  }
  Serial.println("Radio initialized successfully");

  radio.setPacketReceivedAction(setFlag);

  state = radio.startReceive();
  if (state != RADIOLIB_ERR_NONE) {
    while(true) {
      Serial.print("Failed to start radio receiving, code: ");
      Serial.println(state);
      delay(2000);
    }
  }
  Serial.println("Radio receiving");

  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("Receiving");
  display.display();
}

void loop() {
  if (receivedFlag) {
    receivedFlag = false;

    String str;
    int state = radio.readData(str);
    if (state != RADIOLIB_ERR_NONE) {
      while (true) {
        Serial.print("Error, code: ");
        Serial.println(state);
        delay(2000);
      }
    }

    Serial.print("Received data: ");
    Serial.println(str);
    Serial.print("Frequency error: ");
    Serial.print(radio.getFrequencyError());
    Serial.println(" Hz");

    int received_now = str.toInt();

    int diff = received_now - last_received - 1;
    if (diff > 0 && first_packet_received) {
      lost_packets += diff;
    }
    last_received = received_now;
    overall_received++;

    if (!first_packet_received) {
      first_packet_received = true;
    }

    display_data(radio.getRSSI(), radio.getSNR(), received_now, overall_received, lost_packets);
  }
}

void display_data(float rssi, float snr, int packet, int overall_packets, int packets_lost) {
  int16_t x, y;
  uint16_t w, h;
  
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(0, 0);

  display.print((int)rssi);
  display.print(" ");

  display.setTextSize(1);
  if (snr < 0) {
    display.print("BAD");
  } else if (snr > 0 && snr < 5) {
    display.print("OK");
  } else if (snr >= 5) {
    display.print("GOOD");
  }
  display.setTextSize(2);

  String snr_str = String(snr);
  display.getTextBounds(snr_str, 0, 0, &x, &y, &w, &h);

  display.setCursor(SCREEN_WIDTH - w, 0);
  display.println(snr_str);

  display.print(overall_packets);
  display.print(" ");
  display.setTextSize(1);
  display.print(packet);
  display.setTextSize(2);
  String packets_lost_str = String(packets_lost);
  display.getTextBounds(packets_lost_str, 0, 0, &x, &y, &w, &h);
  display.setCursor(SCREEN_WIDTH - w, SCREEN_HEIGHT - h);
  display.print(packets_lost_str);

  display.display();
}