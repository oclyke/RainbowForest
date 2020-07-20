/* 
// This file is subject to the terms and conditions defined in
// file 'LICENSE.md', which is part of this source code package.
*/

// BOARD: Adafruit ESP32 Feather

#include "src/components/uart_bridge/uart_bridge.h"
#include "src/components/cart/cart.h"
#include "src/components/cat/cat.h"

#include "src/components/configuration/configuration.h"

#include "WiFi.h"
#include "WiFiUdp.h"
#include "AsyncTCP.h" // https://github.com/me-no-dev/AsyncTCP

#define DEBUG_PORT Serial
#define DEBUG_BAUD (115200)

#define BRIDGE_PORT Serial1
#define BRIDGE_BAUD (115200)
UARTBridge <cart_t> cartBridge(BRIDGE_PORT);
UARTBridge <cat_t> catBridge(BRIDGE_PORT);
cart_t cart;
cat_t cat;

IPAddress IP;
WiFiUDP udp;
AsyncServer server(NETWORK_TCP_PORT);

AsyncClient* column_clients[STAFF_COLS];

volatile bool button0 = false;
void IRAM_ATTR button0ISR() {
  button0 = true;
}

void onCatReception(cat_t* cat, void* args){
  if(cat->col >= STAFF_COLS){
    DEBUG_PORT.println("received an invalid cat");
    return;
  }
  
  AsyncClient* client = column_clients[cat->col];
  if(client){
    client->write((const char*)cat, (sizeof(cat_t)/sizeof(uint8_t)));
  }
}

void setup() {
  DEBUG_PORT.begin(DEBUG_BAUD);
  BRIDGE_PORT.begin(BRIDGE_BAUD);

  pinMode(0, INPUT_PULLUP);
  attachInterrupt(0, button0ISR, RISING);

  catBridge.onReceive(onCatReception, NULL);

  WiFi.softAP(NETWORK_SSID, NETWORK_PASSWORD);
  IP = WiFi.softAPIP();

  server.onClient(handleClientConnected, NULL);
  server.begin();
  
  udp.begin(IP, NETWORK_UDP_PORT);
  xTaskCreate(broadcastIP, "broadcastIP", 10000, NULL, 1, NULL);
}

void loop() {
  catBridge.check();
  if(button0){
    DEBUG_PORT.println("button0 released");
    button0 = false;
  }
}
