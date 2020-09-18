/*
  Both the TX and RX ProRF boards will need a wire antenna. We recommend a 3"
  piece of wire. This example is a modified version of the example provided by
  the Radio Head Library which can be found here:
  www.github.com/PaulStoffregen/RadioHeadd
*/
#define VERBOSE = 1;
#include <RH_RF95.h>
#include <SPI.h>

RH_RF95 rf95(12, 6);

int LED = 13; // Status LED is on pin 13

float frequency = 915.0; // Broadcast frequency

void setup() {
  pinMode(LED, OUTPUT);

  SerialUSB.begin(115200); // 9600
  SerialUSB.println("RFM Client!");

  // Initialize the Radio.
  if (rf95.init() == false) {
    SerialUSB.println("Radio Init Failed - Freezing");
    while (1)
      ;
  } else {
    // An LED inidicator to let us know radio initialization has completed.
    SerialUSB.println("Transmitter up!");
    digitalWrite(LED, HIGH);
    delay(500);
    digitalWrite(LED, LOW);
    delay(500);
  }

  rf95.setModemConfig(RH_RF95::Bw500Cr45Sf128); // High bandwidth
  // rf95.setModemConfig(RH_RF95::Bw31_25Cr48Sf512); //Long range
  // Set frequency
  rf95.setFrequency(frequency);
  rf95.setTxPower(23, false);
}
int t;
double ignore = millis();
String mtype = "";
String proto = "123:";

void loop() {
#ifdef VERBOSE
  SerialUSB.println("\n Sending message ");
#endif

  uint8_t toSend[] = {'>', '1', '2', '3', ':', 'm', 's', 'g', '0', '1', '<'};
  t = millis();
  rf95.send(toSend, sizeof(toSend));
  rf95.waitPacketSent();
  t = millis() - t;
#ifdef VERBOSE
  SerialUSB.print("Send duration: ");
  SerialUSB.print(t, DEC);
#endif

  t = millis();
  // Now wait for a reply
  byte buf[RH_RF95_MAX_MESSAGE_LEN];
  byte len = sizeof(buf);

  if (rf95.waitAvailableTimeout(2000)) { // Not time out
    // Should be a reply message for us now
    if (rf95.recv(buf, &len)) { // receive message
      String data = (String)((char *)buf);
      if (1) { // data.substring('>', ':') == proto
        SerialUSB.print("Proto: ");
        SerialUSB.println(data.substring('>', ':'));

        String tag = data.substring('m', '<');
        SerialUSB.print("Tag name: ");
        SerialUSB.println(tag);
        if (millis() - ignore <= 3000) { // tag == mtype &&
          ;
        } else {
          tag = mtype;
          ignore = millis();
          t = millis() - t;
#ifdef VERBOSE
          SerialUSB.println("message: ");
          SerialUSB.println((char *)buf);

          rf95.send(buf, sizeof(buf));
          rf95.waitPacketSent();

#endif
        }
      }

    } else {
#ifdef VERBOSE
      SerialUSB.println("Receive failed");
#endif
    }
  } else {
  }
}
