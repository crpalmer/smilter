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
    const char *name;
} color_t;

static color_t colors[] = {
    {   0,   0,   0, "off" },
    {   0,  60, 200, "hold" },
    { 230, 230,  10, "foot only" },
    {   0, 200,  60, "start hold" },
    { 200,  60,   0, "final hold" },
};

#define off (colors[0])

#define N_COLORS (sizeof(colors) / sizeof(colors[0]))

#define PIN 0
#define N_LEDS 100

static NeoPixelPico *neo;
static struct { int fire_high, skulls_high; } flicker = { 55, 55 };
static int purple_pct = 3;
static int red_pct = 12;
static char state[N_LEDS+1];

const char *error = "";
static int n_leds = -1;

static void set_led(int led, color_t *c)
{
    neo->set_led(led, c->r, c->g, c->b);
}

static void set_all(color_t *c)
{
    neo->set_all(c->r, c->g, c->b);
    neo->show();
}

static const char *set_brightness_cgi_handler(int handler_index, int n_params, char *names[], char *values[])
{
    for (int i = 0; i < n_params; i++) {
	if (strcmp(names[i], "value") == 0) {
	    neo->set_brightness(atof(values[i]));
            neo->show();
            return "/ok.shtml";
	}
    }
    error = "Missing required parameter 'value'";
    return "/error.shtml";
}

static const char * set_lights_cgi_handler(int handler_index, int n_params, char *names[], char *values[])
{
    n_leds = -1;

    for (int i = 0; i < n_params; i++) {
	if (strcmp(names[i], "values") == 0) {
	    int led;

	    neo->set_all(0, 0, 0);
	    for (led = 0; values[i][led]; led++) {
		int v = values[i][led] - '0';
		if (v >= 0 && v < N_COLORS) set_led(led, &colors[v]);
	    }
	    snprintf(state, N_LEDS, "%s", values[i]);
	    n_leds = led;
	} else if (strcmp(names[i], "random") == 0) {
	    for (int i = 0; i < N_LEDS; i++) {
		int c = random_number_in_range(0, N_COLORS-1);
		state[i] = c + '0';
		set_led(i, &colors[c]);
	    }
	    n_leds = N_LEDS;
	}
    }
    if (n_leds >= 0) {
       neo->show();
       return "/ok.shtml";
    }
    error = "Missing required parameter 'values'";
    return "/error.shtml";
}

static const char * toggle_cgi_handler(int handler_index, int n_params, char *names[], char *values[])
{
    int id = -1;

    for (int i = 0; i < n_params; i++) {
	if (strcmp(names[i], "state") == 0) strcpy(state, values[i]);
	else if (strcmp(names[i], "id") == 0) id = atoi(values[i]);
    }

    if (id < 0) {
	error = "Missing id parameter";
	return "/error.shtml";
    }

    int old = state[id] - '0';
    int next = (old + 1) % N_COLORS;
    state[id] = next + '0';

    set_led(id, &colors[next]);
    neo->show();

    return "/ui.shtml";
}

static const tCGI cgi_handlers[] = {
    { "/set-brightness", set_brightness_cgi_handler },
    { "/set-lights", set_lights_cgi_handler },
    { "/toggle", toggle_cgi_handler },
};

#define N_CGI_HANDLERS (sizeof(cgi_handlers) / sizeof(cgi_handlers[0]))

const char *ssi_tags[] = { "error", "n_leds",
  /* all the lights */
  "ID0", "ID1", "ID2", "ID3", "ID4", "ID5", "ID6", "ID7", "ID8", "ID9",
  "ID10", "ID11", "ID12", "ID13", "ID14", "ID15", "ID16", "ID17", "ID18", "ID19",
  "ID20", "ID21", "ID22", "ID23", "ID24", "ID25", "ID26", "ID27", "ID28", "ID29",
  "ID20", "ID21", "ID22", "ID23", "ID24", "ID25", "ID26", "ID27", "ID28", "ID29",
  "ID30", "ID31", "ID32", "ID33", "ID34", "ID35", "ID36", "ID37", "ID38", "ID39",
  "ID40", "ID41", "ID42", "ID43", "ID44", "ID45", "ID46", "ID47", "ID48", "ID49",
 };
#define N_SSI_TAGS (sizeof(ssi_tags) / sizeof(ssi_tags[0]))

static u16_t ssi_handler(int tag, char *output, int output_len) {
    switch(tag) {
    case 0: return snprintf(output, output_len, "%s", error);
    case 1: return snprintf(output, output_len, "%d", n_leds);
    }
    if (strncmp(ssi_tags[tag], "ID", 2) == 0) {
printf("%s\n", ssi_tags[tag]);
	int id = atoi(&ssi_tags[tag][2]);
	return snprintf(output, output_len, "<a href=\"toggle?state=%s&id=%d\">%s</a>", state, id, colors[state[id]-'0'].name);
    }
    return 0;
}

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
    neo->set_mode(neopixel_mode_GRB);
    //neo->set_mode(neopixel_mode_RGB);
    //neo->set_brightness(0.25);
    set_all(&off);

    memset(state, '0', N_LEDS);

    cyw43_arch_enable_sta_mode();
    ms_sleep(1000);

    cyw43_wifi_pm(&cyw43_state, cyw43_pm_value(CYW43_NO_POWERSAVE_MODE, 20, 1, 1, 1));
    ms_sleep(1000);

    httpd_init();
    http_set_cgi_handlers(cgi_handlers, N_CGI_HANDLERS);
    http_set_ssi_handler(ssi_handler, ssi_tags, N_SSI_TAGS);

    while (1) {
	int link_status = cyw43_tcpip_link_status(&cyw43_state, CYW43_ITF_STA);
	if (link_status != CYW43_LINK_UP) {
	    printf("Connecting to Wi-Fi, current status is %d\n", link_status);
	    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000)) {
	        printf("failed to connect.\n");
	    } else {
	        printf("Connected.\n");
	    }
        } else {
	    while (1) ms_sleep(100*1000);
        }
	ms_sleep(30*1000);
    }
}
