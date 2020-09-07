/*
This is a mesh of communication used in bks

Epidemic model:

1. The node who receive a message forward it (3 sec ignore same msg)
2. anyone can send a msg


*/
#include <RH_ASK.h>
#include <SPI.h> // Not actually used but needed to compile

int button1 = 4;

RH_ASK driver;
float t;
String m_type;
void setup()
{
    Serial.begin(9600);  // Debugging only
    if (!driver.init())
         Serial.println("init failed");
    t = millis();
    pinMode(button1, INPUT);

}

void send(String msg) {
  char *m = const_cast<char*>(msg.c_str());
  driver.send((uint8_t *)m, strlen(m));
  Serial.print("message sent: ");
  Serial.println(msg);
  delay(10);
}
void loop()
{
    if (digitalRead(button1) == 1) {
      char* msg = ">0001:msg001";
      send(msg);
    }


    uint8_t buf[12];
    uint8_t buflen = sizeof(buf);
    if (driver.recv(buf, &buflen) ) // Non-blocking
    {
      String msg = String((char*)buf);
      if (millis() - t < 3000 && msg.substring('>', ':') == m_type) {
        ;
      } else {
        if (msg.substring('>', ':') == m_type) {
          // actions to do when receive the message
          send(msg);
        }
        String m_type = msg.substring('>', ':');
        t = millis();
        int i;
        // Message with a good checksum received, dump it.
        Serial.print("Message: ");
        Serial.println(msg);
      }
    }
}
