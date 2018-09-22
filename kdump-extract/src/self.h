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
	uint32_t type;
	uint32_t size;
	uint32_t unk;
	uint32_t pad;
} SCE_controlinfo;

typedef struct {
	SCE_controlinfo common;
	char unk[0x100];
} SCE_controlinfo_5;

typedef struct {
	SCE_controlinfo common;
	uint32_t unk1;
	char unk2[0xFC];
} SCE_controlinfo_6;

typedef struct {
	SCE_controlinfo common;
	char unk[0x40];
} SCE_controlinfo_7;

typedef struct {
	uint8_t e_ident[16];              /* ELF identification */
	uint16_t e_type;                  /* object file type */
	uint16_t e_machine;               /* machine type */
	uint32_t e_version;               /* object file version */
	uint32_t e_entry;                 /* entry point address */
	uint32_t e_phoff;                 /* program header offset */
	uint32_t e_shoff;                 /* section header offset */
	uint32_t e_flags;                 /* processor-specific flags */
	uint16_t e_ehsize;                /* ELF header size */
	uint16_t e_phentsize;             /* size of program header entry */
	uint16_t e_phnum;                 /* number of program header entries */
	uint16_t e_shentsize;             /* size of section header entry */
	uint16_t e_shnum;                 /* number of section header entries */
	uint16_t e_shstrndx;              /* section name string table index */
	uint32_t pad[3];
} ELF_header;

typedef struct {
	uint64_t offset;
	uint64_t length;
	uint64_t compression; // 1 = uncompressed, 2 = compressed
	uint64_t encryption; // 1 = encrypted, 2 = plain
} segment_info;

typedef struct {
    uint32_t p_type;
    uint32_t p_offset;
    uint32_t p_vaddr;
    uint32_t p_paddr;
    uint32_t p_filesz;
    uint32_t p_memsz;
    uint32_t p_flags;
    uint32_t p_align;
} e_phdr;
#pragma pack(pop)

enum {
	HEADER_LEN = 0x1000
};

typedef union {
	uint32_t r_short : 4;
	struct {
		uint32_t r_short     : 4;
		uint32_t r_symseg    : 4;
		uint32_t r_code      : 8;
		uint32_t r_datseg    : 4;
		uint32_t r_offset_lo : 12;
		uint32_t r_offset_hi : 20;
		uint32_t r_addend    : 12;
	} r_short_entry;
	struct {
		uint32_t r_short     : 4;
		uint32_t r_symseg    : 4;
		uint32_t r_code      : 8;
		uint32_t r_datseg    : 4;
		uint32_t r_code2     : 8;
		uint32_t r_dist2     : 4;
		uint32_t r_addend;
		uint32_t r_offset;
	} r_long_entry;
	struct {
		uint32_t r_word1;
		uint32_t r_word2;
		uint32_t r_word3;
	} r_raw_entry;
} SCE_Rel;

typedef struct {
	uint16_t attr;
	uint16_t ver;
	uint8_t name[27];
	uint8_t type;
	uint32_t gp;
	uint32_t expTop;
	uint32_t expBtm;
	uint32_t impTop;
	uint32_t impBtm;
	uint32_t nid;
	uint32_t unk[3];
	uint32_t start;
	uint32_t stop;
	uint32_t exidxTop;
	uint32_t exidxBtm;
	uint32_t extabTop;
	uint32_t extabBtm;
} SceModuleInfo;
typedef struct {
	uint16_t size;
	uint16_t lib_version;
	uint16_t attribute;
	uint16_t num_functions;
	uint32_t num_vars;
	uint32_t num_tls_vars;
	uint32_t module_nid;
	uint32_t lib_name;
	uint32_t nid_table;
	uint32_t entry_table;
} SceExportsTable;

typedef struct {
	uint16_t size;
	uint16_t lib_version;
	uint16_t attribute;
	uint16_t num_functions;
	uint16_t num_vars;
	uint16_t num_tls_vars;
	uint32_t reserved1;
	uint32_t module_nid;
	uint32_t lib_name;
	uint32_t reserved2;
	uint32_t func_nid_table;
	uint32_t func_entry_table;
	uint32_t var_nid_table;
	uint32_t var_entry_table;
	uint32_t tls_nid_table;
	uint32_t tls_entry_table;
} SceImportsTable2xx;

typedef struct {
	uint16_t size;
	uint16_t lib_version;
	uint16_t attribute;
	uint16_t num_functions;
	uint16_t num_vars;
	uint16_t unknown1;
	uint32_t module_nid;
	uint32_t lib_name;
	uint32_t func_nid_table;
	uint32_t func_entry_table;
	uint32_t var_nid_table;
	uint32_t var_entry_table;
} SceImportsTable3xx;