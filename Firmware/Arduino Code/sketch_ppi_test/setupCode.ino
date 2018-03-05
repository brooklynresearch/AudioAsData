
void ppi_init() {
  //link timer 1 with ADC
  NRF_PPI->CH[2].EEP = (uint32_t)&NRF_TIMER1->EVENTS_COMPARE[0];  //auto restart
  NRF_PPI->CH[2].TEP = (uint32_t)&NRF_SAADC->TASKS_SAMPLE;
  NRF_PPI->FORK[2].TEP = (uint32_t)&NRF_SAADC->TASKS_START;
  NRF_PPI->CHEN |= PPI_CHENSET_CH2_Enabled << PPI_CHENSET_CH2_Pos;
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
                            | ((SAADC_CH_CONFIG_BURST_Disabled  << SAADC_CH_CONFIG_BURST_Pos)  & SAADC_CH_CONFIG_BURST_Msk);


  // Configure sample rate. Minimum sample rate is 16,000,000/2047 = 7 816.3 Samples pr second => 0.128ms interval
  NRF_SAADC->SAMPLERATE = ( SAADC_SAMPLERATE_MODE_Timers << SAADC_SAMPLERATE_MODE_Pos) |
                          ( 300 << SAADC_SAMPLERATE_CC_Pos );  //get it fast

  /* RAM Bufffer pointer for ADC Result*/
  NRF_SAADC->RESULT.PTR  = (uint32_t)&adcbuffer;

  /* Number of Sample Count*/
  NRF_SAADC->RESULT.MAXCNT = 1;

  // Enable SAADC END interrupt to do maintainance and printing of values.
  NRF_SAADC->INTENSET = SAADC_INTENSET_END_Enabled << SAADC_INTENSET_END_Pos;
  NVIC_EnableIRQ(SAADC_IRQn);
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
  for (int i = 0; i < 7; i++) {
    historyBuffer[i] = historyBuffer[i + 1];
  }
  historyBuffer[7] = adcbuffer - (509);
  newData = true;
}

void TIMER1_IRQHandler() {  //generate 9600hz interrupt
  digitalToggle(7);
  if (NRF_TIMER1->EVENTS_COMPARE[0]) NRF_TIMER1->EVENTS_COMPARE[0] = 0;

}


void timer1_init(void)
{
  NRF_TIMER1->TASKS_STOP     = 1;                       // Stop timer.
  NRF_TIMER1->MODE           = TIMER_MODE_MODE_Timer;   // Set the timer in Timer Mode.
  NRF_TIMER1->PRESCALER      = 0 << TIMER_PRESCALER_PRESCALER_Pos;      // Prescaler 0 produces 16 MHz.
  NRF_TIMER1->BITMODE        = TIMER_BITMODE_BITMODE_16Bit << TIMER_BITMODE_BITMODE_Pos;        // 16 bit mode.

  NRF_TIMER1->CC[0] = 1666;
  NRF_TIMER1->INTENSET = TIMER_INTENSET_COMPARE0_Enabled << TIMER_INTENSET_COMPARE0_Pos;

  // Clear the timer when COMPARE0 event is triggered
  NRF_TIMER1->SHORTS = TIMER_SHORTS_COMPARE0_CLEAR_Enabled << TIMER_SHORTS_COMPARE0_CLEAR_Pos;

  NRF_TIMER1->TASKS_CLEAR    = 1;                         // clear the task first to be usable for later.
  NRF_TIMER1->TASKS_START   = 1;                        // Start timer.

  //NVIC_SetPriority(TIMER1_IRQn, 1);
  //NVIC_EnableIRQ(TIMER1_IRQn);
}
