#include <Arduino.h>

#define SAMPLES_IN_BUFFER 8

extern "C"  //https://forums.adafruit.com/viewtopic.php?f=24&t=125410
{
  void SAADC_IRQHandler();
}


static int16_t m_buffer_pool[2][SAMPLES_IN_BUFFER];

volatile bool saadc_results_ready = false;
volatile uint8_t saadc_use_buffer = 0;

// the setup function runs once when you press reset or power the board
void setup() {
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(7, OUTPUT);
  pinMode(11, OUTPUT);

  digitalWrite(7, HIGH);
  digitalWrite(11, HIGH);

  Serial.begin(115200);

  digitalWrite(7, LOW);



  saadc_init();
  digitalWrite(7, HIGH);
  // Use RTC1 to time SAADC sampling


  digitalWrite(7, LOW);
  // Trigger first SAADC read
  saadc_read();

  digitalWrite(7, HIGH);

}


// the loop function runs over and over again forever
void loop() {



  static bool ledState = LOW;
  static unsigned long previousMillis = 0;
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= 200) {
    previousMillis = currentMillis;
    if (ledState == LOW) {
      ledState = HIGH;
    } else {
      ledState = LOW;
    }
    digitalWrite(LED_BUILTIN, ledState);
  }

  if (saadc_results_ready) {
    for (int i = 0; i < SAMPLES_IN_BUFFER; i++) {
      Serial.print(m_buffer_pool[saadc_use_buffer][i]);
      Serial.print(' ');
    }
    Serial.println();
    // Clear flag and wait for new event.
    saadc_results_ready = false;
  }

}
void saadc_read()
{
  // Enable SAADC. This should be done after the SAADC is configure due to errata 74 SAADC: Started events fires prematurely
  NRF_SAADC->ENABLE = (SAADC_ENABLE_ENABLE_Enabled << SAADC_ENABLE_ENABLE_Pos);

  // Start the ADC Start TASK and Sample TASK
  NRF_SAADC->TASKS_START = 0x01UL;
  while (NRF_SAADC->EVENTS_STARTED == 0)
    ;
  NRF_SAADC->EVENTS_STARTED = 0;
  NRF_SAADC->TASKS_SAMPLE = 1;
}


void SAADC_IRQHandler(void) {
  // Clear events
  NRF_SAADC->EVENTS_END = 0;

  // Alternate between buffers so that we can read and print one buffer while filling the other at the same tim
  if (saadc_use_buffer == 0)
  {
    // Change to second buffer
    NRF_SAADC->RESULT.PTR  = (uint32_t)&m_buffer_pool[1][0];
    saadc_use_buffer = 1;
  }
  else
  {
    // Change to first buffer
    NRF_SAADC->RESULT.PTR  = (uint32_t)&m_buffer_pool[0][0];
    saadc_use_buffer = 0;
  }

  saadc_results_ready = true;

  digitalToggle(7);
}

void saadc_init(void)
{
  NVIC_EnableIRQ(SAADC_IRQn);
  NVIC_ClearPendingIRQ(SAADC_IRQn);
  NRF_SAADC->RESOLUTION = SAADC_RESOLUTION_VAL_10bit;

  // Configure the SAADC channel with AIN0 (P0.02) as positive input, no negative input(single ended).
  NRF_SAADC->CH[0].PSELP = SAADC_CH_PSELP_PSELP_AnalogInput0 << SAADC_CH_PSELP_PSELP_Pos;
  NRF_SAADC->CH[0].PSELN = SAADC_CH_PSELN_PSELN_NC << SAADC_CH_PSELN_PSELN_Pos;

  NRF_SAADC->CH[0].CONFIG = ((SAADC_CH_CONFIG_RESP_Bypass     << SAADC_CH_CONFIG_RESP_Pos)   & SAADC_CH_CONFIG_RESP_Msk)
                            | ((SAADC_CH_CONFIG_RESP_Bypass     << SAADC_CH_CONFIG_RESN_Pos)   & SAADC_CH_CONFIG_RESN_Msk)
                            | ((SAADC_CH_CONFIG_GAIN_Gain1_6    << SAADC_CH_CONFIG_GAIN_Pos)   & SAADC_CH_CONFIG_GAIN_Msk)
                            | ((SAADC_CH_CONFIG_REFSEL_Internal << SAADC_CH_CONFIG_REFSEL_Pos) & SAADC_CH_CONFIG_REFSEL_Msk)
                            | ((SAADC_CH_CONFIG_TACQ_3us        << SAADC_CH_CONFIG_TACQ_Pos)   & SAADC_CH_CONFIG_TACQ_Msk)
                            | ((SAADC_CH_CONFIG_MODE_SE         << SAADC_CH_CONFIG_MODE_Pos)   & SAADC_CH_CONFIG_MODE_Msk)
                            | ((SAADC_CH_CONFIG_BURST_Disabled	<< SAADC_CH_CONFIG_BURST_Pos)  & SAADC_CH_CONFIG_BURST_Msk);


  // Configure sample rate. Minimum sample rate is 16,000,000/2047 = 7 816.3 Samples pr second => 0.128ms interval
  NRF_SAADC->SAMPLERATE = ( SAADC_SAMPLERATE_MODE_Timers << SAADC_SAMPLERATE_MODE_Pos) |
                          ( 511 << SAADC_SAMPLERATE_CC_Pos );  //auto restart: 2047:0.8933ms 1023:0.4468ms 511:0.223ms  why there is an offset?

  /* RAM Bufffer pointer for ADC Result*/
  NRF_SAADC->RESULT.PTR  = (uint32_t)&m_buffer_pool;

  /* Number of Sample Count*/
  NRF_SAADC->RESULT.MAXCNT = SAMPLES_IN_BUFFER;

  // Enable SAADC END interrupt to do maintainance and printing of values.
  NRF_SAADC->INTENSET = SAADC_INTENSET_END_Enabled << SAADC_INTENSET_END_Pos;
  NVIC_EnableIRQ(SAADC_IRQn);

  NRF_PPI->CH[2].EEP = (uint32_t)&NRF_SAADC->EVENTS_END;  //auto restart
  NRF_PPI->CH[2].TEP = (uint32_t)&NRF_SAADC->TASKS_SAMPLE;
  NRF_PPI->FORK[2].TEP = (uint32_t)&NRF_SAADC->TASKS_START;
  NRF_PPI->CHEN |= PPI_CHENSET_CH2_Enabled << PPI_CHENSET_CH2_Pos;
}


