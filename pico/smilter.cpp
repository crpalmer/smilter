#include <stdio.h>
#include <stdlib.h>
#include <pico/bootrom.h>
#include <pico/multicore.h>
#include <tusb.h>
#include "pico/cyw43_arch.h"
#include "lwip/opt.h"
#include "lwip/apps/httpd.h"
#include "gp-input.h"
#include "gp-output.h"
#include "light-sensor.h"
#include "neopixel-pico.h"
#include "pi.h"
#include "util.h"

#include "wifi-credentials.h"

typedef struct {
    int r, g, b;
} color_t;

static color_t off   = {   0,   0,   0 };
static color_t white = { 223, 223, 223 };
static color_t red   = { 200,  60,   0 };
static color_t green = {   0, 255,   0 };
static color_t blue  = {   0,  60, 200 };

static color_t *colors[] = { &off, &white, &red, &green, &blue };
#define N_COLORS (sizeof(colors) / sizeof(colors[0]))

#define PIN 0
#define N_LEDS 50

static NeoPixelPico *neo;
static struct { int fire_high, skulls_high; } flicker = { 55, 55 };
static int purple_pct = 3;
static int red_pct = 12;

static const char * set_lights_cgi_handler(int handler_index, int n_params, char *names[], char *values[])
{
    for (int i = 0; i < n_params; i++) {
	if (strcmp(names[i], "values") == 0) {
	    neo->set_all(0, 0, 0);
	    for (int led = 0; values[i][led]; led++) {
		int v = values[i][led] - '0';
		if (v >= 0 && v < N_COLORS) neo->set_led(led, colors[v]->r, colors[v]->g, colors[v]->b);
	    }
	    neo->show();
	}
    }
    return "/ok.html";
}

static const tCGI cgi_handlers[] = {
    { "/set-lights", set_lights_cgi_handler },
};

#define N_CGI_HANDLERS (sizeof(cgi_handlers) / sizeof(cgi_handlers[0]))

int main(int argc, char **argv)
{
    int color = 0;

    pi_init();

    if (cyw43_arch_init()) {
        printf("failed to initialise\n");
        return 1;
    }

    neo = new NeoPixelPico(PIN, true);
    neo->set_n_leds(N_LEDS);
    neo->set_mode(neopixel_mode_RGB);
    neo->set_brightness(0.25);
    neo->set_all(0, 0, 0);
    neo->show();

    cyw43_arch_enable_sta_mode();

    printf("Connecting to Wi-Fi...\n");
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000)) {
        printf("failed to connect.\n");
        neo->set_all(255, 0, 0);
	neo->show();
        while (1) ms_sleep(1000);
    } else {
        printf("Connected.\n");
    }

    neo->set_all(0, 255, 0);
    neo->show();

    httpd_init();
    http_set_cgi_handlers(cgi_handlers, N_CGI_HANDLERS);

    while (1) {
        ms_sleep(30000);
    }
}
