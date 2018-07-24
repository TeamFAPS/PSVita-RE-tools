// taihen

int module_get_export_func(SceUID pid, const char *modname, uint32_t libnid, uint32_t funcnid, uintptr_t *func);

// SceSblAuthMgr

typedef struct SceSelfInfo // size is 0x90
{
	SceUInt64 program_authority_id;
	SceUInt64 padding1;
	uint8_t capability[0x20];
	uint8_t attribute[0x20];
	uint8_t padding2[0x10];
	uint8_t klicensee[0x10];
	uint32_t unk_70;
	uint32_t unk_74;
	uint32_t unk_78;
	uint32_t unk_7C;
	uint32_t unk_80; // ex: 0x10
	uint32_t unk_84;
	uint32_t unk_88;
	uint32_t unk_8C;
} SceSelfInfo;

typedef struct SceSblSmCommContext130 // size is 0x130 as its name indicates
{
	uint32_t unk_0;
	uint32_t self_type; // kernel = 0, user = 1, SM = 2
	SceSelfInfo caller_self_auth_info; // can be obtained with sceKernelGetSelfInfoForKernel
	SceSelfInfo called_self_auth_info; // set by F00D in F00D SceSblSmCommContext130 response
	uint32_t path_id; // can be obtained with sceSblACMgrGetPathIdForKernel or sceIoGetPathIdExForDriver
	uint32_t unk_12C;
} SceSblSmCommContext130;

int (*sceSblAuthMgrSmStartForKernel)(int* ctx);
void *(*sceSysmemMallocForKernel)(size_t size);
int (*sceSysmemFreeForKernel)(void *ptr);
int (*sceSblAuthMgrAuthHeaderForKernel)(int ctx, char *header, int header_size, SceSblSmCommContext130 *context_130);
int (*sceSblAuthMgrSmFinalizeForKernel)(int ctx);
int (*sceSblAuthMgrLoadSelfSegmentForKernel)(int ctx, int segment_number, size_t segment_size, void *output_buffer, size_t program_size);
int (*sceSblAuthMgrLoadSelfBlockForKernel)(int ctx, char *buffer, int offset);
int (*sceIoGetPathIdExForDriver)(SceUID pid, const char *path, int ignored, int *pathId);

// SceNpDrm

typedef struct {
	uint16_t version;                 // 0x00
	uint16_t version_flag;            // 0x02
	uint16_t type;                    // 0x04
	uint16_t flags;                   // 0x06
	uint64_t aid;                     // 0x08
	char content_id[0x30];            // 0x10
	uint8_t key_table[0x10];          // 0x40
	uint8_t key[0x10];                // 0x50
	uint64_t start_time;              // 0x60
	uint64_t expiration_time;         // 0x68
	uint8_t ecdsa_signature[0x28];    // 0x70

	uint64_t flags2;                  // 0x98
	uint8_t key2[0x10];               // 0xA0
	uint8_t unk_B0[0x10];             // 0xB0
	uint8_t openpsid[0x10];           // 0xC0
	uint8_t unk_D0[0x10];             // 0xD0
	uint8_t cmd56_handshake[0x14];    // 0xE0
	uint32_t unk_F4;                  // 0xF4
	uint32_t unk_F8;                  // 0xF8
	uint32_t sku_flag;                // 0xFC
	uint8_t rsa_signature[0x100];     // 0x100
} SceNpDrmLicense;

int ksceNpDrmGetRifVitaKey(SceNpDrmLicense *license_buf, char *klicensee, uint32_t *flags, uint32_t *sku_flag, uint64_t *start_time, uint64_t *expiration_time);