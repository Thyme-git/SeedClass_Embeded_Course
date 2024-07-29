      
# include "freertos/FreeRTOS.h"
# include "freertos/task.h"
#include "freertos/semphr.h"
# include "display.h"
# include "esp_log.h"
# include "fft.h"
# include <math.h>

#define LED_MAX_NBER_LEDS 8*16
#define LED_DMA_BUFFER_SIZE ((LED_MAX_NBER_LEDS * 16 * (24/4)))+1
#define LED_PIN GPIO_NUM_27
typedef struct {
	spi_host_device_t host;
	spi_device_handle_t spi;
	int dma_chan;
	spi_device_interface_config_t devcfg;
	spi_bus_config_t buscfg;
} SPI_settings_t;
uint16_t* ledDMAbuffer;
uint32_t display_buffer[LED_MAX_NBER_LEDS];
static SPI_settings_t SPI_settings = {
		.host = HSPI_HOST,
		.dma_chan = 2,
		.buscfg = {
				.miso_io_num = -1,
				.mosi_io_num = LED_PIN,
				.sclk_io_num = -1,
				.quadwp_io_num = -1,
				.quadhd_io_num = -1,
				.max_transfer_sz = LED_DMA_BUFFER_SIZE
		},
		.devcfg = {
				.clock_speed_hz = 3.2 * 1000 * 1000, // 3.2 MHz
				.mode = 0, //SPI mode 0
				.spics_io_num = -1, // CS pin
				.queue_size = 1, //Not sure if needed
				.command_bits = 0,
				.address_bits = 0
		}
};

// static CRGB leds[LED_MAX_NBER_LEDS];
void initSPIws2812()
{
	esp_err_t err;

	err = spi_bus_initialize(SPI_settings.host, &SPI_settings.buscfg, SPI_settings.dma_chan);
	ESP_ERROR_CHECK(err);

	//Attach the Accel to the SPI bus
	err = spi_bus_add_device(SPI_settings.host, &SPI_settings.devcfg, &SPI_settings.spi);
	ESP_ERROR_CHECK(err);

	ledDMAbuffer = heap_caps_malloc(LED_DMA_BUFFER_SIZE, MALLOC_CAP_DMA); // Critical to be DMA memory.
}

void led_strip_update()
{
	// static uint16_t LedBitPattern[16] = {
	// 	0x8888, // 1000 1000 1000 1000 -> 0000
	// 	0x8C88, // 1000 1100 1000 1000 -> 0001
	// 	0xC888, // 1100 1000 1000 1000 -> 0010
	// 	0xCC88,
	// 	0x888C,
	// 	0x8C8C,
	// 	0xC88C,
	// 	0xCC8C,
	// 	0x88C8,
	// 	0x8CC8,
	// 	0xC8C8,
	// 	0xCCC8,
	// 	0x88CC,
	// 	0x8CCC,
	// 	0xC8CC,
	// 	0xCCCC // 1100 1100 1100 1100 -> 1111
	// };

	static uint16_t LedBitPattern[16] = {
	    0x8888, // -> 0000
	    0x8E88, // -> 0001
	    0xE888, // -> 0010
	    0xEE88, // -> 0011
	    0x888E, // -> 0100
	    0x8E8E, // -> 0101
	    0xE88E, // -> 0110
	    0xEE8E, // -> 0111
	    0x88E8, // -> 1000
	    0x8EE8, // -> 1001
	    0xE8E8, // -> 1010
	    0xEEE8, // -> 1011
	    0x88EE, // -> 1100
	    0x8EEE, // -> 1101
	    0xE8EE, // -> 1110
	    0xEEEE  // -> 1111
	};

	uint32_t i;
	esp_err_t err;

	memset(ledDMAbuffer, 0, LED_DMA_BUFFER_SIZE);
	int n = 0;
	for (i = 0; i < LED_MAX_NBER_LEDS; i++) {
		uint32_t temp = display_buffer[i];

		//G
		ledDMAbuffer[n++] = LedBitPattern[0x0f & (temp >>12)];
		ledDMAbuffer[n++] = LedBitPattern[0x0f & (temp)>>8];

		//R
		ledDMAbuffer[n++] = LedBitPattern[0x0f & (temp >>4)];
		ledDMAbuffer[n++] = LedBitPattern[0x0f & (temp)];

		//B
		ledDMAbuffer[n++] = LedBitPattern[0x0f & (temp >>20)];
		ledDMAbuffer[n++] = LedBitPattern[0x0f & (temp)>>16];

	}

	spi_transaction_t t;
	memset(&t, 0, sizeof(t));
	t.length = LED_DMA_BUFFER_SIZE * 8; //length is in bits
	t.tx_buffer = ledDMAbuffer;

	err = spi_device_transmit(SPI_settings.spi, &t);
	ESP_ERROR_CHECK(err);
}

# define FLASH_RATE 60
# define FLASH_RATE_MS (1000 / FLASH_RATE)

# define RESOL_H 8
# define RESOL_W 16
static uint32_t screen_indices_map[RESOL_H][RESOL_W];
// static float hight2saturation[RESOL_H];
void init_screen_indices_map() {
	for (int i = 0; i < RESOL_H; i++) {
		for (int j = 0; j < RESOL_W; j++) {
			if (i%2 == 0) screen_indices_map[i][RESOL_W-j-1] = i * RESOL_W + j;
			else screen_indices_map[i][j] = i * RESOL_W + j;
		}
	}

	// for (int i = 0; i < RESOL_H; i++) {
	// 	hight2saturation[i] = i+1 / RESOL_H;
	// }
}


uint32_t hsv_to_rgb(float H, float S, float V) {
	double r = 0, g = 0, b = 0;
	
	double h = H / 360;
	double s = S;
	double v = V;
	
	int i = floor(h * 6);
	double f = h * 6 - i;
	double p = v * (1 - s);
	double q = v * (1 - f * s);
	double t = v * (1 - (1 - f) * s);
	
	switch (i % 6) {
		case 0: r = v, g = t, b = p; break;
		case 1: r = q, g = v, b = p; break;
		case 2: r = p, g = v, b = t; break;
		case 3: r = p, g = q, b = v; break;
		case 4: r = t, g = p, b = v; break;
		case 5: r = v, g = p, b = q; break;
	}

	CRGB color;
	color.r = r * 255;
	color.g = g * 255;
	color.b = b * 255;
	return color.num;
}


void map_test() {
	static int i = 0;
	static int j = 0;
	display_buffer[screen_indices_map[i][j]] = 0x0f0000; // blue
	if (j == RESOL_W - 1) {
		j = 0;
		i++;
	}
	else j++;

	if (i == RESOL_H) i = 0, j = 0;
}


void color_test() {
	static double H = 0.0;
	for (int i = 0; i < RESOL_H; i++) {
		for (int j = 0; j < RESOL_W; j++) {
			display_buffer[screen_indices_map[i][j]] = hsv_to_rgb(H, 0.5, 0.2);
		}
	}
	H += 1;
	if (H >= 360) H = 0;
}

extern float fft_out[FFT_POINTS>>1]; // 0 ~ fft_maxval
extern float fft_maxval;
extern float phase_out[FFT_POINTS>>1]; // 0 ~ 360
SemaphoreHandle_t fft_data_semaphore = NULL;
void write_fft2screen_display_buffer() {
	xSemaphoreTake(fft_data_semaphore, portMAX_DELAY);
	for (int i = 0; i < RESOL_H; i++) {
		for (int j = 0; j < RESOL_W; j++) {
			if ( fft_maxval == 0 || i >= fft_out[j] / fft_maxval * RESOL_H) {
				display_buffer[screen_indices_map[i][j]] = 0x00;
				// ESP_LOGI("fft", "fft_out[%d] = %f, fft_maxval = %f", j, fft_out[j], fft_maxval);
			}
			else {
				display_buffer[screen_indices_map[i][j]] = hsv_to_rgb(phase_out[j], 0.8, 0.2);
			}
		}
	}
	xSemaphoreGive(fft_data_semaphore);
}


void display_main(void *arg) {
	while (1) {
		// color_test();
        write_fft2screen_display_buffer();
		
		led_strip_update();
		vTaskDelay(pdMS_TO_TICKS(FLASH_RATE_MS));
	}
}

static TaskHandle_t timer_xHandle = NULL;
void display_start() {
	fft_data_semaphore = xSemaphoreCreateBinary();
	xSemaphoreGive(fft_data_semaphore);
	init_screen_indices_map();
	initSPIws2812();
	memset(display_buffer, 0, sizeof(display_buffer)); led_strip_update(); // clear screen
    xTaskCreate(display_main, "timer_task", 2048, NULL, 5, &timer_xHandle);
}