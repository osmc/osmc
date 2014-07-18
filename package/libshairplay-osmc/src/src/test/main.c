#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <ao/ao.h>

#include "dnssd.h"
#include "airplay.h"
#include "raop.h"
#include "rsakey.h"
#include "utils.h"

#define CHALLENGE "LfBLs2pkGT4yYAJrxj2K9Q=="
static unsigned char ipaddr[] = { 192, 168, 1, 10 };
static unsigned char hwaddr[] = { 0x00, 0x5A, 0xDB, 0xE4, 0xE6, 0xFD };

#define AESENC "B69yRGHoriZZeJNKghotJi6Pt7dEPEwKSoieM6dk8YE1D23/jyOsg0ZoJZmodHB4lR9Q9CmeoqgU0GX1KkYNafCrNUlXxJAZuMTISGmyeDegnH73ul1NQwIjfphGgwrx7dp7J+p4lyQi+Yt/daQyjE//Od0viD37GQtI9B7GukaiWrMl94wJmSLUL94VpKUxnD9E7T/LesX7bEMfUgSQUpIE+T2anp6eRqE/5R3kNYdEH9JcCEFu5DLqbbvMqgc0ewr81BNeVG5ck1iI2eF+OJVm9g082ZXqGAPFGwmcYiiLfjrQY5hnEUi7IeWqgX5Xd82DyW9BeDzT5MXVyI/GwQ=="

static void
test_rsa(const char *pemstr)
{
	char buffer[2048];
	int ret;
	rsakey_t *rsakey;

	rsakey = rsakey_init_pem(pemstr);
	if (!rsakey) {
		printf("Initializing RSA failed\n");
		return;
	}

	rsakey_sign(rsakey, buffer, sizeof(buffer), CHALLENGE,
	            ipaddr, sizeof(ipaddr), hwaddr, sizeof(hwaddr));
	printf("Signature:\n%s\n", buffer);

	ret = rsakey_decrypt(rsakey, (unsigned char *)buffer, sizeof(buffer), AESENC);
	printf("Decrypted length: %d\n", ret);

	rsakey_destroy(rsakey);
}

static void
photo_cb(char *data, int datalen)
{
	char template[512];
	int written = 0;
	int fd, ret;

	printf("Got photo with data length: %d\n", datalen);

	memset(template, 0, sizeof(template));
	strcpy(template, "/tmp/tmpXXXXXX.JPG");
	fd = mkstemps(template, 4);

	while (written < datalen) {
		ret = write(fd, data+written, datalen-written);
		if (ret <= 0) break;
		written += ret;
	}
	if (written == datalen) {
		printf("Wrote to file %s\n", template);
	}
	close(fd);
}

static void
play_cb()
{
}

static void
stop_cb()
{
}

static void
rate_set_cb()
{
}

static void
scrub_get_cb()
{
}

static void
scrub_set_cb()
{
}

static void
playback_info_cb()
{
}

static void *
audio_init(void *opaque, int bits, int channels, int samplerate)
{
	int driver;
	ao_sample_format format;
	ao_option *ao_opts = NULL;
	ao_device *device;

	printf("Opening audio device\n");
	driver = ao_driver_id("pulse");

	memset(&format, 0, sizeof(format));
	format.bits = bits;
	format.channels = channels;
	format.rate = samplerate;
	format.byte_format = AO_FMT_LITTLE;

	ao_append_option(&ao_opts, "id", "0");
	device = ao_open_live(driver, &format, ao_opts);
	if (!device) {
		fprintf(stderr, "Error opening audio device.\n");
	} else {
		printf("Opening device successful\n");
	}
	return device;
}

static void
audio_process(void *ptr, const void *buffer, int buflen)
{
	ao_device *device = ptr;

	assert(device);

	printf("Got %d bytes of audio\n", buflen);
	ao_play(device, (char *)buffer, buflen);
}

static void
audio_flush(void *ptr)
{
}

static void
audio_destroy(void *ptr)
{
	ao_device *device = ptr;

	printf("Closing audio device\n");
	ao_close(device);
}

int
main(int argc, char *argv[])
{
        const char *name = "AppleTV";
        unsigned short raop_port = 5000;
        unsigned short airplay_port = 7000;
        const char hwaddr[] = { 0x48, 0x5d, 0x60, 0x7c, 0xee, 0x22 };
	char *pemstr;

	dnssd_t *dnssd;
	airplay_t *airplay;
	airplay_callbacks_t ap_cbs;
	raop_t *raop;
	raop_callbacks_t raop_cbs;

	if (utils_read_file(&pemstr, "airport.key") < 0) {
		return -1;
	}
	if (argc > 1) {
		test_rsa(pemstr);
	}

	ao_initialize();

	dnssd = dnssd_init(hwaddr, sizeof(hwaddr), NULL);
	dnssd_register_raop(dnssd, name, raop_port);
	dnssd_register_airplay(dnssd, name, airplay_port);

	ap_cbs.photo_cb = &photo_cb;
	ap_cbs.play_cb = &play_cb;
	ap_cbs.stop_cb = &stop_cb;
	ap_cbs.rate_set_cb = &rate_set_cb;
	ap_cbs.scrub_get_cb = &scrub_get_cb;
	ap_cbs.scrub_set_cb = &scrub_set_cb;
	ap_cbs.playback_info_cb = &playback_info_cb;

	raop_cbs.audio_init = audio_init;
	raop_cbs.audio_process = audio_process;
	raop_cbs.audio_flush = audio_flush;
	raop_cbs.audio_destroy = audio_destroy;

	airplay = airplay_init(&ap_cbs, hwaddr, sizeof(hwaddr));
	airplay_start(airplay, airplay_port);

	raop = raop_init(&raop_cbs, pemstr, hwaddr, sizeof(hwaddr));
	raop_start(raop, &raop_port);

	sleep(100);

	raop_stop(raop);
	raop_destroy(raop);

	airplay_stop(airplay);
	airplay_destroy(airplay);

	dnssd_unregister_airplay(dnssd);
	dnssd_unregister_raop(dnssd);
	dnssd_destroy(dnssd);

	ao_shutdown();

	return 0;
}

