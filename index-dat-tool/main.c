// index-dat-tool
// zecoxao and CelesteBlue 2019

#include "tools.h"
#include "types.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>

#include "sha2.h"

const char dat_key_vita_new[0x20] = {0x27,0x2A,0xE4,0x37,0x8C,0xB0,0x6B,0xF3,0xF6,0x58,0xF5,0x1C,0x77,0xAC,0xA2,0x76,0x9B,0xE8,0x7F,0xB1,0x9B,0xBF,0x3D,0x4D,0x6B,0x1B,0x0E,0xD2,0x26,0xE3,0x9C,0xC6};
const char dat_key_vita_old[0x20] = {0x06,0xCC,0x2E,0x8F,0xD4,0x08,0x05,0xA7,0x36,0xF1,0x7C,0xF2,0xC1,0x3D,0x58,0xA6,0xC8,0xCF,0x10,0x7E,0x9E,0x4A,0x66,0xAE,0x25,0xD3,0x9C,0xA2,0x1C,0x25,0x31,0xCC};
const char dat_iv[0x10] = {0x37,0xFA,0x4E,0xD2,0xB6,0x61,0x8B,0x59,0xB3,0x4F,0x77,0x0F,0xBB,0x92,0x94,0x7B};

int generation_mode = 0;

int main (int argc, char *argv[]) {
	
	FILE *in = NULL;
	size_t len;
	u8 *data;

	if (argc != 3 && argc != 4)
		fail("usage: undat [-g] index.dat version.txt");

	for (int i=0; i<argc; i++) {
		if (strcmp(argv[i], "-g") == 0) {
			generation_mode = 1;
		}
	}
	
	if (!generation_mode) {
		printf("running in decryption mode");
		in = fopen(argv[1], "rb");
		if (in == NULL)
			fail("Unable to open %s", argv[1]);
		fseek(in, 0, SEEK_END);
		len = ftell(in);
		fseek(in, 0, SEEK_SET);
		if (len < 0x20)
			fail("Invalid index.dat size : 0x%X", len);
		data = malloc(len);
		if (fread(data, 1, len, in) != len)
			fail("Unable to read index.dat file");
		fclose(in);

		aes256cbc(dat_key_vita_old, dat_iv, data, len, data);
		
		u8 digest[0x20];
		sha2(data + 32, len - 32, digest, 0);
		if (memcmp(data, digest, 0x20) != 0)
			fail("SHA256 mismatch");

		memcpy_to_file(argv[2], data + 32, len - 32);
	} else {
		printf("running in generation mode");
		size_t new_len;
		u8 *out;
		in = fopen(argv[2], "rb");
		if (in == NULL)
			fail("Unable to open %s", argv[2]);
		fseek(in, 0, SEEK_END);
		len = ftell(in);
		fseek(in, 0, SEEK_SET);
		data = malloc(len);
		if (fread(data, 1, len, in) != len)
			fail("Unable to read version.txt file");
		fclose(in);

		new_len = len + 32;
		if (new_len % 16 != 0)
			new_len += 16 - (new_len % 16);
		out = malloc(new_len);
		memset(out, '\n', new_len);
		memset(out, '0', 32);
		memcpy(out + 32, data, len);
		sha2(out + 32, new_len - 32, out, 0);

		aes256cbc_enc(dat_key_vita_old, dat_iv, out, new_len, out);

		memcpy_to_file(argv[1], out, new_len);
	}

	return 0;
}
