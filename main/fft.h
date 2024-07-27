# ifndef FFT_H
# define FFT_H

#include <stdint.h>
#include <stdlib.h>

# define FFT_TAG "fft"

// # define SAMPLE_RATE 44100
// # define FLASH_RATE 12
// # define SAMPLE_STEP ((int)(SAMPLE_RATE / (FLASH_RATE * FFT_POINTS)))
# define SAMPLE_STEP 1
# define FFT_POINTS 16

// # define item_size_upto (240 * 6) // defined in  bt_app_core.c:127
// # define queue_max_len (item_size_upto * 8)
# define queue_max_len (1024)
# define PI 3.14159265358979323846

size_t size();
char front();
int push(char c);
char pop();
int enqueue(const char *data, size_t len);
void fft_start();

# endif 