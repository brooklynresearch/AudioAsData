//tested with Univox AutoLoop 2.0 Home Hearing Loop System, modulated with bell202

#define SIGNAL_LEVEL_PIN A6
#define DEBUG_PIN1 A5
#define DEBUG_PIN2 A4
#define NRF52_BLUE_LED 19

#define BUZZER1_PIN1 11
#define BUZZER1_PIN2 7

#define BUZZER2_PIN1 15
#define BUZZER2_PIN2 16

#include <SPI.h>

extern "C"  //https://forums.adafruit.com/viewtopic.php?f=24&t=125410
{
  void TIMER1_IRQHandler();
  void SAADC_IRQHandler();
}

volatile int16_t adcbuffer; //interrupt one each time. other wise auto restart has aline issue

volatile int16_t historyBuffer[8];

volatile bool newData = false;

bool recent3bits[3];

int recent3Bytes[3];

bool outputBit;

int capturePhase = 0;
int processingValue;

unsigned char CRCtable[256] = {0x00, 0x07, 0x0e, 0x09, 0x1c, 0x1b, 0x12, 0x15, 0x38, 0x3f, 0x36, 0x31, 0x24, 0x23, 0x2a, 0x2d, 0x70, 0x77, 0x7e, 0x79, 0x6c, 0x6b, 0x62, 0x65, 0x48, 0x4f, 0x46, 0x41, 0x54, 0x53, 0x5a, 0x5d, 0xe0, 0xe7, 0xee, 0xe9, 0xfc, 0xfb, 0xf2, 0xf5, 0xd8, 0xdf, 0xd6, 0xd1, 0xc4, 0xc3, 0xca, 0xcd, 0x90, 0x97, 0x9e, 0x99, 0x8c, 0x8b, 0x82, 0x85, 0xa8, 0xaf, 0xa6, 0xa1, 0xb4, 0xb3, 0xba, 0xbd, 0xc7, 0xc0, 0xc9, 0xce, 0xdb, 0xdc, 0xd5, 0xd2, 0xff, 0xf8, 0xf1, 0xf6, 0xe3, 0xe4, 0xed, 0xea, 0xb7, 0xb0, 0xb9, 0xbe, 0xab, 0xac, 0xa5, 0xa2, 0x8f, 0x88, 0x81, 0x86, 0x93, 0x94, 0x9d, 0x9a, 0x27, 0x20, 0x29, 0x2e, 0x3b, 0x3c, 0x35, 0x32, 0x1f, 0x18, 0x11, 0x16, 0x03, 0x04, 0x0d, 0x0a, 0x57, 0x50, 0x59, 0x5e, 0x4b, 0x4c, 0x45, 0x42, 0x6f, 0x68, 0x61, 0x66, 0x73, 0x74, 0x7d, 0x7a, 0x89, 0x8e, 0x87, 0x80, 0x95, 0x92, 0x9b, 0x9c, 0xb1, 0xb6, 0xbf, 0xb8, 0xad, 0xaa, 0xa3, 0xa4, 0xf9, 0xfe, 0xf7, 0xf0, 0xe5, 0xe2, 0xeb, 0xec, 0xc1, 0xc6, 0xcf, 0xc8, 0xdd, 0xda, 0xd3, 0xd4, 0x69, 0x6e, 0x67, 0x60, 0x75, 0x72, 0x7b, 0x7c, 0x51, 0x56, 0x5f, 0x58, 0x4d, 0x4a, 0x43, 0x44, 0x19, 0x1e, 0x17, 0x10, 0x05, 0x02, 0x0b, 0x0c, 0x21, 0x26, 0x2f, 0x28, 0x3d, 0x3a, 0x33, 0x34, 0x4e, 0x49, 0x40, 0x47, 0x52, 0x55, 0x5c, 0x5b, 0x76, 0x71, 0x78, 0x7f, 0x6a, 0x6d, 0x64, 0x63, 0x3e, 0x39, 0x30, 0x37, 0x22, 0x25, 0x2c, 0x2b, 0x06, 0x01, 0x08, 0x0f, 0x1a, 0x1d, 0x14, 0x13, 0xae, 0xa9, 0xa0, 0xa7, 0xb2, 0xb5, 0xbc, 0xbb, 0x96, 0x91, 0x98, 0x9f, 0x8a, 0x8d, 0x84, 0x83, 0xde, 0xd9, 0xd0, 0xd7, 0xc2, 0xc5, 0xcc, 0xcb, 0xe6, 0xe1, 0xe8, 0xef, 0xfa, 0xfd, 0xf4, 0xf3};


void setup() {

  NRF_CLOCK->TASKS_HFCLKSTART = 1; while (!NRF_CLOCK->EVENTS_HFCLKSTARTED) {} //increase accuracy

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(NRF52_BLUE_LED, OUTPUT);

  analogWrite(NRF52_BLUE_LED, 128); //set up PWM, This should be on channel 0 of HwPWM0
  analogWrite(LED_BUILTIN, 128); //channel 1 of HwPWM0

  pinMode(7, OUTPUT);
  pinMode(SIGNAL_LEVEL_PIN, OUTPUT);
  pinMode(DEBUG_PIN1, OUTPUT);
  pinMode(DEBUG_PIN2, OUTPUT);

  pinMode(BUZZER1_PIN1, OUTPUT);
  pinMode(BUZZER1_PIN2, OUTPUT);

  pinMode(BUZZER2_PIN1, OUTPUT);
  pinMode(BUZZER2_PIN2, OUTPUT);

  Serial.begin(115200);

  //set up PWM
  HwPWM2.begin();
  NRF_PWM2->MODE = (PWM_MODE_UPDOWN_UpAndDown << PWM_MODE_UPDOWN_Pos);

  HwPWM2.setClockDiv(PWM_PRESCALER_PRESCALER_DIV_64);

  HwPWM2.addPin(BUZZER1_PIN1);  //add motorPWM to PWM2
  HwPWM2.addPin(BUZZER1_PIN2);  //add motorPWM to PWM2

  HwPWM2.addPin(BUZZER2_PIN1);  //add motorPWM to PWM2
  HwPWM2.addPin(BUZZER2_PIN2);  //add motorPWM to PWM2

  HwPWM2.setMaxValue(1250); //freq: 16E6/64/1250/2
  HwPWM2.writeChannel(0, 1, false);
  HwPWM2.writeChannel(1, 1250 - 1, true);
  HwPWM2.writeChannel(2, 1, false);
  HwPWM2.writeChannel(3, 1250 - 1, true);

  ppi_init();
  timer1_init(); // Timer used to increase m_counter every 100ms.
  saadc_init();
  saadc_read();// Trigger first SAADC read

  SPI.begin();
  SPI.setClockDivider((F_CPU + 4000000L) / 2000000L); // 2-ish MHz on Due
  SPI.setBitOrder(MSBFIRST);
  SPI.setDataMode(SPI_MODE0);

  
  analogWrite(NRF52_BLUE_LED, 0); //set up PWM, This should be on channel 0 of HwPWM0
  analogWrite(LED_BUILTIN, 0); //channel 1 of HwPWM0

}



void loop() {
  bool ledUpdate = false;
  int led1Value, led2Value;

  if (newData) {
    int32_t demodulateValue = demodulate8Byte((int16_t *)historyBuffer);

#define THRES_MAX   1000L
#define THRES_MIN  -1000L

    if (outputBit == 0) {
      if (demodulateValue > THRES_MAX) outputBit = 1;
    } else {
      if (demodulateValue < THRES_MIN) outputBit = 0;
    }

    unsigned char dataBuffer;
    unsigned char clock_regen_result = clock_regen(outputBit, &dataBuffer);
    static unsigned char receivedData[5];
    static int receivedLength = 0;
    if (clock_regen_result == 1) {
      for (int i = 0; i < 4; i++) {
        receivedData[i] = receivedData[i + 1];
      }
      receivedData[4] = dataBuffer;
      receivedLength++;
      if (receivedLength >= 5) {
        unsigned char crcValue = 0;
        for (int i = 0; i < 4; i++) {
          crcValue = CRCtable[ (crcValue ^ receivedData[i]) ];
        }
        if (receivedData[4] == crcValue) {
          //Serial.println(dataBuffer,HEX);
          ledUpdate = true;
          led1Value = receivedData[0];
          led2Value = receivedData[1];

          //set Motor
          int pwmMaxValue = 125000 / receivedData[2];
          HwPWM2.setMaxValue(pwmMaxValue); //freq: 16E6/64/1250/2
          int pwmValue = pwmMaxValue * receivedData[3] >> 9;
          HwPWM2.writeChannel(0, pwmValue, false);
          HwPWM2.writeChannel(1, pwmMaxValue - pwmValue, true);
          HwPWM2.writeChannel(2, pwmValue, false);
          HwPWM2.writeChannel(3, pwmMaxValue - pwmValue, true);

          receivedLength = 0;
          memset(receivedData, 0, 5);
        }
      }
    } else if (clock_regen_result == 2) {
      //10 bit reset
      receivedLength = 0;
    }

    digitalWrite(SIGNAL_LEVEL_PIN, outputBit);

    newData = false;
  }

  if (ledUpdate) {
    sendLEDValue(led1Value, led1Value, led1Value, led2Value, led2Value, led2Value);
  }
}

#define PHASE_INC 256/8 // 2^(length of phase register)/samples per bit
#define PHASE_CORR PHASE_INC/2

unsigned char clock_regen(uint8_t r, uint8_t *result) {
  unsigned char returnVal = 0;
  static uint8_t bit_phase;
  static uint8_t oldr;
  static uint8_t oldoldr;
  uint16_t temp;
  if (oldr != r) { //edge detection
    if (bit_phase < 0x80) {
      bit_phase += PHASE_CORR;
    } else {
      bit_phase -= PHASE_CORR;
    }
  }
  temp = bit_phase + PHASE_INC;
  bit_phase = temp & 0xff;
  if (temp > 0xff) { //sample at half cycle
    //sample 2 in 3

    uint8_t TMRdecode = ((r + oldr + oldoldr) >= 2) ? 1 : 0;

    //
    static int idleBitCount = 1;
    static int sampleBitCount = 0;
    static unsigned char dataBuffer = 0;

    if (idleBitCount > 0) {
      if (!TMRdecode) { //start bit
        sampleBitCount = 0;
        idleBitCount = 0;
      } else {
        idleBitCount++;
        if (idleBitCount >= 10) {
          idleBitCount = 10;
          returnVal = 2;
          dataBuffer = 0;
        }
      }
    } else {
      dataBuffer = dataBuffer >> 1;
      if (TMRdecode) dataBuffer |= 0x80;
      sampleBitCount++;
      if (sampleBitCount == 8) {
        returnVal = 1;
        *result = dataBuffer;
      } else if (sampleBitCount == 9) {
        idleBitCount = 1;
      }

    }
  }
  oldoldr = oldr;
  oldr = r;

  return returnVal;
}


int16_t Coeffloi[] = {32767, 23169, 0, -23169, -32767, -23169, 0, 23169}; //1200 Hz tone for mark (typically a binary 1)
int16_t Coeffloq[] = {0, 23169, 32767, 23169, 0, -23169, -32767, -23169};
int16_t Coeffhii[] = {32767, 4276, -31650, -12539, 28377, 19947, -23169, //2200 Hz for space (typically a binary 0)
                      -25995
                     };
int16_t Coeffhiq[] = {0, 32486, 8480, -30272, -16383, 25995, 23169, -19947};

int32_t demodulate8Byte(int16_t *data) {
  int32_t outloi, outloq, outhii, outhiq;
  int16_t d;
  uint8_t i;

  outloi = 0;
  outloq = 0;
  outhii = 0;
  outhiq = 0;

  for (i = 0; i < 8; i++) {
    d = data[i];
    outloi += d * Coeffloi[i];
    outloq += d * Coeffloq[i];
    outhii += d * Coeffhii[i];
    outhiq += d * Coeffhiq[i];
  }

  return (outloi >> 16) * (outloi >> 16) + (outloq >> 16) * (outloq >> 16) - ((outhii >> 16) * (outhii >> 16) + (outhiq >> 16) * (outhiq >> 16));
}

