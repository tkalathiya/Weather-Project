#include "Adafruit_ST7735.h"
#include "BME280.h"
#include "mbed.h"


AnalogIn rain_sensor(A0);
DigitalOut buzzer(D2);

#define RAIN_CAP 600
Adafruit_ST7735 tft(D11, D12, D13, D10, D8,
                    D9); // MOSI, MISO, SCLK, SSEL, TFT_DC, TFT_RST

char str[30];
BufferedSerial esp8266Serial(D1,
                             D0); // TX, RX pins for communication with ESP8266

int lastPrediction = -1; // Stores latest prediction from ESP8266

void sendRainDataToESP8266(uint16_t rainValue) {
  char buffer[32];
  int length =
      sprintf(buffer, "RAIN:%d\n", rainValue); // Format the data with a newline
                                               // for easier parsing on ESP8266
  esp8266Serial.write(buffer, length);
}

bool startsWith(const char *str, const char *prefix) {
  while (*prefix) {
    if (*prefix++ != *str++)
      return false;
  }
  return true;
}

void checkESP8266Response() {
  static char buffer[128];
  static int index = 0;

  while (esp8266Serial.readable()) {
    char c;
    esp8266Serial.read(&c, 1);

    if (c == '\n') {
      buffer[index] = '\0';

      // Check if response starts with "SERVER:"
      if (startsWith(buffer, "SERVER:")) {
        const char *json = buffer + 7; // Skip "SERVER:" prefix
        printf("Raw JSON from ESP8266: %s\n", json);

        // Manual parse to extract prediction value
        lastPrediction = -1; // Reset in case not found
        for (int i = 0; json[i] != '\0'; i++) {
          if (startsWith(&json[i], "\"prediction\":")) {
            char value = json[i + 13]; // Or hardcoded 13
            if (value >= '0' && value <= '9') {
              lastPrediction = value - '0';
            }
            break;
          }
        }

        printf("Extracted prediction value: %d\n", lastPrediction);

        // Optional: Display result on TFT
        tft.setTextSize(1);
        tft.setTextColor(ST7735_WHITE, ST7735_RED);
        tft.setCursor(1, 50);
        tft.printf("                          "); // Clear area
        tft.setCursor(1, 50);
      }

      index = 0; // Reset buffer index
    } else if (index < sizeof(buffer) - 1) {
      buffer[index++] = c;
    }
  }
}

int main() {
  tft.initR(INITR_BLACKTAB); // initialize a ST7735S chip, black tab
  tft.fillScreen(ST7735_RED);

  buzzer = 1; // disable buzzer

  while (true) {
    uint16_t rainValue = rain_sensor.read() * 1000;
    sendRainDataToESP8266(rainValue);
    printf("Rain Intensity = %04d ", rainValue);

    if (rainValue > RAIN_CAP) {
      printf(" - Dry Weather");
      buzzer = 1;
    } else {
      printf(" - About to Rain ");
      buzzer = 0;
    }

    checkESP8266Response();
    printf("\r\n\n");
    // Set overall text properties
    tft.setTextColor(ST7735_WHITE);
    tft.setTextWrap(false);

    // Display Prediction (larger and clearer)
    tft.fillRect(0, 10, 160, 20, ST7735_RED); // Clear previous text area
    tft.setCursor(5, 10);
    tft.setTextSize(1); // Make prediction larger

    if (lastPrediction == 1) {
      tft.printf("It looks cloudy");
    } else if (lastPrediction == 0) {
      tft.printf("It looks clear");
    } else {
      tft.printf("Unknown");
    }

    // Display Rain Intensity (smaller, underneath)
    tft.fillRect(0, 40, 160, 20, ST7735_RED); // Clear previous text area
    tft.setCursor(5, 40);
    tft.setTextSize(1); // Keep this small
    sprintf(str, "Rain Intensity: %d", rainValue);
    tft.printf(str);

    ThisThread::sleep_for(500ms);
  }
}
