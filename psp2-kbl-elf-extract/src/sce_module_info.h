#include <stdint.h>
#include <inttypes.h>

#include "elf.h"


#define MODULE_NAME_MAX_LEN 27

typedef struct SceModuleInfo_common { // size is 0x20
  unsigned short modattribute;
  unsigned char modversion[2];
  char modname[MODULE_NAME_MAX_LEN];
  unsigned char infover;
} SceModuleInfo_common;

typedef struct SceModuleInfo_v0 { // size is 0x34
  SceModuleInfo_common c;
  Elf32_Addr gp_value;
  Elf32_Addr ent_top;
  Elf32_Addr ent_btm;
  Elf32_Addr stub_top;
  Elf32_Addr stub_btm;
} SceModuleInfo_v0;

typedef struct SceModuleInfo_v1 { // size is 0x40
  SceModuleInfo_common c;
  Elf32_Addr gp_value;
  Elf32_Addr ent_top;
  Elf32_Addr ent_btm;
  Elf32_Addr stub_top;
  Elf32_Addr stub_btm;
  Elf32_Word dbg_fingerprint;
  Elf32_Addr start_entry;
  Elf32_Addr stop_entry;
} SceModuleInfo_v1;

typedef struct SceModuleInfo_v2 { // size is 0x48
  SceModuleInfo_common c;
  Elf32_Addr gp_value;
  Elf32_Addr ent_top;
  Elf32_Addr ent_btm;
  Elf32_Addr stub_top;
  Elf32_Addr stub_btm;
  Elf32_Word dbg_fingerprint;
  Elf32_Addr start_entry;
  Elf32_Addr stop_entry;
  Elf32_Addr arm_exidx_top;
  Elf32_Addr arm_exidx_btm;
} SceModuleInfo_v2;

typedef struct SceModuleInfo_v3 { // size is 0x54
  SceModuleInfo_common c;
  Elf32_Addr gp_value;
  Elf32_Addr ent_top;
  Elf32_Addr ent_btm;
  Elf32_Addr stub_top;
  Elf32_Addr stub_btm;
  Elf32_Word dbg_fingerprint;
  Elf32_Addr start_entry;
  Elf32_Addr stop_entry;
  Elf32_Addr arm_exidx_top;
  Elf32_Addr arm_exidx_btm;
  Elf32_Addr tls_start;
  Elf32_Addr tls_filesz;
  Elf32_Addr tls_memsz;
} SceModuleInfo_v3;

typedef struct SceModuleInfo_v6 { // size is 0x5C
  SceModuleInfo_common c;
  Elf32_Addr gp_value;
  Elf32_Addr ent_top;
  Elf32_Addr ent_btm;
  Elf32_Addr stub_top;
  Elf32_Addr stub_btm;
  Elf32_Word dbg_fingerprint;
  Elf32_Addr tls_start;
  Elf32_Addr tls_filesz;
  Elf32_Addr tls_memsz;
  Elf32_Addr start_entry;
  Elf32_Addr stop_entry;
  Elf32_Addr arm_exidx_top;
  Elf32_Addr arm_exidx_btm;
  Elf32_Addr arm_extab_top;
  Elf32_Addr arm_extab_btm;
} SceModuleInfo_v6;

typedef struct _scelibent_psp { // size is 0x10 or 0x14
	Elf32_Addr libname; /* <libname> (0 if NONAME) */
	unsigned short version; /* <version> */
	unsigned short attribute; /* <attribute> */
	unsigned char size; /* struct size in dwords */
	unsigned char nvar; /* number of variables */
	unsigned short nfunc; /* number of functions */
	Elf32_Addr entry_table; /* <entry_table> addr (in .rodata.sceResident) */
	union {
		unsigned int unk_0x10; /* Not always present. Might be alias_table. ex: 0x20000 */
	};
} SceLibEntryTable_psp;

typedef struct _scelibent_common { // size is 0x10
	unsigned char size;
	unsigned char auxattribute;
	unsigned short version;
	unsigned short attribute;
	unsigned short nfunc;
	unsigned short nvar;
	unsigned short ntls;
	unsigned char hashinfo;
	unsigned char hashinfotls;
	unsigned char reserved;
	unsigned char nidaltsets;
} SceLibEntryTable_common;

typedef struct _scelibent_1C { // size is 0x1C
	SceLibEntryTable_common c;
	Elf32_Addr libname;
	Elf32_Addr nid_table;
	Elf32_Addr entry_table;
} SceLibEntryTable_1C;

typedef struct _scelibent_20 { // size is 0x20
	SceLibEntryTable_common c;
	Elf32_Word libname_nid;
	Elf32_Addr libname;
	Elf32_Addr nid_table;
	Elf32_Addr entry_table;
} SceLibEntryTable_20;

typedef struct _scelibstub_psp { // size is 0x14 or 0x18
	Elf32_Addr libname; /* library name */
	unsigned short version; /* version */
	unsigned short attribute; /* attribute */
	unsigned char size; /* struct size */
	unsigned char nvar; /* number of variables */
	unsigned short nfunc; /* number of function stubs */
	Elf32_Addr nid_table; /* functions/variables NID table */
	Elf32_Addr stub_table; /* functions stub table */
	union {
		Elf32_Addr var_table; /* variables table */
	};
} SceLibStubTable_psp;

typedef struct _scelibstub_common { // size is 0xC
	unsigned short size;
	unsigned short version;
	unsigned short attribute;
	unsigned short nfunc;
	unsigned short nvar;
	unsigned short ntls;
} SceLibStubTable_common;

typedef struct _scelibstub_24 { // size is 0x24
	SceLibStubTable_common c;
	Elf32_Word libname_nid;
	Elf32_Addr libname;
	Elf32_Addr func_nid_table;
	Elf32_Addr func_entry_table;
	Elf32_Addr var_nid_table;
	Elf32_Addr var_entry_table;
} SceLibStubTable_24;

typedef struct _scelibstub_2C { // size is 0x2C
	SceLibStubTable_common c;
	unsigned char reserved[4];
	Elf32_Addr libname;
	Elf32_Addr func_nid_table;
	Elf32_Addr func_entry_table;
	Elf32_Addr var_nid_table;
	Elf32_Addr var_entry_table;
	Elf32_Addr tls_nid_table;
	Elf32_Addr tls_entry_table;
} SceLibStubTable_2C;

typedef struct _scelibstub_34 { // size is 0x34
	SceLibStubTable_common c;
	unsigned char reserved[4];
	Elf32_Word libname_nid;
	Elf32_Addr libname;
	Elf32_Word sce_sdk_version;
	Elf32_Addr func_nid_table;
	Elf32_Addr func_entry_table;
	Elf32_Addr var_nid_table;
	Elf32_Addr var_entry_table;
	Elf32_Addr tls_nid_table;
	Elf32_Addr tls_entry_table;
} SceLibStubTable_34;