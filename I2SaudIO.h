#ifndef I2SAUDIO_H
#define I2SAUDIO_H

#ifndef MEMS
	#define MEMS true
#endif
	
#define WROOM32 true	// buttons on solder side
//#define FakeWeMOS true	// buttons on comp side

#include <driver/i2s.h>


#if defined (WROOM32)
// new pinouts -- avoid strapping pins, esp. 0, 2, 12, & 15.
// gpio 4,5 not available. These are all on one side of PCB.
  const uint8_t MEMS_BCKL =  5, // was 12,
                MEMS_WSIO =  4, // was 13,
                MEMS_DIN  = 39, // in only // was  0,
                XDAC_BCKL = 14,
                XDAC_WSIO = 16,
                XDAC_DOUT = 13; // was 15; // was 2;
#endif
#if defined (FakeWeMOS)
  #define FLIP true
  #define H 0
  #define W 0
// 4, 15, 16 not available.
  const uint8_t MEMS_BCKL = 32,
                MEMS_WSIO = 33,
                MEMS_DIN  = 34, // input only pin
                XDAC_BCKL = 27, // was 12,
                XDAC_WSIO = 13,
                XDAC_DOUT = 14;
#endif 

// i2s input configuration

const i2s_port_t I2S_PORT0 = I2S_NUM_0;

#if MEMS
  // The I2S config for INMP441 MEMS Mic
  const i2s_config_t i2s_config_in = {
      .mode = i2s_mode_t(I2S_MODE_MASTER | I2S_MODE_RX), // Receive, not transfer
      .sample_rate = 8000,                         //
      .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT, // INMP441 mic needs all 32 clocks per word.
      .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT, // with L/R pin tied low
      .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB), // shift data MSB-first
      .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,     // Interrupt level 1
      .dma_buf_count = 4,                           // number of buffers
      .dma_buf_len = 8                              // 8 samples per buffer (minimum)
  };

  // The pin config depends on which board
  const i2s_pin_config_t pin_config_in = {
      .bck_io_num = MEMS_BCKL,   // BCKL
      .ws_io_num = MEMS_WSIO,    // LRCL
      .data_out_num = -1, // not used (only for speakers)
      .data_in_num = MEMS_DIN    // DOUT
  };
#else	// ! MEMS, then ADC input

  // I2S config for ADC input
  const i2s_config_t i2s_config_in = {
      .mode = i2s_mode_t(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_ADC_BUILT_IN),
      .sample_rate = 8000,
      .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,     // put 12-bit samples into 32-bit word
      .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
      .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
      .intr_alloc_flags = 0,
      .dma_buf_count = 4,
      .dma_buf_len = 8
  };

#endif  

// i2s output configuration 

const i2s_port_t I2S_PORT1 = I2S_NUM_1;

i2s_config_t i2s_config_out = {
     .mode = i2s_mode_t(I2S_MODE_MASTER | I2S_MODE_TX),
     .sample_rate = 8000,
     .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
     .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,   // was RIGHT_LEFT,
     .communication_format = (i2s_comm_format_t)(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
     .intr_alloc_flags = 0, // default interrupt priority
     .dma_buf_count = 4,    // was 8,
     .dma_buf_len = 8,      // was 64,
     .use_apll = 0
    };


i2s_pin_config_t pin_config_out = {
    .bck_io_num = XDAC_BCKL,
    .ws_io_num = XDAC_WSIO,
    .data_out_num = XDAC_DOUT,
    .data_in_num = -1      
};
 
const int BUFFER_MAX = (i2s_config_in.dma_buf_count * i2s_config_in.dma_buf_len);   // was 512;
int32_t transmitBuffer[BUFFER_MAX];                                                 // was uint8_t

esp_err_t err = ESP_OK;

// setup I2S read.
void setupI2S() {
  Serial.println("Configuring I2S...");

  // Configuring the I2S driver and pins.
  // This function must be called before any I2S driver read/write operations.
  err = i2s_driver_install(I2S_PORT0, &i2s_config_in, 0, NULL);
  if (err != ESP_OK) {
    Serial.printf("Failed installing input driver: %d\n", err);
    while (true)yield();
  }

#if MEMS
  
  err = i2s_set_pin(I2S_PORT0, &pin_config_in);
  if (err != ESP_OK) {
    Serial.printf("Failed setting input pin: %d\n", err);
    while (true)yield();
  }

#else

  adc1_config_width(ADC_WIDTH_12Bit);
  adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_11db); //ADC 1 channel 0 GPIO36 (aka svp)
  
//                            UNIT,           CHANNEL
  err = i2s_set_adc_mode((adc_unit_t)1, (adc1_channel_t)0);
  if (err != ESP_OK) {
    Serial.printf("Failed setting ADC UNIT and CHANNEL: %d\n", err);
    while (true)yield();
  }
  
#endif  
  Serial.println("I2S input driver installed.");

// output driver setup

//initialize i2s with configurations above
  i2s_driver_install((i2s_port_t)I2S_NUM_1, &i2s_config_out, 0, NULL);
  i2s_set_pin((i2s_port_t)I2S_NUM_1, &pin_config_out);
  
//set sample rates of i2s to sample rate of input
  i2s_set_sample_rates((i2s_port_t)I2S_NUM_1, i2s_config_in.sample_rate);   // make in/out rates match

  Serial.printf("I2S output driver installed.");
  
/*  if loop ever gives up...
  i2s_driver_uninstall(I2S_PORT0);
  i2s_driver_uninstall(I2S_PORT1);
*/    
}

#endif // I2SAUDIO_H
