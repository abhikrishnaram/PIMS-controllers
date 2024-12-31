#include <ESP8266WiFi.h>
#include <espnow.h>

#define RELAY_PIN 12  // Define the GPIO pin where the relay is connected
bool command = 0;

// Callback function that will be executed when data is received
void OnDataRecv(uint8_t * mac, uint8_t *incomingData, uint8_t len) {
  if (len == sizeof(command)) {
    // Copy the boolean data directly into the command variable
    memcpy(&command, incomingData, sizeof(command));
    Serial.print("Boolean received: ");
    Serial.println(command ? "ON" : "OFF");
  } else {
    Serial.println("Unexpected data length received.");
  }
}

void setup() {
  // Initialize Serial Monitor
  Serial.begin(115200);
  
  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

  // Init ESP-NOW
  if (esp_now_init() != 0) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW); // Initialize relay to OFF
  
  // Register for recv callback to get received packet info
  esp_now_set_self_role(ESP_NOW_ROLE_SLAVE);
  esp_now_register_recv_cb(OnDataRecv);
}

void loop() {
  // Control the relay based on the command received
  if (command) {
    digitalWrite(RELAY_PIN, HIGH);  // Turn on relay
    Serial.println("Relay ON");
  } else {
    digitalWrite(RELAY_PIN, LOW);   // Turn off relay
    Serial.println("Relay OFF");
  }
}
