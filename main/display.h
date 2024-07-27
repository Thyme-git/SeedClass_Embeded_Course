# ifndef DISPLAY_H
# define DISPLAY_H

# include <string.h>
# include "driver/spi_master.h"
# include "driver/gpio.h"
# include "esp_heap_caps.h"
# include "esp_mac.h"

typedef struct CRGB {
	union {
		struct {
            union {
                uint8_t r;
                uint8_t red;
            };
            union {
                uint8_t g;
                uint8_t green;
            };
            union {
                uint8_t b;
                uint8_t blue;
            };
        };
		uint8_t raw[3];
		uint32_t num;
	};
}CRGB;

void initSPIws2812();
void fillCol(uint32_t col);
void fillBuffer(uint32_t* bufLed, int Count);
void led_strip_update();
void display_start();

# endif