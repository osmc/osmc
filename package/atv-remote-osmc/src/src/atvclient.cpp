/*
 * atvclient: AppleTV Remote XBMC client
 *
 * Copyright (C) 2009 Christoph Cantillon <christoph.cantillon@roots.be> 
 * Copyright (C) 2008 Peter Korsgaard <jacmet@sunsite.dk>
 *
 * This file is licensed under the terms of the GNU General Public License
 * version 2.  This program is licensed "as is" without any warranty of any
 * kind, whether express or implied.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <usb.h>
#include <sys/time.h>

#include "xbmcclient.h"

#define VENDOR_APPLE		0x05ac
#define PRODUCT_APPLE_IR_0    0x8240
#define PRODUCT_APPLE_IR_1    0x8241
#define PRODUCT_APPLE_IR_2    0x8242

#define IS_APPLE_REMOTE(dev) ((dev->descriptor.idVendor == VENDOR_APPLE) && \
                             ((dev->descriptor.idProduct == PRODUCT_APPLE_IR_0) || \
                              (dev->descriptor.idProduct == PRODUCT_APPLE_IR_1) || \
                              (dev->descriptor.idProduct == PRODUCT_APPLE_IR_2)))

#define LEDMODE_OFF		0
#define LEDMODE_AMBER		1
#define LEDMODE_AMBER_BLINK	2
#define LEDMODE_WHITE		3
#define LEDMODE_WHITE_BLINK	4
#define LEDMODE_BOTH		5
#define LEDMODE_MAX		5

#define BUTTON_TIMEOUT 150
#define HOLD_TIMEOUT   500

#define EVENT_UP 1
#define EVENT_DOWN 2
#define EVENT_LEFT 3
#define EVENT_RIGHT 4
#define EVENT_PLAY 5
#define EVENT_MENU 6
#define EVENT_HOLD_PLAY 7
#define EVENT_HOLD_MENU 8

#define EVENT_RELEASE 0x80

#define EVENT_EXTRA_PLAY          70
#define EVENT_EXTRA_PAUSE         71
#define EVENT_EXTRA_STOP          72
#define EVENT_EXTRA_REPLAY        73
#define EVENT_EXTRA_SKIP          74
#define EVENT_EXTRA_REWIND        75      
#define EVENT_EXTRA_FORWARD       76
#define EVENT_EXTRA_PAGEUP        77
#define EVENT_EXTRA_PAGEDOWN      78

#define EVENT_HARMONY_UP           1
#define EVENT_HARMONY_DOWN         2
#define EVENT_HARMONY_LEFT         3
#define EVENT_HARMONY_RIGHT        4
#define EVENT_HARMONY_OK           5
#define EVENT_HARMONY_MENU         6
#define EVENT_HARMONY_HOLD_OK      7
#define EVENT_HARMONY_HOLD_MENU    8
#define EVENT_HARMONY_STOP        15
#define EVENT_HARMONY_PLAY        16
#define EVENT_HARMONY_REPLAY      91
#define EVENT_HARMONY_SKIP        92
#define EVENT_HARMONY_RECORD      52
#define EVENT_HARMONY_REWIND      41
#define EVENT_HARMONY_FORWARD     42
#define EVENT_HARMONY_PAUSE       26
#define EVENT_HARMONY_PREV        32
#define EVENT_HARMONY_GUIDE       65
#define EVENT_HARMONY_INFO        31
#define EVENT_HARMONY_EXIT        51
#define EVENT_HARMONY_VOLUP       21
#define EVENT_HARMONY_VOLDOWN     22
#define EVENT_HARMONY_1           11
#define EVENT_HARMONY_2           12
#define EVENT_HARMONY_3           13
#define EVENT_HARMONY_4           14
#define EVENT_HARMONY_5           23
#define EVENT_HARMONY_6           24
#define EVENT_HARMONY_7           33
#define EVENT_HARMONY_8           34
#define EVENT_HARMONY_9           43
#define EVENT_HARMONY_0           44
#define EVENT_HARMONY_CLEAR       45
#define EVENT_HARMONY_ENTER       36
#define EVENT_HARMONY_MUTE        25
#define EVENT_HARMONY_ASPECT      61
#define EVENT_HARMONY_F1          53
#define EVENT_HARMONY_F3          55
#define EVENT_HARMONY_F2          54
#define EVENT_HARMONY_F4          56
#define EVENT_HARMONY_F5          93
#define EVENT_HARMONY_F6          94
#define EVENT_HARMONY_F7          95
#define EVENT_HARMONY_F8          96
#define EVENT_HARMONY_F9          73
#define EVENT_HARMONY_F10         74
#define EVENT_HARMONY_F11         75
#define EVENT_HARMONY_F12         76
#define EVENT_HARMONY_F13         63
#define EVENT_HARMONY_F14         64
#define EVENT_HARMONY_CHANUP      71
#define EVENT_HARMONY_CHANDOWN    72
#define EVENT_HARMONY_LRGDOWN     82
#define EVENT_HARMONY_LRGUP       81
#define EVENT_HARMONY_PWRTOGGLE   66
#define EVENT_HARMONY_QUEUE       62
#define EVENT_HARMONY_SLEEP       46
#define EVENT_HARMONY_RED         83
#define EVENT_HARMONY_GREEN       84
#define EVENT_HARMONY_YELLOW      85
#define EVENT_HARMONY_BLUE        86

bool multi_mode = 0;
bool debug = 0;
int idle_mode = LEDMODE_WHITE;
int button_mode = LEDMODE_OFF;
int special_mode = LEDMODE_AMBER;
int hold_mode = LEDMODE_AMBER;

/* from libusb usbi.h */
struct usb_dev_handle {
	int fd;

	struct usb_bus *bus;
	struct usb_device *device;

	int config;
	int interface;
	int altsetting;

	/* Added by RMT so implementations can store other per-open-device data */
	void *impl_info;
};

struct ir_command {
  unsigned char flags;
  unsigned char unused;
  unsigned char event;
  unsigned char address;
  unsigned char eventId;
};

static CAddress my_addr;
static int sockfd;

static CPacketBUTTON* button_map[0xff];
static CPacketBUTTON* multi_map[0xff];

static CPacketNOTIFICATION remote_paired("Remote paired", "You can now only control XBMC using the control you're holding. To unpair, hold down menu and rewind for 6 seconds.", NULL, NULL);
static CPacketNOTIFICATION remote_unpaired("Remote unpaired", "You can now control XBMC with any Apple remote.", NULL, NULL);
static CPacketNOTIFICATION remote_pair_failed("Remote already paired", "This AppleTV was paired to another remote. To unpair, hold down menu and rewind for 6 seconds.", NULL, NULL);

const char* remoteIdFile = "/etc/atvremoteid";
static int pairedRemoteId = 0; 

/* figure out kernel name corresponding to usb device */
static int usb_make_kernel_name(usb_dev_handle *dev, int interface,
				char *name, int namelen)
{
	DIR *dir;
	struct dirent *ent;
	char busstr[10], devstr[10];
	int buslen, devlen;

	/* kernel names are in the form of:
	   <busnum>-<devpath>:<config>.<interface>
	   We have everything besides devpath, but there doesn't seem to be
	   an easy of going from devnum to devpath, so we scan sysfs */

	buslen = sprintf(busstr, "%d-", atoi(dev->bus->dirname));
	devlen = sprintf(devstr, "%d\n", dev->device->devnum);

	/* scan /sys/bus/usb/devices/<busnum>-* and compare devnum */
	if (chdir("/sys/bus/usb/devices"))
		return -1;

	dir = opendir(".");
	if (!dir)
		return -1;

	while ((ent = readdir(dir))) {
		char buf[PATH_MAX];
		int fd;

		/* only check devices on busnum bus */
		if (strncmp(busstr, ent->d_name, buslen))
			continue;

		sprintf(buf, "%s/devnum", ent->d_name);
		fd = open(buf, O_RDONLY);
		if (fd == -1)
			continue;

		if ((read(fd, buf, sizeof(buf)) == devlen)
		    && !strncmp(buf, devstr, devlen)) {

			close(fd);

			if (snprintf(name, namelen, "%s:%d.%d", ent->d_name,
				     1, interface) >= namelen)
				goto out;

			/* closedir could invalidate ent, so do it after the
			   snprintf */
			closedir(dir);
			return 0;
		}

		close(fd);
	}

 out:
	closedir(dir);
	return -1;
}

/* (re)attach usb device to kernel driver (need hotplug support in kernel) */
static int usb_attach_kernel_driver_np(usb_dev_handle *dev, int interface,
				       const char *driver)
{
	char name[PATH_MAX], buf[PATH_MAX];

	if (!dev || !driver || interface < 0)
		return -1;

	if (!usb_make_kernel_name(dev, interface, name, sizeof(name))) {
		int fd, ret, len;

		/* (re)bind driver to device */
		sprintf(buf, "/sys/bus/usb/drivers/%s/bind", driver);
		len = strlen(name);

		fd = open(buf, O_WRONLY);
		if (fd == -1)
			return -1;

		ret = write(fd, name, len);
		close(fd);

		if (ret != len)
			return -1;
		else
			return 0;
	}

	return -1;
}

static usb_dev_handle *find_ir(void)
{
	struct usb_bus *bus;
	struct usb_device *dev;

	for (bus = usb_busses; bus; bus = bus->next) {
		for (dev = bus->devices; dev; dev = dev->next)
			if (dev->descriptor.idVendor == VENDOR_APPLE)
			    if(IS_APPLE_REMOTE(dev))
 				return usb_open(dev);
	}

	return NULL;
}

static usb_dev_handle *get_ir(void)
{
	static usb_dev_handle *ir = NULL;

	if (!ir) {
		usb_init();
		usb_find_busses();
		usb_find_devices();

		ir = find_ir();
		if (!ir) {
			fprintf(stderr, "IR receiver not found, quitting\n");
			exit(1);
		}

		/* interface is normally handled by hiddev */
		usb_detach_kernel_driver_np(ir, 0);
		if (usb_claim_interface(ir, 0)) {
			fprintf(stderr, "error claiming interface, are you root?\n");
			exit(2);
		}
		//usb_reset(ir);
		//usb_set_configuration(ir, 0);
	}

	return ir;
}

static void reattach(void)
{
	usb_dev_handle *ir;

	ir = get_ir();
	if (ir) {
		usb_release_interface(ir, 0);
		/* attach fails if we still have the file
		   descriptor open */
		usb_close(ir);
		usb_attach_kernel_driver_np(ir, 0, "usbhid");
	}
}

static int set_report(unsigned char* data, int len)
{
	unsigned char *type = data;
	int val;

	val = 0x300 | *type;

	return (usb_control_msg(get_ir(), USB_ENDPOINT_OUT | USB_TYPE_CLASS
				| USB_RECIP_INTERFACE, 9, val, 0,
				(char*) data, len, 1000) != len);
}

static void set_fan(int full)
{
	unsigned char buf[2];

	buf[0] = 0xf; buf[1] = full ? 1 : 0;

	set_report(buf, sizeof(buf));
}

static void set_led(int mode)
{
	unsigned char buf[5];

	memset(buf, 0, sizeof(buf));
	buf[0] = 0xd; buf[1] = mode;

	switch (mode) {
	case LEDMODE_OFF:
		set_report(buf, sizeof(buf));
		buf[1] = 3;
		set_report(buf, 3);
		buf[1] = 4;
		set_report(buf, 3);
		break;

	case LEDMODE_AMBER:
		set_report(buf, sizeof(buf));
		buf[1] = 3; buf[2] = 1;
		set_report(buf, 3);
		buf[1] = 4; buf[2] = 0;
		set_report(buf, 3);
		break;

	case LEDMODE_AMBER_BLINK:
		set_report(buf, sizeof(buf));
		buf[1] = 3;
		set_report(buf, 3);
		buf[1] = 4;
		set_report(buf, 3);
		buf[1] = 3; buf[2] = 2;
		set_report(buf, 3);
		break;

	case LEDMODE_WHITE:
		set_report(buf, sizeof(buf));
		set_report(buf, 3);
		buf[1] = 4; buf[2] = 1;
		set_report(buf, 3);
		break;

	case LEDMODE_WHITE_BLINK:
		set_report(buf, sizeof(buf));
		buf[1] = 3;
		set_report(buf, 3);
		buf[1] = 4;
		set_report(buf, 3);
		buf[1] = 4; buf[2] = 2;
		set_report(buf, 3);
		break;

	case LEDMODE_BOTH:
		buf[1] = 7;
		set_report(buf, sizeof(buf));
		buf[1] = 6; buf[2] = 1;
		set_report(buf, 3);
		break;
	}
}

static void set_led_brightness(int high)
{
	unsigned char buf[5];

	memset(buf, 0, sizeof(buf));
	buf[0] = 0xd;

	if (high) {
		buf[1] = 6;
		set_report(buf, sizeof(buf));
		buf[1] = 5; buf[2] = 1;
		set_report(buf, 3);
	} else {
		buf[1] = 5;
		set_report(buf, sizeof(buf));
		set_report(buf, 3);
	}
}

void dumphex(unsigned char* buf, int size) {
  int i;
  for(i=0; i < size; i++) {
    printf("%02x ", buf[i]);
  }
  printf("\n");
}

unsigned long millis() {
  static struct timeval time;
  gettimeofday(&time, NULL);
  return (time.tv_sec*1000+time.tv_usec/1000);
}

bool is_multi_candidate(struct ir_command command) {
  switch(command.address) {
    case 0x96: case 0x97: case 0x98: case 0x99: case 0x9a: case 0x9b: case 0x9d: case 0x9e: case 0x9f: case 0xa0:
          if(debug) printf("Multi-remote candidate\n"); return multi_mode; break;
        default:
          return false;
  }
}

void send_multi(int button) {
  printf("Sending multi-remote button %02x\n", multi_map[button]->m_ButtonCode);
  multi_map[button] -> Send(sockfd, my_addr);
}

void send_button(int button) {
  printf("Sending button %02x\n", button_map[button]->m_ButtonCode);
  switch(button) {
    case EVENT_UP: printf("Up\n"); break;
    case EVENT_DOWN: printf("Down\n"); break;
    case EVENT_LEFT: printf("Left\n"); break;
    case EVENT_RIGHT: printf("Right\n"); break;
    case EVENT_MENU: printf("Menu\n"); break;
    case EVENT_HOLD_MENU: printf("Menu hold\n"); break;
    case EVENT_PLAY: printf("Play/pause\n"); break;
    case EVENT_HOLD_PLAY: printf("Play/pause hold\n"); break;
  }
  
  button_map[button] -> Send(sockfd, my_addr);
}

void handle_button(struct ir_command command) {
  static unsigned short previousButton;
  static unsigned char holdButtonSent;
  static long buttonStart;

  unsigned short eventId;
  if(command.flags == 0x26) {
    eventId = previousButton;
  } else {
    eventId = is_multi_candidate(command) ? command.address<<8 | command.eventId : command.eventId;
  }

  if(debug) printf("Event id: %04x\n", eventId);

  if(previousButton != eventId && previousButton != 0 && !holdButtonSent) {
    switch(previousButton) {
      case 0x03: case 0x02: send_button(EVENT_MENU); break;
      case 0x05: case 0x04: send_button(EVENT_PLAY); break;
      case 0x9602: send_multi(EVENT_HARMONY_MENU); break;
      case 0x9604: send_multi(EVENT_HARMONY_OK); break;
    }
  }
  
  if(previousButton != eventId) {
    buttonStart = millis();
  }

  switch(eventId) {
    case 0x0a:
    case 0x0b:
      if(eventId != previousButton) send_button(EVENT_UP); break;
    case 0x0c:
    case 0x0d:
      if(eventId != previousButton) send_button(EVENT_DOWN); break;
    case 0x09:
    case 0x08:
      if(eventId != previousButton) send_button(EVENT_LEFT); break;
    case 0x06:
    case 0x07:
      if(eventId != previousButton) send_button(EVENT_RIGHT); break;
      
    // extra buttons mapped by Harmony remotes
    case 0x16: if(eventId != previousButton) send_button( EVENT_EXTRA_PLAY         ); break;
    case 0x32: if(eventId != previousButton) send_button( EVENT_EXTRA_PAUSE        ); break;
    case 0x31: if(eventId != previousButton) send_button( EVENT_EXTRA_STOP         ); break;
    case 0x25: if(eventId != previousButton) send_button( EVENT_EXTRA_REPLAY       ); break;
    case 0x23: if(eventId != previousButton) send_button( EVENT_EXTRA_SKIP         ); break;
    case 0x1c: if(eventId != previousButton) send_button( EVENT_EXTRA_REWIND       ); break;
    case 0x1a: if(eventId != previousButton) send_button( EVENT_EXTRA_FORWARD      ); break;
    case 0x2f: if(eventId != previousButton) send_button( EVENT_EXTRA_PAGEUP       ); break;
    case 0x26: if(eventId != previousButton) send_button( EVENT_EXTRA_PAGEDOWN     ); break;
      
    // multi-remote buttons
    // extra buttons mapped by Harmony remotes - duplicates of ATV, just in case
    case 0x960b: if(eventId != previousButton) send_multi( EVENT_HARMONY_UP           ); break;
    case 0x960d: if(eventId != previousButton) send_multi( EVENT_HARMONY_DOWN         ); break;
    case 0x9608: if(eventId != previousButton) send_multi( EVENT_HARMONY_LEFT         ); break;
    case 0x9607: if(eventId != previousButton) send_multi( EVENT_HARMONY_RIGHT        ); break;
    case 0x9703: if(eventId != previousButton) send_multi( EVENT_HARMONY_PLAY         ); break;
    case 0x9705: if(eventId != previousButton) send_multi( EVENT_HARMONY_STOP         ); break;
    case 0x9803: if(eventId != previousButton) send_multi( EVENT_HARMONY_PAUSE        ); break;
    case 0xa00b: if(eventId != previousButton) send_multi( EVENT_HARMONY_REPLAY       ); break;
    case 0xa00d: if(eventId != previousButton) send_multi( EVENT_HARMONY_SKIP         ); break;
    case 0x9a0b: if(eventId != previousButton) send_multi( EVENT_HARMONY_REWIND       ); break;
    case 0x9a0d: if(eventId != previousButton) send_multi( EVENT_HARMONY_FORWARD      ); break;
    case 0x9b0c: if(eventId != previousButton) send_multi( EVENT_HARMONY_RECORD       ); break;
    case 0x990d: if(eventId != previousButton) send_multi( EVENT_HARMONY_PREV         ); break;
    case 0x9d05: if(eventId != previousButton) send_multi( EVENT_HARMONY_GUIDE        ); break;
    case 0x990b: if(eventId != previousButton) send_multi( EVENT_HARMONY_INFO         ); break;
    case 0x9b0a: if(eventId != previousButton) send_multi( EVENT_HARMONY_EXIT         ); break;
    case 0x980a: if(eventId != previousButton) send_multi( EVENT_HARMONY_VOLUP        ); break;
    case 0x980c: if(eventId != previousButton) send_multi( EVENT_HARMONY_VOLDOWN      ); break;
    case 0x970a: if(eventId != previousButton) send_multi( EVENT_HARMONY_1            ); break;
    case 0x970c: if(eventId != previousButton) send_multi( EVENT_HARMONY_2            ); break;
    case 0x9709: if(eventId != previousButton) send_multi( EVENT_HARMONY_3            ); break;
    case 0x9706: if(eventId != previousButton) send_multi( EVENT_HARMONY_4            ); break;
    case 0x9809: if(eventId != previousButton) send_multi( EVENT_HARMONY_5            ); break;
    case 0x9806: if(eventId != previousButton) send_multi( EVENT_HARMONY_6            ); break;
    case 0x9908: if(eventId != previousButton) send_multi( EVENT_HARMONY_7            ); break;
    case 0x9907: if(eventId != previousButton) send_multi( EVENT_HARMONY_8            ); break;
    case 0x9a08: if(eventId != previousButton) send_multi( EVENT_HARMONY_9            ); break;
    case 0x9a07: if(eventId != previousButton) send_multi( EVENT_HARMONY_0            ); break;
    case 0x9a04: if(eventId != previousButton) send_multi( EVENT_HARMONY_CLEAR        ); break;
    case 0x9902: if(eventId != previousButton) send_multi( EVENT_HARMONY_ENTER        ); break;
    case 0x9805: if(eventId != previousButton) send_multi( EVENT_HARMONY_MUTE         ); break;
    case 0x9d0a: if(eventId != previousButton) send_multi( EVENT_HARMONY_ASPECT       ); break;
    case 0x9b09: if(eventId != previousButton) send_multi( EVENT_HARMONY_F1           ); break;
    case 0x9b06: if(eventId != previousButton) send_multi( EVENT_HARMONY_F2           ); break;
    case 0x9b05: if(eventId != previousButton) send_multi( EVENT_HARMONY_F3           ); break;
    case 0x9b03: if(eventId != previousButton) send_multi( EVENT_HARMONY_F4           ); break;
    case 0xa008: if(eventId != previousButton) send_multi( EVENT_HARMONY_F5           ); break;
    case 0xa007: if(eventId != previousButton) send_multi( EVENT_HARMONY_F6           ); break;
    case 0xa004: if(eventId != previousButton) send_multi( EVENT_HARMONY_F7           ); break;
    case 0xa002: if(eventId != previousButton) send_multi( EVENT_HARMONY_F8           ); break;
    case 0x9e09: if(eventId != previousButton) send_multi( EVENT_HARMONY_F9           ); break;
    case 0x9e06: if(eventId != previousButton) send_multi( EVENT_HARMONY_F10          ); break;
    case 0x9e05: if(eventId != previousButton) send_multi( EVENT_HARMONY_F11          ); break;
    case 0x9e03: if(eventId != previousButton) send_multi( EVENT_HARMONY_F12          ); break;
    case 0x9d09: if(eventId != previousButton) send_multi( EVENT_HARMONY_F13          ); break;
    case 0x9d06: if(eventId != previousButton) send_multi( EVENT_HARMONY_F14          ); break;
    case 0x9e0c: if(eventId != previousButton) send_multi( EVENT_HARMONY_CHANDOWN     ); break;
    case 0x9e0a: if(eventId != previousButton) send_multi( EVENT_HARMONY_CHANUP       ); break;
    case 0x9f0d: if(eventId != previousButton) send_multi( EVENT_HARMONY_LRGDOWN      ); break;
    case 0x9f0b: if(eventId != previousButton) send_multi( EVENT_HARMONY_LRGUP        ); break;
    case 0x9d03: if(eventId != previousButton) send_multi( EVENT_HARMONY_PWRTOGGLE    ); break;
    case 0x9d0c: if(eventId != previousButton) send_multi( EVENT_HARMONY_QUEUE        ); break;
    case 0x9a02: if(eventId != previousButton) send_multi( EVENT_HARMONY_SLEEP        ); break;
    case 0x9f08: if(eventId != previousButton) send_multi( EVENT_HARMONY_RED          ); break;
    case 0x9f07: if(eventId != previousButton) send_multi( EVENT_HARMONY_GREEN        ); break;
    case 0x9f04: if(eventId != previousButton) send_multi( EVENT_HARMONY_YELLOW       ); break;
    case 0x9f02: if(eventId != previousButton) send_multi( EVENT_HARMONY_BLUE         ); break;

    case 0x03: case 0x02: case 0x05: case 0x04: case 0x9603: case 0x9602: case 0x9605: case 0x9604:
      // menu and pause need special treatment
      if(previousButton != eventId) {
        holdButtonSent = 0;
      } else {
        if(millis() - buttonStart > HOLD_TIMEOUT && !holdButtonSent) {
          set_led(hold_mode);
          switch(eventId) {
            case 0x03: case 0x02: send_button(EVENT_HOLD_MENU); break;
            case 0x05: case 0x04: send_button(EVENT_HOLD_PLAY); break;
            case 0x9602: send_multi(EVENT_HARMONY_HOLD_MENU); break;
            case 0x9604: send_multi(EVENT_HARMONY_HOLD_OK); break;
          }
          holdButtonSent = 1;
        }
      }
      break;
    case 0x00:
      // button timeout
      switch(previousButton) {
        case 0x0a:
        case 0x0b:
          send_button(EVENT_UP | EVENT_RELEASE); break;
        case 0x0c:
        case 0x0d:
          send_button(EVENT_DOWN | EVENT_RELEASE); break;
        case 0x09:
        case 0x08:
          send_button(EVENT_LEFT | EVENT_RELEASE); break;
        case 0x06:
        case 0x07:
          send_button(EVENT_RIGHT | EVENT_RELEASE); break;
        case 0x960b: send_multi(EVENT_HARMONY_UP        | EVENT_RELEASE); break;
        case 0x960d: send_multi(EVENT_HARMONY_DOWN      | EVENT_RELEASE); break;
        case 0x9608: send_multi(EVENT_HARMONY_LEFT      | EVENT_RELEASE); break;
        case 0x9607: send_multi(EVENT_HARMONY_RIGHT     | EVENT_RELEASE); break;
        case 0x980a: send_multi(EVENT_HARMONY_VOLUP     | EVENT_RELEASE); break;
        case 0x980c: send_multi(EVENT_HARMONY_VOLDOWN   | EVENT_RELEASE); break;
        case 0x9a0b: send_multi(EVENT_HARMONY_REWIND    | EVENT_RELEASE); break;
        case 0x9a0d: send_multi(EVENT_HARMONY_FORWARD   | EVENT_RELEASE); break;
      }
      break;
    default:
      if(debug) printf("unknown\n");
  }
  previousButton = eventId;
}

int readPairedAddressId() {
  FILE *fp = fopen(remoteIdFile, "r");
  if(fp != NULL) {
    int address;
    fscanf(fp, "%d", &address);
    fclose(fp);   
    return address;
  } else {
    return 0;
  }  
}

void writePairedAddressId(int address) {
  FILE *fp = fopen(remoteIdFile, "w");
  if(fp != NULL) {
    fprintf(fp, "%d\n", address);
    fclose(fp);   
  } else {
    printf("Could not open file `%s' for writing.\n", remoteIdFile); 
  }  
}

char event_map[] = { 0x00, 0x00, 0x03, 0x02, 0x04, 0x04 };

void handle_special(struct ir_command command) {
  static unsigned char previousEventId;
  if(command.eventId != previousEventId) {
    switch(command.eventId) {
      case 0x02: case 0x03: // pair!
        if(pairedRemoteId != 0) {
          printf("Already paired: %d\n", command.address);
          remote_pair_failed.Send(sockfd, my_addr);
        } else {
          printf("Pairing ID: %d\n", command.address);
          writePairedAddressId(command.address);
          pairedRemoteId = command.address;
          remote_paired.Send(sockfd, my_addr);
        }
        break;
      case 0x04: case 0x05: // unpair!
        printf("Unpairing ID: %d\n", command.address);
        writePairedAddressId(0);
        pairedRemoteId = 0;
        remote_unpaired.Send(sockfd, my_addr);
        break;  
    }
    previousEventId = command.eventId;
  }
  
}

void usage(int argc, char **argv) {
  if (argc >=1) {
    printf("Usage: %s [-i mode] [-s mode] [-H mode] [-b mode] [-B] [-h]\n", argv[0]);
    printf("  Options:\n");
    printf("      -m\tEnable multi-remote support.\n");
    printf("      -i\tChange the LED mode for when the receiver is idle.\n");
    printf("      -b\tChange the LED mode for when the receiver is receiving a button press.\n");
    printf("      -H\tChange the LED mode for when the hold event is triggered.\n");
    printf("      -s\tChange the LED mode for when a special event is received.\n");
    printf("      -B\tSwitch LED to low brightness.\n");
    printf("      -d\tEnable debug output,\n");
    printf("      -h\tShow this help screen.\n\n");
    printf("Supported LED modes:\n");
    printf("  0: off\n");
    printf("  1: amber\n");
    printf("  2: blink amber\n");
    printf("  3: white\n");
    printf("  4: blink white\n");
    printf("  5: blink both\n");
    printf("\n");
  }
}

int main(int argc, char **argv) {
  struct ir_command command;
  struct ir_command timeoutCommand;
 
  int c;
  
  int led_brightness = 1;
 
  while ((c = getopt (argc, argv, "mBi:b:s:H:hd")) != -1)
  switch (c) {
    case 'm':
      multi_mode = 1; break;
    case 'i':
      idle_mode = atol(optarg); break;
    case 'b':
      button_mode = atol(optarg); break;
    case 's':
      special_mode = atol(optarg); break;
    case 'H':
      hold_mode = atol(optarg); break;
    case 'd':
      debug = 1; break;
    case 'h':
      usage(argc,argv); exit(0); break;
    case 'B':
      led_brightness = 0; break;
    case '?':
      switch(optopt) {
        case 'i': case 'b': case 's': case 'H':
             fprintf (stderr, "Option -%c requires an argument.\n", optopt);
             break;
        default:
             if (isprint (optopt))
               fprintf (stderr, "Unknown option `-%c'.\n", optopt);
             else
               fprintf (stderr,
                        "Unknown option character `\\x%x'.\n",
                        optopt);
      }
      return 1;
    default:
      abort();
  }
  
  set_led_brightness(led_brightness);
  
  memset(&timeoutCommand, 0, sizeof(timeoutCommand));
  
  if(debug) printf("Creating socket...\n");
  
  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0)
  {
    printf("Error creating socket\n");
    return -1;
  }

  if(debug) printf("Preparing button map...\n");
 
  button_map[EVENT_UP]          = new CPacketBUTTON(EVENT_UP,         "JS0:AppleRemote", BTN_DOWN);
  button_map[EVENT_DOWN]        = new CPacketBUTTON(EVENT_DOWN,       "JS0:AppleRemote", BTN_DOWN);
  button_map[EVENT_LEFT]        = new CPacketBUTTON(EVENT_LEFT,       "JS0:AppleRemote", BTN_DOWN);
  button_map[EVENT_RIGHT]       = new CPacketBUTTON(EVENT_RIGHT,      "JS0:AppleRemote", BTN_DOWN);
  button_map[EVENT_PLAY]        = new CPacketBUTTON(EVENT_PLAY,       "JS0:AppleRemote", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE);
  button_map[EVENT_MENU]        = new CPacketBUTTON(EVENT_MENU,       "JS0:AppleRemote", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE);
  button_map[EVENT_HOLD_PLAY]   = new CPacketBUTTON(EVENT_HOLD_PLAY,  "JS0:AppleRemote", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE);
  button_map[EVENT_HOLD_MENU]   = new CPacketBUTTON(EVENT_HOLD_MENU,  "JS0:AppleRemote", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE);
  button_map[EVENT_UP | EVENT_RELEASE    ]              = new CPacketBUTTON(EVENT_UP,      "JS0:AppleRemote", BTN_UP);
  button_map[EVENT_DOWN | EVENT_RELEASE  ]              = new CPacketBUTTON(EVENT_DOWN,    "JS0:AppleRemote", BTN_UP);
  button_map[EVENT_LEFT | EVENT_RELEASE  ]              = new CPacketBUTTON(EVENT_LEFT,    "JS0:AppleRemote", BTN_UP);
  button_map[EVENT_RIGHT | EVENT_RELEASE ]              = new CPacketBUTTON(EVENT_RIGHT,   "JS0:AppleRemote", BTN_UP);
  
  button_map[EVENT_EXTRA_PLAY]                        = new CPacketBUTTON(EVENT_EXTRA_PLAY,           "JS0:AppleRemote", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE);
  button_map[EVENT_EXTRA_PAUSE]                       = new CPacketBUTTON(EVENT_EXTRA_PAUSE,          "JS0:AppleRemote", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE);
  button_map[EVENT_EXTRA_STOP]                        = new CPacketBUTTON(EVENT_EXTRA_STOP,           "JS0:AppleRemote", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE);
  button_map[EVENT_EXTRA_REPLAY]                      = new CPacketBUTTON(EVENT_EXTRA_REPLAY,         "JS0:AppleRemote", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE);
  button_map[EVENT_EXTRA_SKIP]                        = new CPacketBUTTON(EVENT_EXTRA_SKIP,           "JS0:AppleRemote", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE);
  button_map[EVENT_EXTRA_REWIND]                      = new CPacketBUTTON(EVENT_EXTRA_REWIND,         "JS0:AppleRemote", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE);
  button_map[EVENT_EXTRA_FORWARD]                     = new CPacketBUTTON(EVENT_EXTRA_FORWARD,        "JS0:AppleRemote", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE);
  button_map[EVENT_EXTRA_PAGEUP]                      = new CPacketBUTTON(EVENT_EXTRA_PAGEUP,         "JS0:AppleRemote", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE);
  button_map[EVENT_EXTRA_PAGEDOWN]                    = new CPacketBUTTON(EVENT_EXTRA_PAGEDOWN,       "JS0:AppleRemote", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE);

  multi_map[EVENT_HARMONY_UP]                        = new CPacketBUTTON(EVENT_HARMONY_UP,           "JS0:Harmony", BTN_DOWN);
  multi_map[EVENT_HARMONY_UP        | EVENT_RELEASE] = new CPacketBUTTON(EVENT_HARMONY_UP,           "JS0:Harmony", BTN_UP);
  multi_map[EVENT_HARMONY_DOWN]                      = new CPacketBUTTON(EVENT_HARMONY_DOWN,         "JS0:Harmony", BTN_DOWN);
  multi_map[EVENT_HARMONY_DOWN      | EVENT_RELEASE] = new CPacketBUTTON(EVENT_HARMONY_DOWN,         "JS0:Harmony", BTN_UP);
  multi_map[EVENT_HARMONY_LEFT]                      = new CPacketBUTTON(EVENT_HARMONY_LEFT,         "JS0:Harmony", BTN_DOWN);
  multi_map[EVENT_HARMONY_LEFT      | EVENT_RELEASE] = new CPacketBUTTON(EVENT_HARMONY_LEFT,         "JS0:Harmony", BTN_UP);
  multi_map[EVENT_HARMONY_RIGHT]                     = new CPacketBUTTON(EVENT_HARMONY_RIGHT,        "JS0:Harmony", BTN_DOWN);
  multi_map[EVENT_HARMONY_RIGHT     | EVENT_RELEASE] = new CPacketBUTTON(EVENT_HARMONY_RIGHT,        "JS0:Harmony", BTN_UP);
  multi_map[EVENT_HARMONY_OK]                        = new CPacketBUTTON(EVENT_HARMONY_OK,           "JS0:Harmony", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE);
  multi_map[EVENT_HARMONY_MENU]                      = new CPacketBUTTON(EVENT_HARMONY_MENU,         "JS0:Harmony", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE);
  multi_map[EVENT_HARMONY_HOLD_OK]                   = new CPacketBUTTON(EVENT_HARMONY_HOLD_OK,      "JS0:Harmony", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE);
  multi_map[EVENT_HARMONY_HOLD_MENU]                 = new CPacketBUTTON(EVENT_HARMONY_HOLD_MENU,    "JS0:Harmony", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE);
  multi_map[EVENT_HARMONY_PLAY]                      = new CPacketBUTTON(EVENT_HARMONY_PLAY,         "JS0:Harmony", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE);
  multi_map[EVENT_HARMONY_STOP]                      = new CPacketBUTTON(EVENT_HARMONY_STOP,         "JS0:Harmony", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE);
  multi_map[EVENT_HARMONY_PAUSE]                     = new CPacketBUTTON(EVENT_HARMONY_PAUSE,        "JS0:Harmony", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE);
  multi_map[EVENT_HARMONY_REPLAY]                    = new CPacketBUTTON(EVENT_HARMONY_REPLAY,       "JS0:Harmony", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE);
  multi_map[EVENT_HARMONY_SKIP]                      = new CPacketBUTTON(EVENT_HARMONY_SKIP,         "JS0:Harmony", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE);
  multi_map[EVENT_HARMONY_REWIND]                    = new CPacketBUTTON(EVENT_HARMONY_REWIND,       "JS0:Harmony", BTN_DOWN);
  multi_map[EVENT_HARMONY_REWIND    | EVENT_RELEASE] = new CPacketBUTTON(EVENT_HARMONY_REWIND,       "JS0:Harmony", BTN_UP);
  multi_map[EVENT_HARMONY_FORWARD]                   = new CPacketBUTTON(EVENT_HARMONY_FORWARD,      "JS0:Harmony", BTN_DOWN);
  multi_map[EVENT_HARMONY_FORWARD   | EVENT_RELEASE] = new CPacketBUTTON(EVENT_HARMONY_FORWARD,      "JS0:Harmony", BTN_UP);
  multi_map[EVENT_HARMONY_RECORD]                    = new CPacketBUTTON(EVENT_HARMONY_RECORD,       "JS0:Harmony", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE);
  multi_map[EVENT_HARMONY_PREV]                      = new CPacketBUTTON(EVENT_HARMONY_PREV,         "JS0:Harmony", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE);
  multi_map[EVENT_HARMONY_GUIDE]                     = new CPacketBUTTON(EVENT_HARMONY_GUIDE,        "JS0:Harmony", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE);
  multi_map[EVENT_HARMONY_INFO]                      = new CPacketBUTTON(EVENT_HARMONY_INFO,         "JS0:Harmony", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE);
  multi_map[EVENT_HARMONY_EXIT]                      = new CPacketBUTTON(EVENT_HARMONY_EXIT,         "JS0:Harmony", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE);
  multi_map[EVENT_HARMONY_VOLUP]                     = new CPacketBUTTON(EVENT_HARMONY_VOLUP,        "JS0:Harmony", BTN_DOWN);
  multi_map[EVENT_HARMONY_VOLUP     | EVENT_RELEASE] = new CPacketBUTTON(EVENT_HARMONY_VOLUP,        "JS0:Harmony", BTN_UP);
  multi_map[EVENT_HARMONY_VOLDOWN]                   = new CPacketBUTTON(EVENT_HARMONY_VOLDOWN,      "JS0:Harmony", BTN_DOWN);
  multi_map[EVENT_HARMONY_VOLDOWN   | EVENT_RELEASE] = new CPacketBUTTON(EVENT_HARMONY_VOLDOWN,      "JS0:Harmony", BTN_UP);
  multi_map[EVENT_HARMONY_1]                         = new CPacketBUTTON(EVENT_HARMONY_1,            "JS0:Harmony", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE);
  multi_map[EVENT_HARMONY_2]                         = new CPacketBUTTON(EVENT_HARMONY_2,            "JS0:Harmony", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE);
  multi_map[EVENT_HARMONY_3]                         = new CPacketBUTTON(EVENT_HARMONY_3,            "JS0:Harmony", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE);
  multi_map[EVENT_HARMONY_4]                         = new CPacketBUTTON(EVENT_HARMONY_4,            "JS0:Harmony", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE);
  multi_map[EVENT_HARMONY_5]                         = new CPacketBUTTON(EVENT_HARMONY_5,            "JS0:Harmony", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE);
  multi_map[EVENT_HARMONY_6]                         = new CPacketBUTTON(EVENT_HARMONY_6,            "JS0:Harmony", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE);
  multi_map[EVENT_HARMONY_7]                         = new CPacketBUTTON(EVENT_HARMONY_7,            "JS0:Harmony", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE);
  multi_map[EVENT_HARMONY_8]                         = new CPacketBUTTON(EVENT_HARMONY_8,            "JS0:Harmony", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE);
  multi_map[EVENT_HARMONY_9]                         = new CPacketBUTTON(EVENT_HARMONY_9,            "JS0:Harmony", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE);
  multi_map[EVENT_HARMONY_0]                         = new CPacketBUTTON(EVENT_HARMONY_0,            "JS0:Harmony", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE);
  multi_map[EVENT_HARMONY_CLEAR]                     = new CPacketBUTTON(EVENT_HARMONY_CLEAR,        "JS0:Harmony", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE);
  multi_map[EVENT_HARMONY_ENTER]                     = new CPacketBUTTON(EVENT_HARMONY_ENTER,        "JS0:Harmony", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE);
  multi_map[EVENT_HARMONY_MUTE]                      = new CPacketBUTTON(EVENT_HARMONY_MUTE,         "JS0:Harmony", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE);
  multi_map[EVENT_HARMONY_ASPECT]                    = new CPacketBUTTON(EVENT_HARMONY_ASPECT,       "JS0:Harmony", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE);
  multi_map[EVENT_HARMONY_F1]                        = new CPacketBUTTON(EVENT_HARMONY_F1,           "JS0:Harmony", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE);
  multi_map[EVENT_HARMONY_F2]                        = new CPacketBUTTON(EVENT_HARMONY_F2,           "JS0:Harmony", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE);
  multi_map[EVENT_HARMONY_F3]                        = new CPacketBUTTON(EVENT_HARMONY_F3,           "JS0:Harmony", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE);
  multi_map[EVENT_HARMONY_F4]                        = new CPacketBUTTON(EVENT_HARMONY_F4,           "JS0:Harmony", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE);
  multi_map[EVENT_HARMONY_F5]                        = new CPacketBUTTON(EVENT_HARMONY_F5,           "JS0:Harmony", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE);
  multi_map[EVENT_HARMONY_F6]                        = new CPacketBUTTON(EVENT_HARMONY_F6,           "JS0:Harmony", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE);
  multi_map[EVENT_HARMONY_F7]                        = new CPacketBUTTON(EVENT_HARMONY_F7,           "JS0:Harmony", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE);
  multi_map[EVENT_HARMONY_F8]                        = new CPacketBUTTON(EVENT_HARMONY_F8,           "JS0:Harmony", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE);
  multi_map[EVENT_HARMONY_F9]                        = new CPacketBUTTON(EVENT_HARMONY_F9,           "JS0:Harmony", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE);
  multi_map[EVENT_HARMONY_F10]                       = new CPacketBUTTON(EVENT_HARMONY_F10,          "JS0:Harmony", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE);
  multi_map[EVENT_HARMONY_F11]                       = new CPacketBUTTON(EVENT_HARMONY_F11,          "JS0:Harmony", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE);
  multi_map[EVENT_HARMONY_F12]                       = new CPacketBUTTON(EVENT_HARMONY_F12,          "JS0:Harmony", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE);
  multi_map[EVENT_HARMONY_F13]                       = new CPacketBUTTON(EVENT_HARMONY_F13,          "JS0:Harmony", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE);
  multi_map[EVENT_HARMONY_F14]                       = new CPacketBUTTON(EVENT_HARMONY_F14,          "JS0:Harmony", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE);
  multi_map[EVENT_HARMONY_CHANUP]                    = new CPacketBUTTON(EVENT_HARMONY_CHANUP,       "JS0:Harmony", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE);
  multi_map[EVENT_HARMONY_CHANDOWN]                  = new CPacketBUTTON(EVENT_HARMONY_CHANDOWN,     "JS0:Harmony", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE);
  multi_map[EVENT_HARMONY_LRGDOWN]                   = new CPacketBUTTON(EVENT_HARMONY_LRGDOWN,      "JS0:Harmony", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE);
  multi_map[EVENT_HARMONY_LRGUP]                     = new CPacketBUTTON(EVENT_HARMONY_LRGUP,        "JS0:Harmony", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE);
  multi_map[EVENT_HARMONY_PWRTOGGLE]                 = new CPacketBUTTON(EVENT_HARMONY_PWRTOGGLE,    "JS0:Harmony", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE);
  multi_map[EVENT_HARMONY_QUEUE]                     = new CPacketBUTTON(EVENT_HARMONY_QUEUE,        "JS0:Harmony", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE);
  multi_map[EVENT_HARMONY_SLEEP]                     = new CPacketBUTTON(EVENT_HARMONY_SLEEP,        "JS0:Harmony", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE);
  multi_map[EVENT_HARMONY_RED]                       = new CPacketBUTTON(EVENT_HARMONY_RED,          "JS0:Harmony", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE);
  multi_map[EVENT_HARMONY_GREEN]                     = new CPacketBUTTON(EVENT_HARMONY_GREEN,        "JS0:Harmony", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE);
  multi_map[EVENT_HARMONY_YELLOW]                    = new CPacketBUTTON(EVENT_HARMONY_YELLOW,       "JS0:Harmony", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE);
  multi_map[EVENT_HARMONY_BLUE]                      = new CPacketBUTTON(EVENT_HARMONY_BLUE,         "JS0:Harmony", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE);

  pairedRemoteId = readPairedAddressId();
  
  if(debug) printf("Paired to: %x\n", pairedRemoteId);
  
  if(debug) printf("Ready!\n");
  
  set_led(LEDMODE_WHITE);
  
  int keydown = 0;
  
  set_led(idle_mode);
    
  while(1){
    int result = usb_interrupt_read(get_ir(), 0x82, (char*) &command, sizeof(command), keydown ? BUTTON_TIMEOUT : 0);  
    if(result > 0) {
      // we have an IR code!
      unsigned long start = millis();
      if(debug) dumphex((unsigned char*) &command, result);
      
      if(command.flags == 0x26) {
        // set
        command.event = 0xee;
      }

      switch(command.event) {
        case 0xee:
        case 0xe5: 
          if(pairedRemoteId == 0 || command.address == pairedRemoteId || (is_multi_candidate(command))) {
            set_led(button_mode);
            handle_button(command);
          }
          break;
        case 0xe0:
          set_led(special_mode);
          handle_special(command);
          break;
        default:
          if(debug) printf("Unknown event %x\n", command.event);
      }
      keydown = 1;
      
    } else if(result == -110) {
      // timeout, reset led
      keydown = 0;                        
      set_led(idle_mode);
      handle_button(timeoutCommand);
      handle_special(timeoutCommand);
    } else {
      // something else
      keydown = 0;
      if(debug) printf("Got nuffing: %d\n", result);
    }
  }
  reattach();
}
