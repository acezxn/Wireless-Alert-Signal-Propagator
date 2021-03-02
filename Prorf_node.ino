#define VERBOSE = 1
#define BLOCK_SIZE 16

#include <RHEncryptedDriver.h>
#include <RH_RF95.h>
#include <SPI.h>
#include <Speck.h>
#include <string.h>
#include <time.h>

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
int jumper = A2;

float frequency = 915.0; // Broadcast frequency
String mtype = "";
int t = millis();
int counter = millis();

bool listening = false;
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
  SerialUSB.println(millis()-counter);
  if (millis()-counter > 1000){
    counter = millis();
    myDriver.send(msg, len);
    myDriver.waitPacketSent();
    memcpy(sended_cache, (uint8_t *)msg, len);
    SerialUSB.print("Sent using key: ");
    for (int i = 0; i < sizeof(encryptkey); i++)
    {
        SerialUSB.print(encryptkey[i]);
    }
    SerialUSB.println();
    SerialUSB.print("caching: ");
    SerialUSB.println((char *)sended_cache);
  }
}

void msg01_beep() {
  tone(buzzer, 2000);
  delay(100);
  tone(buzzer, 2000);
  delay(100);
  tone(buzzer, 2000);
  delay(100);
  tone(buzzer, 2000);
  delay(50);
  noTone(buzzer);
}

void msg02_beep() {
  tone(buzzer, 1500);
  delay(150);
  tone(buzzer, 1500);
  delay(150);
  tone(buzzer, 1500);
  delay(50);
  noTone(buzzer);
}

void forward() { // forwarding messages
  byte buf[RH_RF95_MAX_MESSAGE_LEN];
  byte len = sizeof(buf);
  // myDriver.send(share_header, sizeof(share_header));
  // myDriver.waitPacketSent();
  // memcpy(sended_cache, (uint8_t* )share_header, sizeof(share_header));
  if (millis()-counter > 1000) {
    uint8_t share_header[21] = {'n', 'o', 'n', 'r', 'e'};
    memcpy(sended_cache, (uint8_t* )share_header, sizeof(share_header));
    // SerialUSB.println("cache deleted");
    // SerialUSB.print("cache: ");
    // SerialUSB.println((char *)sended_cache);
  }
  if (rf95.waitAvailableTimeout(50)) {
    if (myDriver.recv(buf, &len)) {
      SerialUSB.println((char *)buf);
      if (!(millis() - t < 1000)) {              // ignore rapid messages.
        // SerialUSB.println("");
        // SerialUSB.print("cache: ");
        // SerialUSB.println((char *)sended_cache);
        // SerialUSB.println(millis()-counter);
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
            for (int i = 0; i < sizeof(msg1); i++) {
                SerialUSB.print("MSG1: ");
                SerialUSB.println( (int) msg1[i]);
                SerialUSB.print("BUFF: ");
                SerialUSB.println(buf[i]);
            }
                if (memcmp(buf, msg1, sizeof(msg1)) == 0)
                { // if received message is msg1
                    msg01_beep();
                }
                else
                {
                    if (memcmp(buf, msg2, sizeof(msg2)) == 0)
                    { // if received message is msg2
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
    srand(analogRead(0));
    long randNumber;
    for (int i = 0; i < sizeof(encryptkey); i++)
    {
        randNumber = rand() % 257;
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
    pinMode(jumper, INPUT_PULLUP);
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
    pinMode(buzzer, OUTPUT);
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
  if (firstrun) {
    if (digitalRead(jumper)) {
      listening = false;
    } else {
      listening = true;
    }
  }
  if (!listening && firstrun) {
      SerialUSB.println("Sender");
    generate();
    share();
#ifdef VERBOSE
    SerialUSB.println("Key generated");
#endif
    firstrun = false;

  } else {
    if (firstrun) {
        SerialUSB.println("listener");

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
