

#include <ESP8266WiFi.h>
#include <espnow.h>

// REPLACE WITH RECEIVER MAC Address
uint8_t broadcastAddress[] = {0x9C, 0x9C, 0x1F, 0xAA, 0xF9, 0x00};
String incomingData = "";

// Structure example to send data
// Must match the receiver structure
typedef struct struct_message {
  char a[32];
  int b;
  float c;
  String d;
  bool e;
} struct_message;

// Create a struct_message called myData
struct_message myData;

unsigned long lastTime = 0;  
unsigned long timerDelay = 2000;  // send readings timer

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

  // Once ESPNow is successfully Init, we will register for Send CB to
  // get the status of Trasnmitted packet
  esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
  esp_now_register_send_cb(OnDataSent);
  
  // Register peer
  esp_now_add_peer(broadcastAddress, ESP_NOW_ROLE_SLAVE, 1, NULL, 0);
}
 
void loop() {
  if (Serial.available()>0) {
    incomingData = Serial.readStringUntil('\n');
    incomingData.trim();
    Serial.print("Received from Serial: ");
    Serial.println(incomingData);
    strcpy(myData.a, "THIS IS A CHAR");
    myData.b = random(1,20);incomingData.trim();
    myData.c = 1.2;
    myData.d = "Hello";
    if (incomingData == "1"){
      myData.e = true;
       esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));


    }
    else if (incomingData == "0"){
      myData.e = false;
       esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));

    }
    else {
       Serial.println("Invalid command");
    }
    

    // Send message via ESP-NOW
   
    lastTime = millis();
  }
}