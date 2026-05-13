#include <Arduino.h>
#include <SPI.h>
#include <Ethernet.h>
#include "LoRa_E220.h"

#define ETH_SCK  36
#define ETH_MISO 37
#define ETH_MOSI 35
#define ETH_CS   10
#define LORA_AUX 15
#define LORA_M0  4
#define LORA_M1  5
#define LORA_RX  16
#define LORA_TX  17

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
char server[] = "httpbin.org";
String path = "/post";

struct __attribute__((packed)) DataDevicePJU {
    char id[10];
    bool state;
    uint16_t vin;
    uint16_t crc;
};

LoRa_E220 e220(&Serial2, LORA_AUX, LORA_M0, LORA_M1);
EthernetClient ethClient;

uint16_t calculateCRC(uint16_t vin, bool state) {
    return vin ^ (state ? 0x4398 : 0x1234);
}

void postToServer(String json) {
    if (ethClient.connect(server, 80)) {
        ethClient.println("POST " + path + " HTTP/1.1");
        ethClient.println("Host: " + String(server));
        ethClient.println("Content-Type: application/json");
        ethClient.print("Content-Length: ");
        ethClient.println(json.length());
        ethClient.println();
        ethClient.print(json);

        uint32_t timeout = millis();
        while (ethClient.connected() && millis() - timeout < 2000) {
            if (ethClient.available()) {
                char c = ethClient.read();
                Serial.print(c);
                timeout = millis();
            }
        }
        ethClient.stop();
        Serial.println("\n[Gateway] Data Uploaded");
    } else {
        Serial.println("[Gateway] Connection Failed");
    }
}

void sendData() {
    DataDevicePJU data;
    strncpy(data.id, "PJU001", 10);
    data.state = true;
    data.vin = 220;
    data.crc = calculateCRC(data.vin, data.state);

    Serial.println("[Gateway] Sending data...");
    Serial.print("ID: ");    Serial.println(data.id);
    Serial.print("State: "); Serial.println(data.state ? "ON" : "OFF");
    Serial.print("Vin: ");   Serial.println(data.vin);
    Serial.print("CRC: ");   Serial.println(data.crc);

    String jsonBody = "{";
    jsonBody += "\"id\":\"" + String(data.id) + "\",";
    jsonBody += "\"state\":" + String(data.state ? "true" : "false") + ",";
    jsonBody += "\"vin\":" + String(data.vin) + ",";
    jsonBody += "\"crc\":" + String(data.crc);
    jsonBody += "}";

    postToServer(jsonBody);
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    pinMode(36, OUTPUT);
    pinMode(37, OUTPUT);

    SPI.begin(ETH_SCK, ETH_MISO, ETH_MOSI, ETH_CS);
    Ethernet.init(ETH_CS);

    Serial.println("Mencoba DHCP...");
    int dhcpRetry = 0;
    while (Ethernet.begin(mac) == 0 && dhcpRetry < 3) {
        Serial.println("Retry DHCP...");
        delay(2000);
        dhcpRetry++;
    }
    if (Ethernet.localIP() == IPAddress(0, 0, 0, 0)) {
        Serial.println("[Gateway] DHCP Failed, pakai static IP");
        Ethernet.begin(mac, IPAddress(192, 168, 1, 100));
    }
    Serial.print("[Gateway] IP: ");
    Serial.println(Ethernet.localIP());

    Serial2.begin(9600, SERIAL_8N1, LORA_RX, LORA_TX);
    e220.begin();

    ResponseStructContainer c = e220.getConfiguration();
    Configuration config = *(Configuration*) c.data;
    Serial.print("[LoRa] Channel: "); Serial.println(config.CHAN);
    Serial.print("[LoRa] ADDH: ");    Serial.println(config.ADDH);
    Serial.print("[LoRa] ADDL: ");    Serial.println(config.ADDL);
    c.close();

    Serial.println("[Gateway] Ready");
}

void loop() {
    if (e220.available() > 0 || Serial2.available() > 0) {
        Serial.println("\n[LoRa] Data diterima!");
        ResponseStructContainer rsc = e220.receiveMessage(sizeof(DataDevicePJU));
        struct DataDevicePJU* rxData = (struct DataDevicePJU*) rsc.data;

        if (rsc.status.code == 1) {
            uint16_t checkCRC = calculateCRC(rxData->vin, rxData->state);
            if (checkCRC == rxData->crc) {
                Serial.println("[Gateway] Valid Packet Received");
                String jsonBody = "{";
                jsonBody += "\"id\":\"" + String(rxData->id) + "\",";
                jsonBody += "\"state\":" + String(rxData->state ? "true" : "false") + ",";
                jsonBody += "\"vin\":" + String(rxData->vin) + ",";
                jsonBody += "\"crc\":" + String(rxData->crc);
                jsonBody += "}";
                postToServer(jsonBody);
            } else {
                Serial.print("[Gateway] CRC Error - Received: ");
                Serial.print(rxData->crc);
                Serial.print(" Expected: ");
                Serial.println(checkCRC);
            }
        }
        rsc.close();
    }

    static uint32_t lastSend = 0;
    if (millis() - lastSend >= 10000) {
        lastSend = millis();
        sendData();
    }
}