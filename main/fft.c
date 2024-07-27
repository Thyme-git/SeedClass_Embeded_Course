# include <stdint.h>
# include <stdlib.h>
# include "fft.h"
# include "esp_log.h"
# include "freertos/FreeRTOS.h"
# include "freertos/task.h"
# include "esp_system.h"
# include "esp_dsp.h"
# include <stdbool.h>
# include <math.h>
# include <esp_task_wdt.h>


static TaskHandle_t fft_xHandle = NULL;
static char data_queue[queue_max_len];
static float imag[FFT_POINTS];
static size_t _frontp = 0;
static size_t _tailp = 0;
static size_t _size = 0;

size_t size() {
    return _size;
}


char front() {
    if (! _size) return -1;
    return data_queue[_frontp];
}


int push(char c) {
    if (_size == queue_max_len) return -1;
    data_queue[_tailp] = c;
    _tailp = (_tailp + 1) % queue_max_len;
    _size++;
    return 0;
}


char pop() {
    // if (! _size) return -1;
    while (!size());
    char c = data_queue[_frontp];
    _frontp = (_frontp + 1) % queue_max_len;
    _size--;
    return c;
}


void clear() {
    _frontp = 0;
    _tailp = 0;
    _size = 0;
}


int enqueue(const char *data, size_t len) {
    if (_size + len / SAMPLE_STEP > queue_max_len) {
        // ESP_LOGW(FFT_TAG, "queue full, clear with size %d, data len %d", size(), len / SAMPLE_STEP);
        clear();
    }
    // static size_t counter = 0;
    for (size_t i = 0; i < len; i += SAMPLE_STEP*2) {
        data_queue[_tailp] = data[i];
        _tailp = (_tailp + 1) % queue_max_len;
        data_queue[_tailp] = data[i+1];
        _tailp = (_tailp + 1) % queue_max_len;
        _size += 2;
    }
    return 0;
}

float fft_out[FFT_POINTS];
float phase_out[FFT_POINTS];
float fft_maxval;
void fft(float real[], int n) {
    // 位逆序置换
    int i, j, k;
    int m;
    float t_real, t_imag, u_real, u_imag;
    int size, halfsize;
    int tablestep;

    for (i = 0; i < n; i++) imag[i] = 0;

    // 位逆序置换
    j = 0;
    for (i = 1; i < n; ++i) {
        m = n >> 1;
        while (j >= m) {
            j -= m;
            m >>= 1;
        }
        j += m;
        if (i < j) {
            t_real = real[i];
            real[i] = real[j];
            real[j] = t_real;
            t_imag = imag[i];
            imag[i] = imag[j];
            imag[j] = t_imag;
        }
    }

    // 蝴蝶操作
    for (size = 2; size <= n; size *= 2) {
        halfsize = size / 2;
        tablestep = n / size;
        for (i = 0; i < n; i += size) {
            for (j = i, k = 0; j < i + halfsize; ++j, k += tablestep) {
                t_real = real[j + halfsize] * cos(-2 * PI * k / n) - imag[j + halfsize] * sin(-2 * PI * k / n);
                t_imag = real[j + halfsize] * sin(-2 * PI * k / n) + imag[j + halfsize] * cos(-2 * PI * k / n);
                u_real = real[j];
                u_imag = imag[j];
                real[j] = u_real + t_real;
                imag[j] = u_imag + t_imag;
                real[j + halfsize] = u_real - t_real;
                imag[j + halfsize] = u_imag - t_imag;
            }
        }
    }

    fft_maxval = 0;
    for (i = 0; i < n ; i++) {
        fft_out[i] = sqrt(real[i] * real[i] + imag[i] * imag[i]);
        if (fft_out[i] > fft_maxval) fft_maxval = fft_out[i];
        phase_out[i] = atan2(imag[i], real[i]) * 180.0 / PI + 180.0;
    }


    // nyw:放一个神奇的EMA在这里，作用只可意会不可言传
    static float fft_prev_maxval;
    fft_maxval = fft_maxval * 0.8 + fft_prev_maxval * 0.2;
    fft_prev_maxval = fft_maxval;
}

void fft_main(void *arg) {
    static float fft_input[FFT_POINTS];
    static size_t cnt = 0;

    for (;;) {
        while (size() >= 2) {
            int16_t sample = (uint16_t)pop() | (uint16_t)pop() << 8;
            fft_input[cnt++] = (float)sample;

            if (cnt == FFT_POINTS) {
                cnt = 0;
                fft(fft_input, FFT_POINTS);
                // dsps_fft2r_fc32(fft_input, FFT_POINTS);
                // dsps_bit_rev_fc32(fft_input, FFT_POINTS);
                // dsps_cplx2reC_fc32(fft_input, FFT_POINTS);
                ESP_LOGI(FFT_TAG, "fft vals: %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f", fft_out[0], fft_out[1], fft_out[2], fft_out[3], fft_out[4], fft_out[5], fft_out[6], fft_out[7], fft_out[8], fft_out[9], fft_out[10], fft_out[11], fft_out[12], fft_out[13], fft_out[14], fft_out[15]);
            }

        }
    }
}

void fft_start() {
     xTaskCreate( fft_main, "fft_main", 8192, NULL, tskIDLE_PRIORITY, &fft_xHandle );
}