/*
DOIT ESP32 DEVKIT V1
*/

#include <esp_now.h>
#include <WiFi.h>
#include "ESPAsyncWebServer.h"
#include <Arduino_JSON.h>
#include "ThingSpeak.h"
WiFiClient  client;

// Sleep for 10 minutes after data point
int time_in_us = 25*60*1000000;
unsigned long myChannelNumber = 1756801;
const char * myWriteAPIKey = "46LUWZJQL7LBCMVN";
// Replace with your network credentials (STATION)
const char* ssid = "VM5191158_EXT";
const char* password = "kgLpwnrX3jrr";

// Structure example to receive data
// Must match the sender structure
typedef struct struct_message {
  int id;
  float temp;
  float hum;
  float moist_1;
  float moist_2;
  unsigned int readingId;
} struct_message;
bool newData = false;
struct_message incomingReadings;

JSONVar board;

//AsyncWebServer server(80);
AsyncEventSource events("/events");

// callback function that will be executed when data is received
void OnDataRecv(const uint8_t * mac_addr, const uint8_t *incomingData, int len) {
  // Copies the sender mac address to a string
  char macStr[18];
  Serial.print("Packet received from: ");
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.println(macStr);
  memcpy(&incomingReadings, incomingData, sizeof(incomingReadings));

  board["id"] = incomingReadings.id;
  board["temperature"] = incomingReadings.temp;
  board["humidity"] = incomingReadings.hum;
  board["moist 1"] = incomingReadings.moist_1;
  board["moist 2"] = incomingReadings.moist_2;
  board["readingId"] = String(incomingReadings.readingId);
  String jsonString = JSON.stringify(board);
  events.send(jsonString.c_str(), "new_readings", millis());

  Serial.printf("Board ID %u: %u bytes\n", incomingReadings.id, len);
  Serial.printf("t value: %4.2f \n", incomingReadings.temp);
  Serial.printf("h value: %4.2f \n", incomingReadings.hum);
  Serial.printf("m1 value: %4.2f \n", incomingReadings.moist_1);
  Serial.printf("m2 value: %4.2f \n", incomingReadings.moist_2);
  Serial.printf("readingID value: %d \n", incomingReadings.readingId);
  Serial.println();

  newData = true;
}

//const char index_html[] PROGMEM = R"rawliteral(
//<!DOCTYPE HTML><html>
//<head>
//  <title>ESP-NOW DASHBOARD</title>
//  <meta name="viewport" content="width=device-width, initial-scale=1">
//  <link rel="stylesheet" href="https://use.fontawesome.com/releases/v5.7.2/css/all.css" integrity="sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr" crossorigin="anonymous">
//  <link rel="icon" href="data:,">
//  <style>
//    html {font-family: Arial; display: inline-block; text-align: center;}
//    p {  font-size: 1.2rem;}
//    body {  margin: 0;}
//    .topnav { overflow: hidden; background-color: #2f4468; color: white; font-size: 1.7rem; }
//    .content { padding: 20px; }
//    .card { background-color: white; box-shadow: 2px 2px 12px 1px rgba(140,140,140,.5); }
//    .cards { max-width: 700px; margin: 0 auto; display: grid; grid-gap: 2rem; grid-template-columns: repeat(auto-fit, minmax(300px, 1fr)); }
//    .reading { font-size: 2.8rem; }
//    .packet { color: #bebebe; }
//    .card.temperature { color: #fd7e14; }
//    .card.humidity { color: #1b78e2; }
//  </style>
//</head>
//<body>
//  <div class="topnav">
//    <h3>ESP-NOW DASHBOARD</h3>
//  </div>
//  <div class="content">
//    <div class="cards">
//      <div class="card temperature">
//        <h4><i class="fas fa-thermometer-half"></i> BOARD #1 - TEMPERATURE</h4><p><span class="reading"><span id="t1"></span> &deg;C</span></p><p class="packet">Reading ID: <span id="rt1"></span></p>
//      </div>
//      <div class="card humidity">
//        <h4><i class="fas fa-tint"></i> BOARD #1 - HUMIDITY</h4><p><span class="reading"><span id="h1"></span> &percnt;</span></p><p class="packet">Reading ID: <span id="rh1"></span></p>
//      </div>
//      <div class="card temperature">
//        <h4><i class="fas fa-thermometer-half"></i> BOARD #2 - TEMPERATURE</h4><p><span class="reading"><span id="t2"></span> &deg;C</span></p><p class="packet">Reading ID: <span id="rt2"></span></p>
//      </div>
//      <div class="card humidity">
//        <h4><i class="fas fa-tint"></i> BOARD #2 - HUMIDITY</h4><p><span class="reading"><span id="h2"></span> &percnt;</span></p><p class="packet">Reading ID: <span id="rh2"></span></p>
//      </div>
//    </div>
//  </div>
//<script>
//if (!!window.EventSource) {
// var source = new EventSource('/events');
//
// source.addEventListener('open', function(e) {
//  console.log("Events Connected");
// }, false);
// source.addEventListener('error', function(e) {
//  if (e.target.readyState != EventSource.OPEN) {
//    console.log("Events Disconnected");
//  }
// }, false);
//
// source.addEventListener('message', function(e) {
//  console.log("message", e.data);
// }, false);
//
// source.addEventListener('new_readings', function(e) {
//  console.log("new_readings", e.data);
//  var obj = JSON.parse(e.data);
//  document.getElementById("t"+obj.id).innerHTML = obj.temperature.toFixed(2);
//  document.getElementById("h"+obj.id).innerHTML = obj.humidity.toFixed(2);
//  document.getElementById("rt"+obj.id).innerHTML = obj.readingId;
//  document.getElementById("rh"+obj.id).innerHTML = obj.readingId;
// }, false);
//}
//</script>
//</body>
//</html>)rawliteral";

void setup() {
  // Initialize Serial Monitor
  Serial.begin(115200);
  esp_sleep_enable_timer_wakeup(time_in_us);
  delay(500);
  // Set the device as a Station and Soft Access Point simultaneously
  WiFi.mode(WIFI_AP_STA);
  delay(500);
  // Set device as a Wi-Fi Station
  WiFi.begin(ssid, password);
  delay(500);
  int tryCounter = 0;
  while (WiFi.status() != WL_CONNECTED) {
    if (tryCounter==10)
    {
      Serial.println("Restarting...");
      ESP.restart();
    }
    delay(1000);
    Serial.println("Setting as a Wi-Fi Station..");
    tryCounter++;
  }
  Serial.print("MAC Address:  ");
  Serial.println(WiFi.macAddress());
  Serial.print("Station IP Address: ");
  Serial.println(WiFi.localIP());
  Serial.print("Wi-Fi Channel: ");
  Serial.println(WiFi.channel());

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  ThingSpeak.begin(client);  // Initialize ThingSpeak
  // Once ESPNow is successfully Init, we will register for recv CB to
  // get recv packer info
  esp_now_register_recv_cb(OnDataRecv);

//  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
//    request->send_P(200, "text/html", index_html);
//  });

  events.onConnect([](AsyncEventSourceClient * client) {
    if (client->lastId()) {
      Serial.printf("Client reconnected! Last message ID that it got is: %u\n", client->lastId());
    }
    // send event with message "hello!", id current millis
    // and set reconnect delay to 1 second
    client->send("hello!", NULL, millis(), 10000);
  });
//  server.addHandler(&events);
//  server.begin();
  Serial.println("Listening...");
}

void loop() {
//  static unsigned long lastEventTime = millis();
//  static const unsigned long EVENT_INTERVAL_MS = 5000;
//  if ((millis() - lastEventTime) > EVENT_INTERVAL_MS) {
    events.send("ping", NULL, millis());
//    lastEventTime = millis();
    if (newData)
    {
      ThingSpeak.setField(1, incomingReadings.temp);
      ThingSpeak.setField(2, incomingReadings.hum);
      ThingSpeak.setField(3, incomingReadings.moist_1);
      ThingSpeak.setField(4, incomingReadings.moist_2);
      int x = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
      newData = false;
      Serial.println("Going to sleep now");
      delay(1000);
      Serial.flush(); 
      esp_deep_sleep_start();
    }
    delay(500);
//  }

}
