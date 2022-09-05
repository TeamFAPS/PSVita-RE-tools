#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <dirent.h>

#define ARRAYSIZE(x) ((sizeof(x)/sizeof(0[x])) / ((size_t)(!(sizeof(x) % sizeof(0[x])))))
#define SECTOR_SIZE 0x200

#if (defined(_WIN32) || defined(__WIN32__))
#define mkdir(A, B) mkdir(A)
#endif

typedef struct SceMbrPartEntry { // size is 0x11-bytes
	unsigned int start_lba;
	unsigned int n_sectors;
	unsigned char id;
	unsigned char type;
	unsigned char flag;
	unsigned short acl;
	unsigned int unused;
} __attribute__((packed)) SceMbrPartEntry;

typedef struct SceMbrParts { // size is 0x110-bytes
	SceMbrPartEntry entries[0x10];
} __attribute__((packed)) SceMbrParts;

typedef struct SceMbr { // size is 0x200-bytes
	char magic[0x20];
	unsigned int version;
	unsigned int nSector;
	uint64_t unused;

	unsigned int loader_start;
	unsigned int loader_count;
	unsigned int current_bl_lba;
	unsigned int bl_bank0_lba;

	unsigned int bl_bank1_lba;
	unsigned int current_os_lba;
	uint64_t unused2;
	SceMbrParts parts;
	char unused3[0x6E];
	char unused4[0x30];
	unsigned short signature;
} __attribute__((packed)) SceMbr;

const char *part_id(int id) {
	static char *ids[] = {
		"empty",
		"idstor",
		"sloader",
		"os",
		"vsh",
		"vshdata",
		"vtrm",
		"user",
		"userext",
		"gamero",
		"gamerw",
		"updater",
		"sysdata",
		"mediaid",
		"pidata",
		"unused"
	};
	return ids[id];
}

const char *part_type(int type) {
	if (type == 6)
		return "FAT16";
	else if (type == 7)
		return "exFAT";
	else if (type == 0xDA)
		return "raw";
	return "unknown";
}

void unpack(char *filename) {
	char dirname[256];
	char outpath[256];
	FILE *in;
	if ((in = fopen(filename, "rb")) == NULL) {
		perror("open");
		return;
	}
	snprintf(dirname, 256, "%s_out", filename);
	mkdir(dirname, 0777);
	SceMbr mbr;
	fread(&mbr, 1, sizeof(mbr), in);
	for (int part_idx = 0; part_idx < ARRAYSIZE(mbr.parts.entries); ++part_idx) {
		SceMbrPartEntry *p = &mbr.parts.entries[part_idx];
		printf("Partition %d, id=%s, type=%s, flag=%d, start_lba=0x%08x, n_sectors=0x%08x, acl=0x%08x, unused=0x%08x\n", part_idx, part_id(p->id), part_type(p->type), p->flag, p->start_lba, p->n_sectors, p->acl, p->unused);
		if (memcmp(part_id(p->id), "empty", 5) != 0){
			printf("Unpacking partition %s active=%d start_lba 0x%08x n_sectors 0x%08x...\n", part_id(p->id), p->flag, p->start_lba, p->n_sectors);
			unsigned char *buffer = (unsigned char *) malloc (p->n_sectors * SECTOR_SIZE);
			fseek(in, p->start_lba * SECTOR_SIZE, SEEK_SET);
			fread(buffer, 1, p->n_sectors * SECTOR_SIZE, in);
			snprintf(outpath, 256, "%s/%s%s", dirname, part_id(p->id), p->flag == 0 ? "" : "_active");
			FILE *out;
			if ((out = fopen(outpath, "wb")) == NULL) {
				perror("open");
				return;
			}
			fwrite(buffer, 1, p->n_sectors * SECTOR_SIZE, out);
			fclose(out);
			free(buffer);
		}
	}
	fclose(in);
}

int main (int argc, char *argv[]){
	if (argc > 1)
		unpack(argv[1]);
	else
		unpack("emmc.bin");
	return 0;
}