#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ESP8266HTTPClient.h>

const char* ssid = "WIFI_USERNAMAE";
const char* password = "WIFI_PASSWORD";

int rainValue = 0;  // Global so it can be used after Serial input

void setup() {
  Serial.begin(9600);
  WiFi.begin(ssid, password);

  Serial.println("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
}

void loop() {
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim(); // Clean up whitespace

    if (input.startsWith("DATA:")) {
      input = input.substring(5); // Remove "DATA:"

  int comma1 = input.indexOf(',');
  int comma2 = input.indexOf(',', comma1 + 1);

  if (comma1 > 0 && comma2 > comma1) {
    float rainValue = input.substring(0, comma1).toFloat();
    float temperature = input.substring(comma1 + 1, comma2).toFloat();
    float humidity = input.substring(comma2 + 1).toFloat();

    Serial.println("Received → Rain: " + String(rainValue) + ", Temp: " + String(temperature) + ", Hum: " + String(humidity));

      if (WiFi.status() == WL_CONNECTED) {
        float temp = float(temperature); //  From Serial input
        float rainlevel = float(rainValue); // From Serial input
        float hum = float(humidity); // From Serial input

        String serverPath = "https://weather-project-3rpj.onrender.com/predict/?precipitation=" +
                            String(temp) +
                            "&rain=" +
                            String(rainlevel) +
                            "&humidity=" +
                            String(hum);

        WiFiClientSecure client;
        client.setInsecure();

        HTTPClient https;
        Serial.println("Requesting URL: " + serverPath);

        if (https.begin(client, serverPath)) {
          int httpCode = https.GET();
          Serial.print("HTTP Code: ");
          Serial.println(httpCode);

          if (httpCode > 0) {
            String payload = https.getString();
            Serial.println("Response:");
            Serial.println(payload);
            Serial.println("SERVER:" + payload); // Send to FRDM-K66F in identifiable format
          } else {
            Serial.println("GET request failed");
          }

          https.end();
        } else {
          Serial.println("Unable to connect");
        }
      } else {
        Serial.println("WiFi not connected.");
      }
    }
  }
}
  delay(100); // Small delay to avoid spamming loop
}
