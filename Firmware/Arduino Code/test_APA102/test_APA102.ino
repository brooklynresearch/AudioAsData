
#include <SPI.h>


// the setup function runs once when you press reset or power the board
void setup() {
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);

  pinMode(12, OUTPUT);
  pinMode(13, OUTPUT);


  SPI.begin();

  SPI.setClockDivider((F_CPU + 4000000L) / 1000000L); // 1-ish MHz on Due

  SPI.setBitOrder(MSBFIRST);
  SPI.setDataMode(SPI_MODE0);

}

void sendLEDValue(uint8_t r1, uint8_t g1, uint8_t b1, uint8_t r2, uint8_t g2, uint8_t b2) {
  SPI.transfer(0);
  SPI.transfer(0);
  SPI.transfer(0);
  SPI.transfer(0);
  SPI.transfer(0xFF);
  SPI.transfer(b1);
  SPI.transfer(g1);
  SPI.transfer(r1);
  SPI.transfer(0xFF);
  SPI.transfer(b2);
  SPI.transfer(g2);
  SPI.transfer(r2);
  SPI.transfer(0xFF);
}

// the loop function runs over and over again forever
void loop() {
  digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
  digitalWrite(13, HIGH);

  sendLEDValue(0, 0, 50, 0, 0, 0);

  delay(20);                       // wait for a second
  digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
  digitalWrite(13, LOW);

  sendLEDValue(0, 0, 0, 50, 0, 0);

  delay(20);                       // wait for a second
}
