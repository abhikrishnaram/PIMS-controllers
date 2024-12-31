#include <ESP8266WiFi.h>
#include <espnow.h>

String incomingData = "";  // To hold the MAC address + command
uint8_t broadcastAddress[6];  // Array to store parsed MAC address
bool myBooleanValue = false;  // Boolean variable to send

// Callback when data is sent
void OnDataSent(uint8_t *mac_addr, uint8_t sendStatus) {
  Serial.print("Last Packet Send Status: ");
  if (sendStatus == 0){
    Serial.println("Delivery success");
  }
  else{
    Serial.println("Delivery fail");
  }
}
 
void setup() {
  // Init Serial Monitor
  Serial.begin(115200);
 
  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

  // Init ESP-NOW
  if (esp_now_init() != 0) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Register callback for send status
  esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
  esp_now_register_send_cb(OnDataSent);
}
 
void loop() {
  if (Serial.available() > 0) {
    // Read incoming data from Serial
    incomingData = Serial.readStringUntil('\n');
    incomingData.trim();
    
    // Check for valid format - expecting "MAC_ADDRESS,command"
    int commaIndex = incomingData.indexOf(',');
    if (commaIndex == -1) {
      Serial.println("Invalid format. Expected MAC_ADDRESS,command.");
      return;
    }

    // Extract the MAC address and command from incomingData
    String macStr = incomingData.substring(0, commaIndex);
    String command = incomingData.substring(commaIndex + 1);

    // Convert MAC address string to uint8_t array
    if (convertMacStringToUint8Array(macStr, broadcastAddress)) {
      // Register peer with the parsed MAC address
      if (esp_now_add_peer(broadcastAddress, ESP_NOW_ROLE_SLAVE, 1, NULL, 0) != 0) {
        Serial.println("Failed to add peer");
        return;
      } else {
        Serial.println("Peer added successfully");
      }

      // Set the boolean variable based on the command
      myBooleanValue = (command == "1");

      // Send the boolean variable
      esp_now_send(broadcastAddress, (uint8_t *) &myBooleanValue, sizeof(myBooleanValue));

      // Print the command received and sent status
      Serial.print("Command executed: ");
      Serial.println(command);

      // Remove peer after sending (optional to avoid duplication in future sends)
      esp_now_del_peer(broadcastAddress);
    } else {
      Serial.println("Invalid MAC address format.");
    }
  }
}

// Helper function to convert MAC string to uint8_t array
bool convertMacStringToUint8Array(String macStr, uint8_t* macArray) {
  int macOctets[6];
  if (sscanf(macStr.c_str(), "%x:%x:%x:%x:%x:%x", 
             &macOctets[0], &macOctets[1], &macOctets[2], 
             &macOctets[3], &macOctets[4], &macOctets[5]) == 6) {
    for (int i = 0; i < 6; i++) {
      macArray[i] = (uint8_t) macOctets[i];
    }
    return true; // Success
  }
  return false; // Failed to parse MAC address
}
