// Copyright 2010            Sven Peter <svenpeter@gmail.com>
// Copyright 2007,2008,2010  Segher Boessenkool  <segher@kernel.crashing.org>
// Licensed under the terms of the GNU GPL, version 2
// http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt

#include <sys/types.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <zlib.h>
#include <dirent.h>
#include <assert.h>

#ifdef WIN32
#include "mingw_mmap.h"
#include <windows.h>
#include <wincrypt.h>
#else
#include <sys/mman.h>
#endif

#include "tools.h"
#include "aes.h"
#include "sha1.h"
#include "common.h"

static struct id2name_tbl t_key2file[] = {
        {KEY_LV0, "lv0"},
        {KEY_LV1, "lv1"},
        {KEY_LV2, "lv2"},
        {KEY_APP, "app"},
        {KEY_ISO, "iso"},
        {KEY_LDR, "ldr"},
        {KEY_PKG, "pkg"},
        {KEY_SPP, "spp"},
        {KEY_NPDRM, "app"},
        {0, NULL}
};

//
// misc
//

void print_hash(u8 *ptr, u32 len)
{
	while(len--)
		printf(" %02x", *ptr++);
}

void *mmap_file(const char *path)
{
	int fd;
	struct stat st;
	void *ptr;

	fd = open(path, O_RDONLY);
	if(fd == -1)
		fail("open %s", path);
	if(fstat(fd, &st) != 0)
		fail("fstat %s", path);

	ptr = mmap(0, st.st_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
	if(ptr==NULL)
		fail("mmap");
	close(fd);

	return ptr;
}

void memcpy_to_file(const char *fname, u8 *ptr, u64 size)
{
	FILE *fp;

	fp = fopen(fname, "wb");
	fwrite(ptr, size, 1, fp);
	fclose(fp);
}

void fail(const char *a, ...)
{
	char msg[1024];
	va_list va;

	va_start(va, a);
	vsnprintf(msg, sizeof msg, a, va);
	fprintf(stderr, "%s\n", msg);
	perror("perror");

	exit(1);
}

void decompress(u8 *in, u64 in_len, u8 *out, u64 out_len)
{
	z_stream s;
	int ret;

	memset(&s, 0, sizeof(s));

	s.zalloc = Z_NULL;
	s.zfree = Z_NULL;
	s.opaque = Z_NULL;

	ret = inflateInit(&s);
	if (ret != Z_OK)
		fail("inflateInit returned %d", ret);

	s.avail_in = in_len;
	s.next_in = in;

	s.avail_out = out_len;
	s.next_out = out;

	ret = inflate(&s, Z_FINISH);
	if (ret != Z_OK && ret != Z_STREAM_END)
		fail("inflate returned %d", ret);

	inflateEnd(&s);
}

const char *id2name(u32 id, struct id2name_tbl *t, const char *unk)
{
	while (t->name != NULL) {
		if (id == t->id)
			return t->name;
		t++;
	}
	return unk;
}

#ifdef WIN32
void get_rand(u8 *bfr, u32 size)
{
	HCRYPTPROV hProv;

	if (!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
		fail("unable to open random");

	if (!CryptGenRandom(hProv, size, bfr))
		fail("unable to read random numbers");

	CryptReleaseContext(hProv, 0);
}
#else
void get_rand(u8 *bfr, u32 size)
{
	FILE *fp;

	fp = fopen("/dev/urandom", "r");
	if (fp == NULL)
		fail("unable to open random");

	if (fread(bfr, size, 1, fp) != 1)
		fail("unable to read random numbers");

	fclose(fp);
}
#endif

//
// ELF helpers
//
int elf_read_hdr(u8 *hdr, struct elf_hdr *h)
{
	int arch64;
	memcpy(h->e_ident, hdr, 16);
	hdr += 16;

	arch64 = h->e_ident[4] == 2;

	h->e_type = be16(hdr);
	hdr += 2;
	h->e_machine = be16(hdr);
	hdr += 2;
	h->e_version = be32(hdr);
	hdr += 4;

	if (arch64) {
		h->e_entry = be64(hdr);
		h->e_phoff = be64(hdr + 8);
		h->e_shoff = be64(hdr + 16);
		hdr += 24;
	} else {
		h->e_entry = be32(hdr);
		h->e_phoff = be32(hdr + 4);
		h->e_shoff = be32(hdr + 8);
		hdr += 12;
	}

	h->e_flags = be32(hdr);
	hdr += 4;

	h->e_ehsize = be16(hdr);
	hdr += 2;
	h->e_phentsize = be16(hdr);
	hdr += 2;
	h->e_phnum = be16(hdr);
	hdr += 2;
	h->e_shentsize = be16(hdr);
	hdr += 2;
	h->e_shnum = be16(hdr);
	hdr += 2;
	h->e_shtrndx = be16(hdr);

	return arch64;
}

void elf_read_phdr(int arch64, u8 *phdr, struct elf_phdr *p)
{
	if (arch64) {
		p->p_type =   be32(phdr + 0);
		p->p_flags =  be32(phdr + 4);
		p->p_off =    be64(phdr + 1*8);
		p->p_vaddr =  be64(phdr + 2*8);
		p->p_paddr =  be64(phdr + 3*8);
		p->p_filesz = be64(phdr + 4*8);
		p->p_memsz =  be64(phdr + 5*8);
		p->p_align =  be64(phdr + 6*8);
	} else {
		p->p_type =   be32(phdr + 0*4);
		p->p_off =    be32(phdr + 1*4);
		p->p_vaddr =  be32(phdr + 2*4);
		p->p_paddr =  be32(phdr + 3*4);
		p->p_filesz = be32(phdr + 4*4);
		p->p_memsz =  be32(phdr + 5*4);
		p->p_flags =  be32(phdr + 6*4);
		p->p_align =  be32(phdr + 7*4);
	}
}

void elf_read_shdr(int arch64, u8 *shdr, struct elf_shdr *s)
{
	if (arch64) {
		s->sh_name =	  be32(shdr + 0*4);
		s->sh_type =	  be32(shdr + 1*4);
		s->sh_flags =	  be64(shdr + 2*4);
		s->sh_addr =	  be64(shdr + 2*4 + 1*8);
		s->sh_offset =	  be64(shdr + 2*4 + 2*8);
		s->sh_size =	  be64(shdr + 2*4 + 3*8);
		s->sh_link =	  be32(shdr + 2*4 + 4*8);
		s->sh_info =	  be32(shdr + 3*4 + 4*8);
		s->sh_addralign = be64(shdr + 4*4 + 4*8);
		s->sh_entsize =   be64(shdr + 4*4 + 5*8);
	} else {
		s->sh_name =	  be32(shdr + 0*4);
		s->sh_type =	  be32(shdr + 1*4);
		s->sh_flags =	  be32(shdr + 2*4);
		s->sh_addr =	  be32(shdr + 3*4);
		s->sh_offset =	  be32(shdr + 4*4);
		s->sh_size =	  be32(shdr + 5*4);
		s->sh_link =	  be32(shdr + 6*4);
		s->sh_info =	  be32(shdr + 7*4);
		s->sh_addralign = be32(shdr + 8*4);
		s->sh_entsize =   be32(shdr + 9*4);
	}
}

void elf_write_shdr(int arch64, u8 *shdr, struct elf_shdr *s)
{
	if (arch64) {
		wbe32(shdr + 0*4, s->sh_name);
		wbe32(shdr + 1*4, s->sh_type);
		wbe64(shdr + 2*4, s->sh_flags);
		wbe64(shdr + 2*4 + 1*8, s->sh_addr);
		wbe64(shdr + 2*4 + 2*8, s->sh_offset);
		wbe64(shdr + 2*4 + 3*8, s->sh_size);
		wbe32(shdr + 2*4 + 4*8, s->sh_link);
		wbe32(shdr + 3*4 + 4*8, s->sh_info);
		wbe64(shdr + 4*4 + 4*8, s->sh_addralign);
		wbe64(shdr + 4*4 + 5*8, s->sh_entsize);
	} else {
		wbe32(shdr + 0*4, s->sh_name);
		wbe32(shdr + 1*4, s->sh_type);
		wbe32(shdr + 2*4, s->sh_flags);
		wbe32(shdr + 3*4, s->sh_addr);
		wbe32(shdr + 4*4, s->sh_offset);
		wbe32(shdr + 5*4, s->sh_size);
		wbe32(shdr + 6*4, s->sh_link);
		wbe32(shdr + 7*4, s->sh_info);
		wbe32(shdr + 8*4, s->sh_addralign);
		wbe32(shdr + 9*4, s->sh_entsize);
	}
}

//
// crypto
//
void aes256cbc(u8 *key, u8 *iv_in, u8 *in, u64 len, u8 *out)
{
	AES_KEY k;
	u32 i;
	u8 tmp[16];
	u8 iv[16];

	memcpy(iv, iv_in, 16);
	memset(&k, 0, sizeof k);
	AES_set_decrypt_key(key, 256, &k);

	while (len > 0) {
		memcpy(tmp, in, 16);
		AES_decrypt(in, out, &k);

		for (i = 0; i < 16; i++)
			out[i] ^= iv[i];

		memcpy(iv, tmp, 16);

		out += 16;
		in += 16;
		len -= 16;

	}
}


void aes256cbc_enc(u8 *key, u8 *iv, u8 *in, u64 len, u8 *out)
{
	AES_KEY k;
	u32 i;
	u8 tmp[16];

	memcpy(tmp, iv, 16);
	memset(&k, 0, sizeof k);
	AES_set_encrypt_key(key, 256, &k);

	while (len > 0) {
		for (i = 0; i < 16; i++)
			tmp[i] ^= *in++;

		AES_encrypt(tmp, out, &k);
		memcpy(tmp, out, 16);

		out += 16;
		len -= 16;
	}
}


void aes128cbc(u8 *key, u8 *iv_in, u8 *in, u64 len, u8 *out)
{
	AES_KEY k;
	u32 i;
	u8 tmp[16];
	u8 iv[16];

	memcpy(iv, iv_in, 16);
	memset(&k, 0, sizeof k);
	AES_set_decrypt_key(key, 128, &k);

	while (len > 0) {
		memcpy(tmp, in, 16);
		AES_decrypt(in, out, &k);

		for (i = 0; i < 16; i++)
			out[i] ^= iv[i];

		memcpy(iv, tmp, 16);

		out += 16;
		in += 16;
		len -= 16;

	}
}

void aes128cbc_enc(u8 *key, u8 *iv, u8 *in, u64 len, u8 *out)
{
	AES_KEY k;
	u32 i;
	u8 tmp[16];

	memcpy(tmp, iv, 16);
	memset(&k, 0, sizeof k);
	AES_set_encrypt_key(key, 128, &k);

	while (len > 0) {
		for (i = 0; i < 16; i++)
			tmp[i] ^= *in++;

		AES_encrypt(tmp, out, &k);
		memcpy(tmp, out, 16);

		out += 16;
		len -= 16;
	}
}

void aes128ctr(u8 *key, u8 *iv, u8 *in, u64 len, u8 *out)
{
	AES_KEY k;
	u32 i;
	u8 ctr[16];
	u64 tmp;

	memset(ctr, 0, 16);
	memset(&k, 0, sizeof k);

	AES_set_encrypt_key(key, 128, &k);

	for (i = 0; i < len; i++) {
		if ((i & 0xf) == 0) {
			AES_encrypt(iv, ctr, &k);

			// increase nonce
			tmp = be64(iv + 8) + 1;
			wbe64(iv + 8, tmp);
			if (tmp == 0)
				wbe64(iv, be64(iv) + 1);
		}
		*out++ = *in++ ^ ctr[i & 0x0f];
	}
}

void aes128(u8 *key, const u8 *in, u8 *out) {
    AES_KEY k;

    assert(!AES_set_decrypt_key(key, 128, &k));
    AES_decrypt(in, out, &k);
}

void aes128_enc(u8 *key, const u8 *in, u8 *out) {
    AES_KEY k;

    assert(!AES_set_encrypt_key(key, 128, &k));
    AES_encrypt(in, out, &k);
}



// FIXME: use a non-broken sha1.c *sigh*
static void sha1_fixup(struct SHA1Context *ctx, u8 *digest)
{
	u32 i;

	for(i = 0; i < 5; i++) {
		*digest++ = ctx->Message_Digest[i] >> 24 & 0xff;
		*digest++ = ctx->Message_Digest[i] >> 16 & 0xff;
		*digest++ = ctx->Message_Digest[i] >> 8 & 0xff;
		*digest++ = ctx->Message_Digest[i] & 0xff;
	}
}

void sha1(u8 *data, u32 len, u8 *digest)
{
	struct SHA1Context ctx;

	SHA1Reset(&ctx);
	SHA1Input(&ctx, data, len);
	SHA1Result(&ctx);

	sha1_fixup(&ctx, digest);
}

void sha1_hmac(u8 *key, u8 *data, u32 len, u8 *digest)
{
	struct SHA1Context ctx;
	u32 i;
	u8 ipad[0x40];
	u8 tmp[0x40 + 0x14]; // opad + hash(ipad + message)

	SHA1Reset(&ctx);

	for (i = 0; i < sizeof ipad; i++) {
		tmp[i] = key[i] ^ 0x5c; // opad
		ipad[i] = key[i] ^ 0x36;
	}

	SHA1Input(&ctx, ipad, sizeof ipad);
	SHA1Input(&ctx, data, len);
	SHA1Result(&ctx);

	sha1_fixup(&ctx, tmp + 0x40);

	sha1(tmp, sizeof tmp, digest);

}

static int key_build_path(char *ptr)
{
	char *home = NULL;
	char *dir = NULL;

	memset(ptr, 0, 256);

	dir = getenv("PS4_KEYS");
	if (dir != NULL) {
		strncpy(ptr, dir, 256);
		return 0;
	}

#ifdef WIN32
        home = getenv("USERPROFILE");
#else
	home = getenv("HOME");
#endif
	if (home == NULL) {
          snprintf (ptr, 256, "ps4keys");
        } else {
#ifdef WIN32
          snprintf(ptr, 256, "%s\\ps4keys\\", home);
#else
          snprintf(ptr, 256, "%s/.ps4/", home);
#endif
        }

	return 0;
}

static int key_read(const char *path, u32 len, u8 *dst)
{
	FILE *fp = NULL;
	u32 read;
	int ret = -1;

	fp = fopen(path, "rb");
	if (fp == NULL)
		goto fail;

	read = fread(dst, len, 1, fp);

	if (read != 1)
		goto fail;

	ret = 0;

fail:
	if (fp != NULL)
		fclose(fp);

	return ret;
}

struct keylist *keys_get(enum sce_key type)
{
	const char *name = NULL;
	char base[256];
	char path[256];
	void *tmp = NULL;
	char *id;
	DIR *dp;
	struct dirent *dent;
	struct keylist *klist;
	u8 bfr[4];

	klist = malloc(sizeof *klist);
	if (klist == NULL)
		goto fail;

	memset(klist, 0, sizeof *klist);

	name = id2name(type, t_key2file, NULL);
	if (name == NULL)
		goto fail;

	if (key_build_path(base) < 0)
		goto fail;

	dp = opendir(base);
	if (dp == NULL)
		goto fail;

	while ((dent = readdir(dp)) != NULL) {
		if (strncmp(dent->d_name, name, strlen(name)) == 0 &&
		    strstr(dent->d_name, "key") != NULL) {
			tmp = realloc(klist->keys, (klist->n + 1) * sizeof(struct key));
			if (tmp == NULL)
				goto fail;

			id = strrchr(dent->d_name, '-');
			if (id != NULL)
				id++;

			klist->keys = tmp;
			memset(&klist->keys[klist->n], 0, sizeof(struct key));

			snprintf(path, sizeof path, "%s/%s-key-%s", base, name, id);
			if (key_read(path, 32, klist->keys[klist->n].key) != 0) {
				printf("  key file:   %s (ERROR)\n", path);
			}

			snprintf(path, sizeof path, "%s/%s-iv-%s", base, name, id);
			if (key_read(path, 16, klist->keys[klist->n].iv) != 0) {
				printf("  iv file:    %s (ERROR)\n", path);
			}

			klist->keys[klist->n].pub_avail = -1;
			klist->keys[klist->n].priv_avail = -1;

			snprintf(path, sizeof path, "%s/%s-pub-%s", base, name, id);
			if (key_read(path, 40, klist->keys[klist->n].pub) == 0) {
				snprintf(path, sizeof path, "%s/%s-ctype-%s", base, name, id);
				key_read(path, 4, bfr);

				klist->keys[klist->n].pub_avail = 1;
				klist->keys[klist->n].ctype = be32(bfr);
			} else {
				printf("  pub file:   %s (ERROR)\n", path);
			}

			snprintf(path, sizeof path, "%s/%s-priv-%s", base, name, id);
			if (key_read(path, 21, klist->keys[klist->n].priv) == 0) {
				klist->keys[klist->n].priv_avail = 1;
			} else {
				printf("  priv file:  %s (ERROR)\n", path);
			}


			klist->n++;
		}
	}

    if (type == KEY_NPDRM) {
        klist->idps = calloc(sizeof(struct key), 1);
        if (klist->idps == NULL)
            goto fail;
        snprintf(path, sizeof path, "%s/idps", base);
        if (key_read(path, 16, klist->idps->key) != 0) {
            printf("  key file:   %s (ERROR)\n", path);
        }

        klist->klic = calloc(sizeof(struct key), 1);
        if (klist->klic == NULL)
            goto fail;
        snprintf(path, sizeof path, "%s/klic-key", base);
        if (key_read(path, 16, klist->klic->key) != 0) {
            printf("  key file:   %s (ERROR)\n", path);
        }

        klist->rif = calloc(sizeof(struct key), 1);
        if (klist->rif == NULL)
            goto fail;
        snprintf(path, sizeof path, "%s/rif-key", base);
        if (key_read(path, 16, klist->rif->key) != 0) {
            printf("  key file:   %s (ERROR)\n", path);
        }

        klist->npdrm_const = calloc(sizeof(struct key), 1);
        if (klist->npdrm_const == NULL)
            goto fail;
        snprintf(path, sizeof path, "%s/npdrm-const", base);
        if (key_read(path, 16, klist->npdrm_const->key) != 0) {
            printf("  key file:   %s (ERROR)\n", path);
        }

        klist->free_klicensee = calloc(sizeof(struct key), 1);
        if (klist->free_klicensee == NULL)
            goto fail;
        snprintf(path, sizeof path, "%s/free_klicensee-key", base);
        if (key_read(path, 16, klist->free_klicensee->key) != 0) {
            printf("  key file:   %s (ERROR)\n", path);
        }
    }

	return klist;

fail:
	if (klist != NULL) {
		if (klist->keys != NULL)
			free(klist->keys);
		free(klist);
	}
	klist = NULL;

	return NULL;
}

int key_get_simple(const char *name, u8 *bfr, u32 len)
{
	char base[256];
	char path[256];

	if (key_build_path(base) < 0)
		return -1;

	snprintf(path, sizeof path, "%s/%s", base, name);
	if (key_read(path, len, bfr) < 0)
		return -1;

	return 0;
}

int key_get(enum sce_key type, const char *suffix, struct key *k)
{
	const char *name = "";
	const char *rev = "";
	char base[256];
	char path[256];
	u8 tmp[4];

	if ( strncmp( suffix, "retail", strlen( suffix ) ) == 0 ) {
		rev = "retail";
	} else if ( atoi( suffix ) <= 92 ) {
		suffix = "092";
		rev = "0x00";
	} else if ( atoi( suffix ) <= 331 ) {
		suffix = "315";
		rev = "0x01";
	} else if ( atoi( suffix ) <= 342 ) {
		suffix = "341";
		rev = "0x04";
	} else if ( atoi( suffix ) <= 350 ) {
		suffix = "350";
		rev = "0x07";
	} else if ( atoi( suffix ) <= 355 ) {
		suffix = "355";
		rev = "0x0a";
	} else if ( atoi( suffix ) <= 356 ) {
		suffix = "356";
		rev = "0x0d";
	}
	printf("  file suffix:    %s (rev %s)\n", suffix, rev );

	if (key_build_path(base) < 0)
		return -1;

	name = id2name(type, t_key2file, NULL);
	if (name == NULL)
		return -1;

	snprintf(path, sizeof path, "%s/%s-key-%s", base, name, suffix);
	if (key_read(path, 32, k->key) < 0) {
		printf("  key file:   %s (ERROR)\n", path);
		return -1;
	}

	snprintf(path, sizeof path, "%s/%s-iv-%s", base, name, suffix);
	if (key_read(path, 16, k->iv) < 0) {
		printf("  iv file:    %s (ERROR)\n", path);
		return -1;
	}

	k->pub_avail = k->priv_avail = 1;

	snprintf(path, sizeof path, "%s/%s-ctype-%s", base, name, suffix);
	if (key_read(path, 4, tmp) < 0) {
		k->pub_avail = k->priv_avail = -1;
		printf("  ctype file: %s (ERROR)\n", path);
		return 0;
	}

	k->ctype = be32(tmp);

	snprintf(path, sizeof path, "%s/%s-pub-%s", base, name, suffix);
	if (key_read(path, 40, k->pub) < 0) {
		printf("  pub file:   %s (ERROR)\n", path);
		k->pub_avail = -1;
	}

	snprintf(path, sizeof path, "%s/%s-priv-%s", base, name, suffix);
	if (key_read(path, 21, k->priv) < 0) {
		printf("  priv file:  %s (ERROR)\n", path);
		k->priv_avail = -1;
	}

	return 0;
}

struct rif *rif_get(const char *content_id) {
	char base[256];
	char path[256];
    struct rif *rif;
    FILE *fp;
    u32 read;
	
    rif = malloc(sizeof(struct rif));
	if (rif == NULL)
		goto fail;

	if (key_build_path(base) < 0)
		goto fail;

    snprintf(path, sizeof path, "%s/exdata/%s.rif", base, content_id);

    fp = fopen(path, "rb");
    if (fp == NULL)
        goto fail;

    read = fread(rif, sizeof(struct rif), 1, fp);

    if (read != 1)
        goto fail;

    return rif;

fail:
	if (rif != NULL) {
		free(rif);
	}

	return NULL; 
}

struct actdat *actdat_get(void) {
	char base[256];
	char path[256];
    struct actdat *actdat;
    FILE *fp;
    u32 read;
	
    actdat = malloc(sizeof(struct actdat));
	if (actdat == NULL)
		goto fail;

	if (key_build_path(base) < 0)
		goto fail;

    snprintf(path, sizeof path, "%s/exdata/act.dat", base);

    fp = fopen(path, "rb");
    if (fp == NULL)
        goto fail;

    read = fread(actdat, sizeof(struct actdat), 1, fp);

    if (read != 1)
        goto fail;

    return actdat;

fail:
	if (actdat != NULL) {
		free(actdat);
	}

	return NULL; 
}

static void memcpy_inv(u8 *dst, u8 *src, u32 len)
{
	u32 j;

	for (j = 0; j < len; j++)
		dst[j] = ~src[j];
}

int ecdsa_get_params(u32 type, u8 *p, u8 *a, u8 *b, u8 *N, u8 *Gx, u8 *Gy)
{
	static u8 tbl[64 * 121];
	char path[256];
	u32 offset;

	if (type >= 64)
		return -1;

	if (key_build_path(path) < 0)
		return -1;

	strncat(path, "/curves", sizeof path);

	if (key_read(path, sizeof tbl, tbl) < 0)
		return -1;

	offset = type * 121;

	memcpy_inv(p, tbl + offset + 0, 20);
	memcpy_inv(a, tbl + offset + 20, 20);
	memcpy_inv(b, tbl + offset + 40, 20);
	memcpy_inv(N, tbl + offset + 60, 21);
	memcpy_inv(Gx, tbl + offset + 81, 20);
	memcpy_inv(Gy, tbl + offset + 101, 20);

	return 0;
}

int sce_remove_npdrm(u8 *ptr, struct keylist *klist)
{
    u64 ctrl_offset;
    u64 ctrl_size;
    u32 block_type;
    u32 block_size;
    u32 license_type;
    char content_id[0x31] = {'\0'};
    struct rif *rif;
    struct actdat *actdat;
    u8 enc_const[0x10];
    u8 dec_actdat[0x10];
    struct key klicensee;
    u64 i;

    ctrl_offset = be64(ptr + 0x58);
    ctrl_size = be64(ptr + 0x60);

    for (i = 0; i < ctrl_size; ) {
        block_type = be32(ptr + ctrl_offset + i);
        block_size = be32(ptr + ctrl_offset + i + 0x4);

        if (block_type == 3) {
            license_type = be32(ptr + ctrl_offset + i + 0x18);
            switch (license_type) {
                case 1:
                    // cant decrypt network stuff
                    return -1;
                case 2:
                    memcpy(content_id, ptr + ctrl_offset + i + 0x20, 0x30);
                    rif = rif_get(content_id);
                    if (rif == NULL) {
                        return -1;
                    }
                    aes128(klist->rif->key, rif->padding, rif->padding);
                    aes128_enc(klist->idps->key, klist->npdrm_const->key, enc_const);
                    actdat = actdat_get();
                    if (actdat == NULL) {
                        return -1;
                    }
                    aes128(enc_const, &actdat->keyTable[swap32(rif->actDatIndex)*0x10], dec_actdat);
                    aes128(dec_actdat, rif->key, klicensee.key);
                    sce_decrypt_npdrm(ptr, klist, &klicensee);
                    return 1;
                case 3:
                    sce_decrypt_npdrm(ptr, klist, klist->free_klicensee);
                    return 1;
            }
        }

        i += block_size;
    }

    return 0;
}

void sce_decrypt_npdrm(u8 *ptr, struct keylist *klist, struct key *klicensee)
{
	u32 meta_offset;
    struct key d_klic;

	meta_offset = be32(ptr + 0x0c);

    // iv is 0
    memset(&d_klic, 0, sizeof(struct key));
    aes128(klist->klic->key, klicensee->key, d_klic.key);

    aes128cbc(d_klic.key, d_klic.iv, ptr + meta_offset + 0x20, 0x40, ptr + meta_offset + 0x20);
}


int sce_decrypt_header(u8 *ptr, struct keylist *klist)
{
	u32 meta_offset;
	u32 meta_len;
	u64 header_len;
	u32 i, j;
	u8 tmp[0x40];
	int success = 0;


	meta_offset = be32(ptr + 0x0c);
	header_len  = be64(ptr + 0x10);

	for (i = 0; i < klist->n; i++) {
		aes256cbc(klist->keys[i].key,
			  klist->keys[i].iv,
			  ptr + meta_offset + 0x20,
			  0x40,
			  tmp);

		success = 1;
		for (j = 0x10; j < (0x10 + 0x10); j++)
			if (tmp[j] != 0)
				success = 0;

		for (j = 0x30; j < (0x30 + 0x10); j++)
			if (tmp[j] != 0)
			       success = 0;

		if (success == 1) {
			memcpy(ptr + meta_offset + 0x20, tmp, 0x40);
			break;
		}
	}

	if (success != 1)
		return -1;

	memcpy(tmp, ptr + meta_offset + 0x40, 0x10);
	aes128ctr(ptr + meta_offset + 0x20,
		  tmp,
		  ptr + meta_offset + 0x60,
		  0x20,
		  ptr + meta_offset + 0x60);

	meta_len = header_len - meta_offset;

	aes128ctr(ptr + meta_offset + 0x20,
		  tmp,
		  ptr + meta_offset + 0x80,
		  meta_len - 0x80,
		  ptr + meta_offset + 0x80);

	return i;
}

int sce_encrypt_header(u8 *ptr, struct key *k)
{
	u32 meta_offset;
	u32 meta_len;
	u64 header_len;
	u8 iv[16];

	meta_offset = be32(ptr + 0x0c);
	header_len  = be64(ptr + 0x10);
	meta_len = header_len - meta_offset;

	memcpy(iv, ptr + meta_offset + 0x40, 0x10);
	aes128ctr(ptr + meta_offset + 0x20,
		  iv,
		  ptr + meta_offset + 0x60,
		  meta_len - 0x60,
		  ptr + meta_offset + 0x60);

	aes256cbc_enc(k->key, k->iv,
	              ptr + meta_offset + 0x20,
		      0x40,
		      ptr + meta_offset + 0x20);


	return 0;
}

int sce_decrypt_data(u8 *ptr)
{
	u64 meta_offset;
	u32 meta_len;
	u32 meta_n_hdr;
	u64 header_len;
	u32 i;

	u64 offset;
	u64 size;
	u32 keyid;
	u32 ivid;
	u8 *tmp;

	u8 iv[16];

	meta_offset = be32(ptr + 0x0c);
	header_len  = be64(ptr + 0x10);
	meta_len = header_len - meta_offset;
	meta_n_hdr = be32(ptr + meta_offset + 0x60 + 0xc);

	for (i = 0; i < meta_n_hdr; i++) {
		tmp = ptr + meta_offset + 0x80 + 0x30*i;
		offset = be64(tmp);
		size = be64(tmp + 8);
		keyid = be32(tmp + 0x24);
		ivid = be32(tmp + 0x28);

		if (keyid == 0xffffffff || ivid == 0xffffffff)
			continue;

		memcpy(iv, ptr + meta_offset + 0x80 + 0x30 * meta_n_hdr + ivid * 0x10, 0x10);
		aes128ctr(ptr + meta_offset + 0x80 + 0x30 * meta_n_hdr + keyid * 0x10,
		          iv,
 		          ptr + offset,
			  size,
			  ptr + offset);
	}

	return 0;
}

int sce_encrypt_data(u8 *ptr)
{
	return sce_decrypt_data(ptr);
}
