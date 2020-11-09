#define VERBOSE = 1
#define BLOCK_SIZE 16

#include <RHEncryptedDriver.h>
#include <RH_RF95.h>
#include <SPI.h>
#include <Speck.h>
#include <string.h>

byte encryptkey[16] = {1, 2,  3,  4,  5,  6,  7,  8,
                       9, 10, 11, 12, 13, 14, 15, 16}; // The very secret key
byte sended_cache[RH_RF95_MAX_MESSAGE_LEN];

uint8_t share_header[21] = {'s', 'h', 'a', 'r', 'e'};

uint8_t msg1[5] = {'m', 's', 'g', '0', '1'};
uint8_t msg2[5] = {'m', 's', 'g', '0', '2'};

RH_RF95 rf95(12, 6);
Speck myCipher; // Instanciate a Speck block ciphering
RHEncryptedDriver myDriver(rf95,
                           myCipher); // Instantiate the driver with those two

int msglen = 5;
int LED = 13; // Status LED is on pin 13
int firstpin = 2;
int secondpin = 3;
int buzzer = 5;

float frequency = 915.0; // Broadcast frequency
String mtype = "";
int t = millis();

bool listening = true;
int debounce_timer;
bool pressing = false;

void init_radio() {
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
}

void broadcast(uint8_t *msg, int len) { // send message
  byte sended_cache[RH_RF95_MAX_MESSAGE_LEN];
  myDriver.send(msg, len);
  myDriver.waitPacketSent();
  memcpy(sended_cache, (uint8_t *)msg, len);
}

void msg01_beep() {
  tone(buzzer, 2000);
  noTone(buzzer);
  delay(100);
  tone(buzzer, 2000);
  noTone(buzzer);
  delay(100);
  tone(buzzer, 2000);
  noTone(buzzer);
  delay(100);
  tone(buzzer, 2000);
  noTone(buzzer);
  delay(50);
}

void msg02_beep() {
  tone(buzzer, 1000);
  noTone(buzzer);
  delay(150);
  tone(buzzer, 1000);
  noTone(buzzer);
  delay(150);
  tone(buzzer, 1000);
  noTone(buzzer);
  delay(50);
}

void forward() { // forwarding messages
  byte buf[RH_RF95_MAX_MESSAGE_LEN];
  byte len = sizeof(buf);
  // myDriver.send(share_header, sizeof(share_header));
  // myDriver.waitPacketSent();
  // memcpy(sended_cache, (uint8_t* )share_header, sizeof(share_header));
  if (rf95.waitAvailableTimeout(50)) {
    if (myDriver.recv(buf, &len)) {
      SerialUSB.println((char *)buf);
      if (!(millis() - t < 1000)) {              // ignore rapid messages.
        if (memcmp(buf, sended_cache, 5) != 0) { // ignore sent msgs
          t = millis();
          digitalWrite(LED_BUILTIN, HIGH);
          if (memcmp(buf, msg1, 4) == 0 || memcmp(buf, msg2, 4) == 0) { // check if received message is valid
#ifdef VERBOSE
            SerialUSB.print("Forwarding Message: ");
            SerialUSB.println((char *)buf);
#endif
            myDriver.send(buf, len);
            myDriver.waitPacketSent();
            digitalWrite(LED_BUILTIN, LOW);
            if (memcmp(buf, msg1, 4) == 0) { // if received message is msg1
              msg01_beep();
            } else {
              if (memcmp(buf, msg2, 4) == 0) { // if received message is msg2
                msg02_beep();
              }
            }
          }
        }
      }
    }
  }
}

void generate() { // generate key
  long randNumber;
  for (int i = 0; i < sizeof(encryptkey); i++) {
    randNumber = random(0, 256);
    encryptkey[i] = randNumber;
  }
  myCipher.setKey(encryptkey, sizeof(encryptkey));
}
void share() { // share key
  digitalWrite(LED_BUILTIN, HIGH);
  for (int i = 5; i < sizeof(share_header); i++) {
    share_header[i] = encryptkey[i - 5];
  }
  for (int i = 0; i < 20; i++) { // keep sharing in 10 sec
    rf95.send(share_header, sizeof(share_header));
    rf95.waitPacketSent();
    delay(500);
  }
  digitalWrite(LED_BUILTIN, LOW);
}
void listen() { // listen for key
  while (true) {
    byte buf[RH_RF95_MAX_MESSAGE_LEN];
    byte len = sizeof(buf);
    bool is_key = true;
    if (rf95.waitAvailableTimeout(2000)) {
      if (rf95.recv(buf, &len)) {
        for (int i = 0; i < 5; i++) { // checking whether the header is the share header
          if ((char)buf[i] != share_header[i]) {
            is_key = false;
            break;
          }
        }
        if (is_key) { // if received a key
          for (int i = 5; i < sizeof(share_header); i++) {
            encryptkey[i - 5] = buf[i];
          }
          for (int i = 0; i < sizeof(encryptkey); i++) {
            SerialUSB.print(encryptkey[i]);
          }
          myCipher.setKey(encryptkey, sizeof(encryptkey));
          listening = false;
          break;
        }
      }
    }
  }
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(firstpin, INPUT);
  pinMode(secondpin, INPUT);
  SerialUSB.begin(115200); // 9600
  SerialUSB.println("RFM Client!");

  // Initialize the Radio.
  init_radio();

  rf95.setModemConfig(RH_RF95::Bw500Cr45Sf128); // High bandwidth
  // rf95.setModemConfig(RH_RF95::Bw31_25Cr48Sf512); //Long range
  // Set frequency
  rf95.setFrequency(frequency);
  rf95.setTxPower(23, false);
  myCipher.setKey(encryptkey, sizeof(encryptkey));

  // generate(); // uncomment this to generate key automatically every start
  debounce_timer = millis();
}

void detect_press() {
  if (digitalRead(firstpin) && !pressing &&
      millis() - debounce_timer > 100) { // pressed button #1
    debounce_timer = millis();
    pressing = true;
    broadcast(msg1, 5);

  } else {
    if (digitalRead(secondpin) && !pressing && // pressed button #2
        millis() - debounce_timer > 100) {
      debounce_timer = millis();
      pressing = true;
      broadcast(msg2, 5);
    } else {
      if (!digitalRead(firstpin) && // both not pressing
          !digitalRead(secondpin)) {
        pressing = false;
      }
    }
  }
}

bool firstrun = true;

void loop() {
  // listening: no jumper
  if (!listening && firstrun) {
    generate();
    share();
#ifdef VERBOSE
    SerialUSB.println("Key generated");
#endif
    firstrun = false;

  } else {
    if (firstrun) {
      listen(); // listen until receiving a key
#ifdef VERBOSE
      SerialUSB.println("Key received");
#endif
      digitalWrite(LED_BUILTIN, HIGH);
      firstrun = false;
    }
  }
  detect_press();
  forward();
  // if (!listening && !firstrun) {
  //   broadcast(msg1, 5);
  //   digitalWrite(LED_BUILTIN, HIGH);
  //   delay(1000);
  //   digitalWrite(LED_BUILTIN, LOW);
  // }
}
