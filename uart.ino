#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ESP8266HTTPClient.h>

const char* ssid = "Home";
const char* password = "JayShreeRamji";

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

    if (input.startsWith("RAIN:")) {
      rainValue = input.substring(5).toInt();
      Serial.println("Received rain value: " + String(rainValue));

      if (WiFi.status() == WL_CONNECTED) {
        float precipitation = 50.0; // Example value
        float rainlevel = float(rainValue); // From Serial input
        float humidity = 10.0; // Example value

        String serverPath = "https://weather-project-3rpj.onrender.com/predict/?precipitation=" +
                            String(precipitation) +
                            "&rain=" +
                            String(rainlevel) +
                            "&humidity=" +
                            String(humidity);

        WiFiClientSecure client;
        client.setInsecure(); // ⚠️ Dev only

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

  delay(100); // Small delay to avoid spamming loop
}
