// wol.cpp : Definiert den Einstiegspunkt für die Konsolenanwendung.
//
#include "stdafx.h"

#ifdef WIN32
#include <winsock2.h>
#include <ws2tcpip.h>

#ifdef _MSC_VER
static size_t strlen(char const * p) {
	int n = 0;
	while (*p++) {
		++n;
	}
	return n;
}
extern "C" void _RTC_Shutdown() {}
extern "C" void _RTC_InitBase() {}
#endif

static HANDLE mstdout;
static void init() {
	mstdout = CreateFileA("con:", GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	WSADATA wsaData;
	WSAStartup(0x202, &wsaData);
}

// text nach stdout schreiben
static unsigned long out;
static void cputs(char const * s)
{
	WriteFile(mstdout, s, strlen(s), &out, 0);
	WriteFile(mstdout, "\r\n", 2, &out, 0);
}
static void printf1(char const * s)
{
	WriteFile(mstdout, s, strlen(s), &out, 0);
}
#define puts cputs

static char buf[100];
#define printf5(a,b,c,d,e) wsprintfA(buf, a, b, c, d, e); printf1(buf)
//#define printf(a,b) wsprintfA(buf, a, b); printf1(buf)
#define printf(a) printf1(a)
#else
#define printf1 printf
#define printf5 printf
#endif


static int sendWol(char * arg)
{
	static char data[102] = {
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff
	};
	struct sockaddr_in addr = { AF_INET, 0 };
	addr.sin_addr.s_addr = 0xffffffff;

	int sock, i, n;
	int option = 1;
	char * r = arg;
	puts(arg);

	// parse the mac address
	n = 0;
	for (;;) {
		int hi;
		if (*r >= '0' && *r <= '9') {
			hi = *r - '0';
		}
		else
			if (*r >= 'A' && *r <= 'F') {
			hi = *r - 'A' + 10;
			}
			else
				if (*r >= 'a' && *r <= 'f') {
			hi = *r - 'a' + 10;
				}
				else
					break;

		++r;

		int lo;
		if (*r >= '0' && *r <= '9') {
			lo = *r - '0';
		}
		else
			if (*r >= 'A' && *r <= 'F') {
			lo = *r - 'A' + 10;
			}
			else
				if (*r >= 'a' && *r <= 'f') {
			lo = *r - 'a' + 10;
				}
				else
					break;

		data[6 + n] = (char)((hi << 4) | lo);
		++n;
		++r;

		if (*r == ' ' || *r == ':' || *r == '-')
			++r;
	}
	if (n != 6) {
		puts("cannot parse macaddress:");
		puts(arg);
		return 2;
	}


	for (i = 12; i < 102; ++i) {
		data[i] = data[i - 6];
	}

	char hostname[256];
	gethostname(hostname, 255);

	PADDRINFOA ai = 0;
	for (getaddrinfo(hostname, 0, 0, &ai); ai; ai = ai->ai_next) {
		if (ai->ai_family != AF_INET)
			continue;

		printf5("sending via %d.%d.%d.%d ... ", ai->ai_addr->sa_data[2] & 0xff, ai->ai_addr->sa_data[3] & 0xff, ai->ai_addr->sa_data[4] & 0xff, ai->ai_addr->sa_data[5] & 0xff);

		n = 0;
		do {
			sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
			if (sock < 0) {
				printf("open socket");
				n = 9;
				break;
			}

			int err = bind(sock, ai->ai_addr, ai->ai_addrlen);
			if (err < 0) {
				printf("bind");
				n = 8;
				break;
			}
			err = setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (char *)&option, sizeof(option));
			if (err < 0) {
				printf("enable broadcast");
				n = 7;
				break;
			}
			err = sendto(sock, data, sizeof(data), 0, (struct sockaddr*)&addr, sizeof(addr));
			if (err != 102) {
				puts("send");
				n = 6;
				break;
			}
		} while (0);
		if (sock > 0)
			closesocket(sock);
		puts(n == 0 ? " ok" : " failed");
	}
	return n;
}


#ifdef WIN32
static char * endOfQuote(char * p) {
	while (*p) {
		if (*p == '"') {
			++p;
			break;
		}
		if (*p == '\\') {
			++p;
			if (!*p)
				break;
		}
		++p;
	}
	return p;
}

static char * endOfWord(char * p) {
	if (*p == '"')
		return endOfQuote(p + 1);
	while ((unsigned char)*p > 32) ++p;
	return p;
}

static char * endOfSpace(char * p) {
	while (*p != 0 && (unsigned char)*p <= 32) ++p;
	return p;
}

#ifdef _MSC_VER
extern "C" int __stdcall mainCRTStartup()
#else
extern "C" int __stdcall MiniMain()
#endif
{
	init();
	char *q, *r, *cmdLine = GetCommandLineA();
	puts(cmdLine);

	char * exe = cmdLine;
	q = endOfWord(exe);
	r = endOfSpace(q);
	*q = 0;

	if (*r == 0) {
		puts("usage: wol <macaddress>\r\ne.g.:  wol aa:bb:C0:12:1d:13 or wol 00-11-22-33-44-55");
		return 1;
	}

	int x = sendWol(r);
	ExitProcess(x);
}
#else
int main(int argc, char ** argv) {
	if (argc < 2)
	{
		puts("usage: wol <macaddress>\r\ne.g.:  wol aa:bb:C0:12:1d:13 or wol 00-11-22-33-44-55");
		return 1;
	}
	return sendWol(argv[1]);
}
#endif

