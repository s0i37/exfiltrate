#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "qrcodegen.h"
#include <windows.h>

/*
https://github.com/nayuki/QR-Code-generator
cl /c qr_upload.c
cl /c qrcodegen.c
link /out:qr_upload.exe qr_upload.obj qrcodegen.obj
chcp 866
*/

int buf_size;

static void printQr(const uint8_t qrcode[]) {
	int size = qrcodegen_getSize(qrcode);
	int border = 4;
	for (int y = -border; y < size + border; y++) {
		for (int x = -border; x < size + border; x++) {
			fputs((qrcodegen_getModule(qrcode, x, y) ? "\xdb\xdb" : "  "), stdout);
		}
		fputs("\n", stdout);
	}
	fputs("\n", stdout);
}

static void qrcode(unsigned char *buf, int len) {

	enum qrcodegen_Ecc errCorLvl = qrcodegen_Ecc_LOW;  // Error correction level
	
	// Make and print the QR Code symbol
	uint8_t qrcode[qrcodegen_BUFFER_LEN_MAX];
	uint8_t tempBuffer[qrcodegen_BUFFER_LEN_MAX];
	//bool ok = qrcodegen_encodeText(buf, tempBuffer, qrcode, errCorLvl,
	//	qrcodegen_VERSION_MIN, qrcodegen_VERSION_MAX, qrcodegen_Mask_AUTO, true);

	struct qrcodegen_Segment seg;
	seg.mode = qrcodegen_Mode_BYTE;
	seg.numChars = len;
	seg.bitLength = len*8;
	seg.data = buf;
	bool ok = qrcodegen_encodeSegments(&seg, 1, errCorLvl, tempBuffer, qrcode);
	if (ok)
		printQr(qrcode);
}

int main(int argc, char **argv) {
	FILE *f;
	unsigned char *buf;
	char *filename;
	int timeout;
	int len;
	short num = 0;

	filename = argv[1];
	timeout = getenv("TIMEOUT") ? atoi(getenv("TIMEOUT")) : 1000;
	buf_size = getenv("SIZE") ? atoi(getenv("SIZE")) : 100;

	f = fopen(filename, "rb");
	buf = malloc(buf_size);
	while(!feof(f))
	{
		*((short *)buf) = num;
		len = fread(buf+2, 1, buf_size-2, f);
		qrcode(buf, len);
		Sleep(timeout);
		system("cls");
		memset(buf, '\x00', buf_size);
		num++;
	}
	free(buf);
	fclose(f);
	return EXIT_SUCCESS;
}