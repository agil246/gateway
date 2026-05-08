#include <Arduino.h>
#include <SPI.h>
#include <Ethernet.h>
#include "e220.h"

#define ETH_SCK 12
#define ETH_MISO 13
#define ETH_MOSI 11
#define ETH_CS 10

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
EthernetClient client;

const char* serverHost = "log.pcbjogja.com";
const char* serverPath = "/test.php";

IPAddress ip(192, 168, 1, 177);
IPAddress dns(8, 8, 8, 8);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

void setupGateway() {
    Serial.begin(115200);

    SPI.begin(ETH_SCK, ETH_MISO, ETH_MOSI, ETH_CS);
    Ethernet.init(ETH_CS);

    if (Ethernet.hardwareStatus() == EthernetNoHardware) {
        while (true) {
            delay(1);
        }
    }

    if (Ethernet.begin(mac) == 0) {
        Ethernet.begin(mac, ip, dns, gateway, subnet);
    }

    delay(2000);
    setupE220();
}

void loopGateway() {
    char buffer[200];

    if (receiveData(buffer, sizeof(buffer))) {
        Serial.print("Data: ");
        Serial.println(buffer);

        if (client.connect(serverHost, 80)) {
            String postData = "data=" + String(buffer);

            client.print("POST ");
            client.print(serverPath);
            client.println(" HTTP/1.1");
            
            client.print("Host: ");
            client.println(serverHost);
            
            client.println("Content-Type: application/x-www-form-urlencoded");
            client.print("Content-Length: ");
            client.println(postData.length());
            client.println("Connection: close");
            client.println();
            client.print(postData);

            unsigned long timeout = millis();
            while (client.available() == 0) {
                if (millis() - timeout > 5000) {
                    client.stop();
                    return;
                }
            }

            while (client.available()) {
                char c = client.read();
                Serial.print(c);
            }
            client.stop();
        } else {
            Serial.println("Koneksi Gagal. Cek Kabel Internet/Gateway!");
        }
    }
}