// Copyright 2010            Sven Peter <svenpeter@gmail.com>
// Copyright 2007,2008,2010  Segher Boessenkool  <segher@kernel.crashing.org>
// Licensed under the terms of the GNU GPL, version 2
// http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt

#ifndef TOOLS_H__
#define TOOLS_H__ 1
#include <stdint.h>

#include "types.h"

enum sce_key {
	KEY_LV0 = 0,
	KEY_LV1,
	KEY_LV2,
	KEY_APP,
	KEY_ISO,
	KEY_LDR,
	KEY_PKG,
	KEY_SPP,
    KEY_NPDRM
};

void print_hash(u8 *ptr, u32 len);
void *mmap_file(const char *path);
void memcpy_to_file(const char *fname, u8 *ptr, u64 size);
const char *id2name(u32 id, struct id2name_tbl *t, const char *unk);
void fail(const char *fmt, ...) __attribute__((noreturn));
void decompress(u8 *in, u64 in_len, u8 *out, u64 out_len);
void get_rand(u8 *bfr, u32 size);

int elf_read_hdr(u8 *hdr, struct elf_hdr *h);
void elf_read_phdr(int arch64, u8 *phdr, struct elf_phdr *p);
void elf_read_shdr(int arch64, u8 *shdr, struct elf_shdr *s);
void elf_write_shdr(int arch64, u8 *shdr, struct elf_shdr *s);

void aes256cbc(u8 *key, u8 *iv, u8 *in, u64 len, u8 *out);
void aes256cbc_enc(u8 *key, u8 *iv, u8 *in, u64 len, u8 *out);
void aes128ctr(u8 *key, u8 *iv, u8 *in, u64 len, u8 *out);
void aes128cbc(u8 *key, u8 *iv_in, u8 *in, u64 len, u8 *out);
void aes128cbc_enc(u8 *key, u8 *iv, u8 *in, u64 len, u8 *out);
void aes128(u8 *key, const u8 *in, u8 *out);
void aes128_enc(u8 *key, const u8 *in, u8 *out);

void sha1(u8 *data, u32 len, u8 *digest);
void sha1_hmac(u8 *key, u8 *data, u32 len, u8 *digest);

int key_get(enum sce_key type, const char *suffix, struct key *k);
int key_get_simple(const char *name, u8 *bfr, u32 len);
struct keylist *keys_get(enum sce_key type);

struct rif *rif_get(const char *content_id);
struct actdat *actdat_get(void);

int sce_remove_npdrm(u8 *ptr, struct keylist *klist);
void sce_decrypt_npdrm(u8 *ptr, struct keylist *klist, struct key *klicensee);

int sce_decrypt_header(u8 *ptr, struct keylist *klist);
int sce_encrypt_header(u8 *ptr, struct key *k);
int sce_decrypt_data(u8 *ptr);
int sce_encrypt_data(u8 *ptr);

int ecdsa_get_params(u32 type, u8 *p, u8 *a, u8 *b, u8 *N, u8 *Gx, u8 *Gy);
int ecdsa_set_curve(u32 type);
void ecdsa_set_pub(u8 *Q);
void ecdsa_set_priv(u8 *k);
int ecdsa_verify(u8 *hash, u8 *R, u8 *S);
void ecdsa_sign(u8 *hash, u8 *R, u8 *S);

void bn_copy(u8 *d, u8 *a, u32 n);
int bn_compare(u8 *a, u8 *b, u32 n);
void bn_reduce(u8 *d, u8 *N, u32 n);
void bn_add(u8 *d, u8 *a, u8 *b, u8 *N, u32 n);
void bn_sub(u8 *d, u8 *a, u8 *b, u8 *N, u32 n);
void bn_to_mon(u8 *d, u8 *N, u32 n);
void bn_from_mon(u8 *d, u8 *N, u32 n);
void bn_mon_mul(u8 *d, u8 *a, u8 *b, u8 *N, u32 n);
void bn_mon_inv(u8 *d, u8 *a, u8 *N, u32 n);

#define		round_up(x,n)	(-(-(x) & -(n)))

#define		array_size(x)	(sizeof(x) / sizeof(*(x)))
#endif
