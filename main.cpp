#include "Adafruit_ST7735.h"
#include "mbed.h"
#include "DHT.h"

AnalogIn rain_sensor(A0);
DigitalOut buzzer(D2);
DHT dht11(D3, DHT11);

#define RAIN_CAP 600
Adafruit_ST7735 tft(D11, D12, D13, D10, D8, D9); // MOSI, MISO, SCLK, SSEL, TFT_DC, TFT_RST

char str[30];
BufferedSerial esp8266Serial(D1, D0); // TX, RX

int lastPrediction = -1;

int temperature = 0, humidity = 0;
bool waitingForResponse = false;
Mutex dataMutex;

Thread DHT11_Thread;
Thread SendESPThread;

bool startsWith(const char *str, const char *prefix) {
    while (*prefix) {
        if (*prefix++ != *str++)
            return false;
    }
    return true;
}

void sendDataToESP8266(uint16_t rain, int temp, int hum) {
    char buffer[64];
    int length = sprintf(buffer, "DATA:%d,%d,%d\n", rain, temp, hum);
    esp8266Serial.write(buffer, length);
    printf("Sent to ESP8266: %s", buffer);
    waitingForResponse = true;
}

void checkESP8266Response() {
    static char buffer[128];
    static int index = 0;

    while (esp8266Serial.readable()) {
        char c;
        esp8266Serial.read(&c, 1);

        if (c == '\n') {
            buffer[index] = '\0';

            if (startsWith(buffer, "SERVER:")) {
                const char *json = buffer + 7;
                printf("Raw JSON from ESP8266: %s\n", json);

                lastPrediction = -1;
                for (int i = 0; json[i] != '\0'; i++) {
                    if (startsWith(&json[i], "\"prediction\":")) {
                        char value = json[i + 13];
                        if (value >= '0' && value <= '9') {
                            lastPrediction = value - '0';
                        }
                        break;
                    }
                }

                printf("Extracted prediction value: %d\n", lastPrediction);
                waitingForResponse = false;
            }

            index = 0;
        } else if (index < sizeof(buffer) - 1) {
            buffer[index++] = c;
        }
    }
}

void Read_dht11() {
    int dht11_error = 0;
    ThisThread::sleep_for(2000ms);

    while (true) {
        dht11_error = dht11.readData();
        dataMutex.lock();
        if (dht11_error == 0) {
            temperature = (int)dht11.ReadTemperature(CELCIUS);
            humidity = (int)dht11.ReadHumidity();
            printf("Temperature= %d,  Humidity= %d %% \n", temperature, humidity);
        } else {
            printf("DHT11 Not Responsive...! \n");
            temperature = 0;
            humidity = 0;
        }
        dataMutex.unlock();
        ThisThread::sleep_for(2000ms);
    }
}

void SendToESP8266Thread() {
    while (true) {
        if (!waitingForResponse) {
            uint16_t rain;
            int temp, hum;

            dataMutex.lock();
            rain = rain_sensor.read() * 1000;
            temp = temperature;
            hum = humidity;
            dataMutex.unlock();

            sendDataToESP8266(rain, temp, hum);
        }

        ThisThread::sleep_for(10s);
    }
}

int main() {
    tft.initR(INITR_BLACKTAB);
    tft.fillScreen(ST7735_BLACK);
    buzzer = 1;

    DHT11_Thread.start(Read_dht11);
    SendESPThread.start(SendToESP8266Thread);

    while (true) {
        uint16_t rainValue = rain_sensor.read() * 1000;
        printf("Rain Intensity = %d\n", rainValue);
        if (rainValue > RAIN_CAP) {
            buzzer = 1;
        } else {
            buzzer = 0;
        }

        checkESP8266Response();

        tft.setTextColor(ST7735_WHITE);
        tft.setTextWrap(false);

        tft.fillRect(0, 20, 160, 20, ST7735_BLACK);
        tft.setCursor(5, 20);
        tft.setTextSize(1);

        if (lastPrediction == 1) {
            tft.printf("It looks cloudy");
        } else if (lastPrediction == 0) {
            tft.printf("It looks clear");
        } else {
            tft.printf("Cannot Detect");
        }

        tft.fillRect(0, 40, 160, 20, ST7735_BLACK);
        tft.setCursor(5, 40);
        tft.setTextSize(1);
        tft.printf("Rain Intensity: %d", rainValue);

        dataMutex.lock();
        int temp = temperature;
        int hum = humidity;
        dataMutex.unlock();

        if (temp != 0 && hum != 0) {
            tft.fillRect(0, 60, 160, 20, ST7735_BLACK);
            tft.setCursor(5, 60);
            tft.printf("Temperature = %d C", temp);

            tft.fillRect(0, 80, 160, 20, ST7735_BLACK);
            tft.setCursor(5, 80);
            tft.printf("Humidity = %d %%", hum);
        } else {
            tft.fillRect(0, 60, 160, 20, ST7735_BLACK);
            tft.setCursor(5, 60);
            tft.printf("Temperature = NOT AVAILABLE");

            tft.fillRect(0, 80, 160, 20, ST7735_BLACK);
            tft.setCursor(5, 80);
            tft.printf("Humidity = NOT AVAILABLE");
        }

        ThisThread::sleep_for(1000ms);
    }
}
