#include "WiFi.h"
#include "AsyncUDP.h"

#include <initWiFi.h>
#include "I2SaudIO.h"

AsyncUDP udp;

void hexdump(uint8_t *buffer, size_t len) {   // $ hd -x
const int columns = 16;
int i;

  Serial.println();
  for (i=0; i<len; ++i) {
    Serial.printf("0x%02x ", buffer[i]);
    if ((i) && (i % columns) == (columns - 1)) {
      Serial.print("  "); 
      for (int j = i - columns; j<i; j++) {
        if (isPrintable(buffer[i])) {
          Serial.printf("%c ",buffer[i]);
        } else {
          Serial.print(". ");
        }
      }
      Serial.println();
    }
  }
}

// feed packet to I2S buffer (-->speaker)
void sayPacket(uint8_t *data, size_t length) {
int bytes_written;  

  err = i2s_write(I2S_PORT1, (void *)data, length, (size_t *)&bytes_written, portMAX_DELAY);
    if (err != ESP_OK) {
      Serial.printf("i2s_write() returns error: %d\n", err);
    }
}

void setup()
{

    Serial.begin(115200);

    initWiFi();

    setupI2S();
    
    if(udp.listenMulticast(IPAddress(239,1,1,234), 8888)) {
        Serial.print("UDP Listening on IP: ");
        Serial.println(WiFi.localIP());
        udp.onPacket([](AsyncUDPPacket packet) {
            Serial.print("UDP Packet Type: ");
            Serial.print(packet.isBroadcast()?"Broadcast":packet.isMulticast()?"Multicast":"Unicast");
            Serial.print(", From: ");
            Serial.print(packet.remoteIP());
            Serial.print(":");
            Serial.print(packet.remotePort());
            Serial.print(", To: ");
            Serial.print(packet.localIP());
            Serial.print(":");
            Serial.print(packet.localPort());
            Serial.print(", Length: ");
            Serial.print(packet.length());
            Serial.print(", Data: ");
//            Serial.write(packet.data(), packet.length());
//            hexdump(packet.data(), packet.length());
            Serial.println();
            sayPacket(packet.data(), packet.length());
            //reply to the client (~20 bytes)
//            packet.printf("Got %u bytes of data", packet.length());
            
        });
        //Send multicast
//        udp.print("Hello!");
    }
}

void loop()
{
// copy from DMA buffer and send.
int i, bytes_read = 0;

// read microphone data buffer 
  err = i2s_read(I2S_PORT0, (void *)&transmitBuffer, (size_t)sizeof(transmitBuffer), (size_t *)&bytes_read, portMAX_DELAY);  // no timeout, wait for it
  if (err != ESP_OK) {
    Serial.printf("i2s_read() returns error: %d.\n", err);
  }

  Serial.printf("read: %d bytes.\n", bytes_read);

  udp.write((uint8_t *)transmitBuffer, (size_t)bytes_read);
}
