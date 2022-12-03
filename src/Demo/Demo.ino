//DOIT ESP32 DEVKIT V1

#include "flora_sht20.h"
#define uS_TO_S_FACTOR 1000000ULL  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  30*60       /* Time ESP32 will go to sleep (in seconds) */

//  PERIPHERALS
flora_sht20 sht20(&Wire, SHT20_I2C_ADDR);
#include <esp_now.h>
#include <esp_wifi.h>
#include <WiFi.h>
#include <Adafruit_Sensor.h>

// Set your Board ID (ESP32 Sender #1 = BOARD_ID 1, ESP32 Sender #2 = BOARD_ID 2, etc)
#define BOARD_ID 1

//MAC Address of the receiver
uint8_t broadcastAddress[] = {0x24, 0x4C, 0xAB, 0x16, 0x91, 0x50};

//Structure example to send data
//Must match the receiver structure
typedef struct struct_message {
  int id;
  float temp;
  float hum;
  float moist_1;
  float moist_2;
  int readingId;
} struct_message;

//Create a struct_message called myData
struct_message myData;

unsigned int readingId = 0;

// Insert your SSID
constexpr char WIFI_SSID[] = "VM5191158";

int32_t getWiFiChannel(const char *ssid) {
  if (int32_t n = WiFi.scanNetworks()) {
    for (uint8_t i = 0; i < n; i++) {
      if (!strcmp(ssid, WiFi.SSID(i).c_str())) {
        return WiFi.channel(i);
      }
    }
  }
  return 0;
}


// callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
//  Serial.print("\r\nLast Packet Send Status:\t");
//  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}
esp_now_peer_info_t peerInfo;

void setup()
{
  Serial.begin(115200); // The baudrate of Serial monitor is set in 9600

  sht20.initSHT20();
  delay(100);
  Serial.println("Sensor init finish!");
  sht20.checkSHT20();

  // Set device as a Wi - Fi Station and set channel
  WiFi.mode(WIFI_STA);

  int32_t channel = getWiFiChannel(WIFI_SSID);

  WiFi.printDiag(Serial); // Uncomment to verify channel number before
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous(false);
  WiFi.printDiag(Serial); // Uncomment to verify channel change after

  //Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Once ESPNow is successfully Init, we will register for Send CB to
  // get the status of Trasnmitted packet
  esp_now_register_send_cb(OnDataSent);

  //Register peer

  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.encrypt = false;

  //Add peer
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
}

void loop()
{

  //  DEMO: SHT20
  float humd = sht20.readHumidity(); //Read the measured data of air humidity
  float temp = sht20.readTemperature(); //Read the measured temp data


  // Save the last time a new reading was published
  //Set values to send 
  myData.id = BOARD_ID;
  myData.temp = temp;
  myData.hum = humd;
  myData.moist_1 = 100-map(touchRead(12), 0, 35, 0, 100);
  myData.moist_2 = 100-map(touchRead(13), 0, 35, 0, 100);
  myData.readingId = readingId++;
 
  //Send message via ESP-NOW
  for (int i = 0; i < 5; i++)
  {
    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));
    if (result == ESP_OK && i==5) {
      Serial.println("Sent with success");
    }
    else if(result != ESP_OK && i==5) 
    {
      Serial.println("Error sending the data");
    }
    delay(5000);
  }
  
  esp_deep_sleep_start();
}
