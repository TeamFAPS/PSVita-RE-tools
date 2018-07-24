#pragma once

#include <inttypes.h>

// some info taken from the wiki, see http://vitadevwiki.com/index.php?title=SELF_File_Format

#pragma pack(push, 1)
typedef struct {
	uint32_t magic;                 /* 53434500 = SCE\0 */
	uint32_t version;               /* header version 3*/
	uint16_t sdk_type;              /* */
	uint16_t header_type;           /* 1 self, 2 unknown, 3 pkg */
	uint32_t metadata_offset;       /* metadata offset */
	uint64_t header_len;            /* self header length */
	uint64_t elf_filesize;          /* ELF file length */
	uint64_t self_filesize;         /* SELF file length */
	uint64_t unknown;               /* UNKNOWN */
	uint64_t self_offset;           /* SELF offset */
	uint64_t appinfo_offset;        /* app info offset */
	uint64_t elf_offset;            /* ELF #1 offset */
	uint64_t phdr_offset;           /* program header offset */
	uint64_t shdr_offset;           /* section header offset */
	uint64_t section_info_offset;   /* section info offset */
	uint64_t sceversion_offset;     /* version offset */
	uint64_t controlinfo_offset;    /* control info offset */
	uint64_t controlinfo_size;      /* control info size */
	uint64_t padding;
} SCE_header;

typedef struct {
	uint64_t authid;                /* auth id */
	uint32_t vendor_id;             /* vendor id */
	uint32_t self_type;             /* app type */
	uint64_t version;               /* app version */
	uint64_t padding;               /* UNKNOWN */
} SCE_appinfo;

typedef struct {
	uint32_t unk1;
	uint32_t unk2;
	uint32_t unk3;
	uint32_t unk4;
} SCE_version;

typedef struct {
  uint32_t type; // 4==PSVita ELF digest info; 5==PSVita NPDRM info; 6==PSVita boot param info; 7==PSVita shared secret info
  uint32_t size;
  uint64_t next; // 1 if another Control Info structure follows else 0
  union {
    // type 4, 0x50 bytes
	struct  { // 0x40 bytes of data
      uint8_t constant[0x14]; // same for every PSVita/PS3 SELF, hardcoded in make_fself.exe: 627CB1808AB938E32C8C091708726A579E2586E4
      uint8_t elf_digest[0x20]; // on PSVita: SHA-256 of source ELF file, on PS3: SHA-1
      uint8_t padding[8];
      uint32_t min_required_fw; // ex: 0x363 for 3.63
    } PSVita_elf_digest_info;
    // type 5, 0x110 bytes
    struct { // 0x80 bytes of data
      uint32_t magic;               // 7F 44 52 4D (".DRM")
      uint32_t finalized_flag;      // ex: 80 00 00 01
      uint32_t drm_type;            // license_type ex: 2 local, 0XD free with license
      uint32_t padding;
      uint8_t content_id[0x30];
      uint8_t digest[0x10];         // ?sha-1 hash of debug self/sprx created using make_fself_npdrm?
      uint8_t padding_78[0x78];
      uint8_t hash_signature[0x38]; // unknown hash/signature
    } PSVita_npdrm_info;
    // type 6, 0x110 bytes
    struct { // 0x100 bytes of data
      uint32_t is_used; // 0=false, 1=true
      uint8_t boot_param[0x9C]; // ex: starting with 02 00 00 00
    } PSVita_boot_param_info;
    // type 7, 0x50 bytes
    struct { // 0x40 bytes of data
      uint8_t shared_secret_0[0x10]; // ex: 0x7E7FD126A7B9614940607EE1BF9DDF5E or full of zeroes
      uint8_t shared_secret_1[0x10]; // ex: full of zeroes
      uint8_t shared_secret_2[0x10]; // ex: full of zeroes
      uint8_t shared_secret_3[0x10]; // ex: full of zeroes
    } PSVita_shared_secret_info;
  };
} __attribute__((packed)) PSVita_CONTROL_INFO;

typedef struct {
	uint64_t offset;
	uint64_t length;
	uint64_t compression; // 1 = uncompressed, 2 = compressed
	uint64_t encryption; // 1 = encrypted, 2 = plain
} segment_info;

#pragma pack(pop)

enum {
	HEADER_LEN = 0x1000,
	SCE_MAGIC = 0x454353
};
