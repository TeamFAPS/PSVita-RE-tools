// Copyright 2010       Sven Peter <svenpeter@gmail.com>
// Licensed under the terms of the GNU GPL, version 2
// http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt
#ifndef TYPES_H__
#define TYPES_H__

#include <stdint.h>

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;


struct elf_phdr {
	u32 p_type;
	u64 p_off;
	u64 p_vaddr;
	u64 p_paddr;
	u64 p_filesz;
	u64 p_memsz;
	u32 p_flags;
	u64 p_align;

	void *ptr;
};

struct elf_shdr {
	u32 sh_name;
	u32 sh_type;
	u32 sh_flags;
	u64 sh_addr;
	u64 sh_offset;
	u32 sh_size;
	u32 sh_link;
	u32 sh_info;
	u32 sh_addralign;
	u32 sh_entsize;
};

#define	ET_NONE		0
#define ET_REL		1
#define	ET_EXEC		2
#define	ET_DYN		3
#define	ET_CORE		4
#define	ET_LOOS		0xfe00
#define	ET_HIOS		0xfeff
#define ET_LOPROC	0xff00
#define	ET_HIPROC	0xffff
struct elf_hdr {
	char e_ident[16];
	u16 e_type;
	u16 e_machine;
	u32 e_version;
	u64 e_entry;
	u64 e_phoff;
	u64 e_shoff;
	u32 e_flags;
	u16 e_ehsize;
	u16 e_phentsize;
	u16 e_phnum;
	u16 e_shentsize;
	u16 e_shnum;
	u16 e_shtrndx;
};


struct id2name_tbl {
	u32 id;
	const char *name;
};

struct key {
	u8 key[32];
	u8 iv[16];

	int pub_avail;
	int priv_avail;
	u8 pub[40];
	u8 priv[21];
	u32 ctype;
};

struct keylist {
	u32 n;
	struct key *keys;
    struct key *idps;
    struct key *klic;
    struct key *rif;
    struct key *npdrm_const;
    struct key *free_klicensee;
};

struct rif {
    u8 unk1[0x10]; //version, license type and user number
    u8 titleid[0x30]; //Content ID
    u8 padding[0xC]; //Padding for randomness
    u32 actDatIndex; //Key index on act.dat between 0x00 and 0x7F
    u8 key[0x10]; //encrypted klicensee
    u64 unk2; //timestamp??
    u64 unk3; //Always 0
    u8 rs[0x28];
} __attribute__ ((packed));

struct actdat {
    u8 unk1[0x10]; //Version, User number
    u8 keyTable[0x800]; //Key Table
    u8 unk2[0x800];
    u8 signature[0x28];
} __attribute__ ((packed));

static inline u8 be8(u8 *p)
{
	return *p;
}

static inline u16 be16(u8 *p)
{
	u16 a;

	a  = p[0] << 8;
	a |= p[1];

	return a;
}

static inline u32 be32(u8 *p)
{
	u32 a;

	a  = p[0] << 24;
	a |= p[1] << 16;
	a |= p[2] <<  8;
	a |= p[3] <<  0;

	return a;
}

static inline u64 be64(u8 *p)
{
	u32 a, b;

	a = be32(p);
	b = be32(p + 4);

	return ((u64)a<<32) | b;
}

static inline void wbe16(u8 *p, u16 v)
{
	p[0] = v >>  8;
	p[1] = v;
}

static inline void wbe32(u8 *p, u32 v)
{
	p[0] = v >> 24;
	p[1] = v >> 16;
	p[2] = v >>  8;
	p[3] = v;
}

static inline void wbe64(u8 *p, u64 v)
{
	wbe32(p + 4, v);
	v >>= 32;
	wbe32(p, v);
}

#endif
