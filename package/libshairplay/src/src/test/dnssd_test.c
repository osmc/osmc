#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "dnssd.h"
#include "compat.h"

int
main(int argc, char *argv[])
{
	const char hwaddr[] = { 0x01, 0x23, 0x45, 0x67, 0x89, 0xAB };
	dnssd_t *dnssd;


#ifdef WIN32
	WORD wVersionRequested;
	WSADATA wsaData;

	wVersionRequested = MAKEWORD(2, 2);
	if (WSAStartup(wVersionRequested, &wsaData) != 0) {
		return -1;
	}
	if (LOBYTE(wsaData.wVersion) != 2 ||
	    HIBYTE(wsaData.wVersion) != 2) {
		return -1;
	}
#endif

	dnssd = dnssd_init(NULL);
	if (!dnssd) {
		printf("Failed to init dnssd\n");
		return -1;
	}
	dnssd_register_raop(dnssd, "Test", 5000, hwaddr, sizeof(hwaddr));
	dnssd_register_airplay(dnssd, "Test", 6000, hwaddr, sizeof(hwaddr));

	sleepms(60000);

	dnssd_unregister_raop(dnssd);
	dnssd_unregister_airplay(dnssd);
	dnssd_destroy(dnssd);

#ifdef WIN32
	WSACleanup();
#endif

	return 0;
}
