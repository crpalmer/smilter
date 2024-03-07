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

static void set_led(int led, color_t *c)
{
    neo->set_led(led, c->r, c->g, c->b);
}

static void set_all(color_t *c)
{
    neo->set_all(c->r, c->g, c->b);
    neo->show();
}

const char *error = "";

static const char * set_lights_cgi_handler(int handler_index, int n_params, char *names[], char *values[])
{
    int n_leds = -1;

    for (int i = 0; i < n_params; i++) {
	if (strcmp(names[i], "values") == 0) {
	    int led;

	    neo->set_all(0, 0, 0);
	    for (led = 0; values[i][led]; led++) {
		int v = values[i][led] - '0';
		if (v >= 0 && v < N_COLORS) set_led(led, colors[v]);
	    }
	    if (n_leds < 0) n_leds = led;
	    else n_leds += led;
	}
    }
    if (n_leds >= 0) {
       neo->show();
       return "/ok.html";
    }
    error = "Missing required parameter 'values'";
    return "/error.html";
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
    set_all(&off);

    cyw43_arch_enable_sta_mode();
    ms_sleep(1000);

    cyw43_wifi_pm(&cyw43_state, cyw43_pm_value(CYW43_NO_POWERSAVE_MODE, 20, 1, 1, 1));
    ms_sleep(1000);

    httpd_init();
    http_set_cgi_handlers(cgi_handlers, N_CGI_HANDLERS);

    while (1) {
	int link_status = cyw43_tcpip_link_status(&cyw43_state, CYW43_ITF_STA);
	if (link_status != CYW43_LINK_UP) {
	    printf("Connecting to Wi-Fi, current status is %d\n", link_status);
	    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000)) {
	        printf("failed to connect.\n");
	    } else {
	        printf("Connected.\n");
	    }
        }
	ms_sleep(1000);
    }
}
