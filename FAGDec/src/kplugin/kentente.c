#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/syslimits.h>

#include <vitasdkkern.h>
#include <taihen.h>

#include "../common/elf.h"
#include "../common/self.h"
#include "kernel.h"
#include "context_130_base_sceshell.h"
#include "context_130_base_kernel.h"
#include "kentente.h"

#define printf ksceDebugPrintf
static char log[128];
static int status = ENTENTE_DONE;

void _log(int stat, const char *text, ...) {
	va_list args;
    va_start(args, text);
    vsprintf(log, text, args);
    va_end(args);
	status = stat;
}

void kuEntenteGetLogs(char *dest) {
	int state;
	ENTER_SYSCALL(state);
	ksceKernelMemcpyKernelToUser((uintptr_t)dest, log, sizeof(log));
	status = ENTENTE_NONE;
	EXIT_SYSCALL(state);
}

int	kuEntenteStatus() {
	int state, ret;
	ENTER_SYSCALL(state);
	ret = status;
	EXIT_SYSCALL(state);
	return ret;
}

int decrypt_self(char *path, char *outpath, char *klicensee, int path_id, int usecdram, int self_type, int self_auth) {
	int ctx, ret;
	SceUID fd = 0, wfd = 0;
	char *data_buf = NULL, *data_buf_aligned;
	char *hdr_buf = NULL, *hdr_buf_aligned;
	
	// set up F00D context
	if ((ret = sceSblAuthMgrSmStartForKernel(&ctx)) < 0)
		return 1;
	
	// set up SceSblSmCommContext130
	SceSblSmCommContext130 *context_130 = (SceSblSmCommContext130 *)sceSysmemMallocForKernel(0x130);
	if (context_130 == NULL)
		goto fail;
	memset(context_130, 0, 0x130);
	if (klicensee == NULL) {
		memcpy(context_130, CONTEXT_130_BASE_KERNEL, CONTEXT_130_BASE_KERNEL_SIZE);
		context_130->self_type = self_type;
	} else {
		memcpy(context_130, CONTEXT_130_BASE_SCESHELL, CONTEXT_130_BASE_SCESHELL_SIZE);
		memcpy(context_130->called_self_auth_info.klicensee, klicensee, 0x10);
	}
	
	// Set path_id according to the normal path
	if (path_id >= 0)
		context_130->path_id = path_id;
	else if ((ret = sceIoGetPathIdExForDriver(KERNEL_PID, path, 1, (int *)&(context_130->path_id))) < 0) 
		goto fail;
	
	hdr_buf = sceSysmemMallocForKernel(HEADER_LEN + 63);
	hdr_buf_aligned = (char *)(((int)hdr_buf + 63) & 0xFFFFFFC0);
	
	if (hdr_buf == NULL)
		goto fail;
	
	if ((fd = ksceIoOpen(path, 1, 0)) < 0)
		goto fail;
	ret = ksceIoRead(fd, hdr_buf_aligned, HEADER_LEN);
	
	SCE_header *shdr  = (SCE_header *)(hdr_buf_aligned);
	
	if (shdr->header_len > HEADER_LEN)
		goto fail;
	
	// set up SBL decryption for this SELF.
	if ((ret = sceSblAuthMgrAuthHeaderForKernel(ctx, hdr_buf_aligned, shdr->header_len, context_130)) < 0)
		goto fail;
	char *ending = strchr(outpath, 0);
	Elf32_Ehdr *ehdr = (Elf32_Ehdr *)(hdr_buf_aligned + shdr->elf_offset);
	Elf32_Phdr *phdrs = (Elf32_Phdr *)(hdr_buf_aligned + shdr->phdr_offset);
	segment_info *segs = (segment_info*)(hdr_buf_aligned + shdr->section_info_offset);
	
	data_buf = sceSysmemMallocForKernel(0x10000 + 63);
	data_buf_aligned = (char *)(((int)data_buf + 63) & 0xFFFFFFC0);
	if (data_buf == NULL)
		goto fail;
	void *pgr_buf;
	
	// decrypt sections
	SceUID blkid = 0;
	
	for (int i=0; i < ehdr->e_phnum; i++) {
		_log(ENTENTE_UPDATE, "Decrypting segment: %i", i);
		if (segs[i].encryption == 1) {
			*ending = 0;
			snprintf(outpath, PATH_MAX, "%s.temp%i",outpath, i);
			ksceIoClose(wfd);
			
			wfd = ksceIoOpen(outpath, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 6);	
			if (wfd < 0)
				break;
			size_t aligned_size = (phdrs[i].p_filesz + 4095) & 0xFFFFF000;
			
			// set up read buffer
			if (blkid)
				ksceKernelFreeMemBlock(blkid);	
			
			if (usecdram)
				blkid = ksceKernelAllocMemBlock("self_decrypt_buffer", 0x40404006, 0x4000000, NULL);
			else
				blkid = ksceKernelAllocMemBlock("self_decrypt_buffer", 0x1020D006, aligned_size, NULL);
			
			if ((ret = ksceKernelGetMemBlockBase(blkid, &pgr_buf)) < 0)
				break;

			// setup buffer for output
			if ((ret = sceSblAuthMgrLoadSelfSegmentForKernel(ctx, i, segs[i].length, pgr_buf, phdrs[i].p_filesz)) < 0)
				break;
			
			if ((ret = ksceIoLseek(fd, segs[i].offset, SCE_SEEK_SET)) < 0)
				break;
			
			size_t length = segs[i].length;
			int to_read = length > 0x10000 ? 0x10000 : length;
			size_t num_read, off = 0;
			
			while (length > 0 && (num_read = ksceIoRead(fd, data_buf_aligned+off, to_read)) > 0) {
				off += num_read;
				length -= num_read;
				if (num_read < to_read) {
					to_read -= num_read;
					continue;
				}
				ret = sceSblAuthMgrLoadSelfBlockForKernel(ctx, data_buf_aligned, off); // decrypt buffer
				if (ret < 0)
					_log(ENTENTE_UPDATE, "!!! ERROR !!!\n");			
				ksceIoWrite(wfd, data_buf_aligned, to_read);
				off = 0;
				to_read = length > 0x10000 ? 0x10000 : length;
			}
			// write buffer
			off = 0;
			while ((off += ksceIoWrite(wfd, pgr_buf+off, phdrs[i].p_filesz-off)) < phdrs[i].p_filesz);			
			
			
			ksceIoClose(wfd);
			strncpy(outpath + PATH_MAX/2, outpath, PATH_MAX/2);
			*ending = 0;
			snprintf(outpath, PATH_MAX/2,  "%s.seg%i",outpath, i);
			ksceKernelDelayThread(10 * 1000);
			ksceIoRename(outpath + PATH_MAX/2, outpath);
			_log(ENTENTE_DONESEG, "Segment decrypted: %i", i);
		}
		
	}
	*ending = 0;
	if (blkid)
		ksceKernelFreeMemBlock(blkid);
	
	if (self_auth) {
		snprintf(outpath, PATH_MAX, "%s.auth",outpath);
		wfd = ksceIoOpen(outpath, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 6);
		memset(context_130->called_self_auth_info.klicensee, 0, 0x10);
		ksceIoWrite(wfd, &context_130->called_self_auth_info, sizeof(context_130->called_self_auth_info));
		ksceIoClose(wfd);
		*ending = 0;
	}
	
fail:
	_log(ENTENTE_DONE, "kuEntente Returned: %x", ret);
	sceSblAuthMgrSmFinalizeForKernel(ctx);
	if (fd)
		ksceIoClose(fd);
	if (context_130)
		sceSysmemFreeForKernel(context_130);
	if (hdr_buf)
		sceSysmemFreeForKernel(hdr_buf);
	if (data_buf)
		sceSysmemFreeForKernel(data_buf);
	return 1;
}

int getlicensee_rif(char *path, char *klicensee) {
	int res, fd;
	char *klicensee_buf = NULL, *klicensee_buf_aligned;
	klicensee_buf = sceSysmemMallocForKernel(0x200 + 63);
	klicensee_buf_aligned = (char *)(((int)klicensee_buf + 63) & 0xFFFFFFC0);
	memset(klicensee, 0, 0x10);
	fd = ksceIoOpen(path, SCE_O_RDONLY, 0);
	ksceIoRead(fd, klicensee_buf_aligned, 0x200);
	ksceIoClose(fd);
	res = ksceNpDrmGetRifVitaKey((SceNpDrmLicense *)klicensee_buf_aligned, klicensee, NULL, NULL, NULL, NULL);
	sceSysmemFreeForKernel(klicensee_buf);
	return res;
}
static ententeParams param;

int decrypt_thread(SceSize args, void *argp) {
	char klicensee[0x10];
	char *outpath_buf = param.outpath, *outpath_buf_aligned;
	char *inpath_buf = param.rifpath, *inpath_buf_aligned;
	char *path_buf = param.path, *path_buf_aligned;
	outpath_buf_aligned = (char *)(((int)outpath_buf + 63) & 0xFFFFFFC0);
	path_buf_aligned = (char *)(((int)path_buf + 63) & 0xFFFFFFC0);
	
	char *klicensee_ptr = klicensee;
	_log(ENTENTE_UPDATE, "Starting decryption...");
	if (inpath_buf!=NULL) {
		inpath_buf_aligned = (char *)(((int)inpath_buf + 63) & 0xFFFFFFC0);	
		_log(ENTENTE_UPDATE, "Getting klicense from rif");
		getlicensee_rif(inpath_buf_aligned, klicensee);	
		sceSysmemFreeForKernel(inpath_buf);
	} else
		klicensee_ptr = NULL;
	decrypt_self(path_buf_aligned, outpath_buf_aligned, klicensee_ptr, param.path_id, param.usecdram, param.self_type, param.self_auth);
	sceSysmemFreeForKernel(outpath_buf);
	sceSysmemFreeForKernel(path_buf);
	ksceKernelExitDeleteThread(0);
	return 0;
}

int kuEntenteDecryptSelf(ententeParams *u_param) {
	int state, res, tid;
	char *outpath_buf = NULL, *outpath_buf_aligned;
	char *inpath_buf = NULL, *inpath_buf_aligned;
	char *path_buf = NULL, *path_buf_aligned;
	
	ENTER_SYSCALL(state);

	outpath_buf = sceSysmemMallocForKernel(PATH_MAX + 63);
	outpath_buf_aligned = (char *)(((int)outpath_buf + 63) & 0xFFFFFFC0);
	
	inpath_buf = sceSysmemMallocForKernel(PATH_MAX + 63);
	inpath_buf_aligned = (char *)(((int)inpath_buf + 63) & 0xFFFFFFC0);	
	
	path_buf = sceSysmemMallocForKernel(PATH_MAX + 63);
	path_buf_aligned = (char *)(((int)path_buf + 63) & 0xFFFFFFC0);
	ksceKernelMemcpyUserToKernel(&param, (uintptr_t)u_param, sizeof(ententeParams));
	ksceKernelStrncpyUserToKernel(path_buf_aligned, (uintptr_t)param.path, PATH_MAX);
	ksceKernelStrncpyUserToKernel(outpath_buf_aligned, (uintptr_t)param.outpath, PATH_MAX);
	if (param.rifpath!=NULL)
		ksceKernelStrncpyUserToKernel(inpath_buf_aligned, (uintptr_t)param.rifpath, PATH_MAX);
	else {
		sceSysmemFreeForKernel(inpath_buf);
		inpath_buf = NULL;
	}
	param.path = path_buf;
	param.outpath = outpath_buf;
	param.rifpath = inpath_buf;
	tid = ksceKernelCreateThread("kuEntente_thread", (SceKernelThreadEntry)decrypt_thread, 64, 0x1000, 0, 0, NULL);
	res = ksceKernelStartThread(tid, 0, NULL);

	EXIT_SYSCALL(state);
	return res;
}

void _start() __attribute__ ((weak, alias ("module_start")));
int module_start(SceSize argc, const void *args) {
	module_get_export_func(KERNEL_PID, "SceSblAuthMgr", 0x7ABF5135, 0xA9CD2A09, (uintptr_t*)&sceSblAuthMgrSmStartForKernel);
	module_get_export_func(KERNEL_PID, "SceSblAuthMgr", 0x7ABF5135, 0xF3411881, (uintptr_t*)&sceSblAuthMgrAuthHeaderForKernel);
	module_get_export_func(KERNEL_PID, "SceSblAuthMgr", 0x7ABF5135, 0x026ACBAD, (uintptr_t*)&sceSblAuthMgrSmFinalizeForKernel);
	module_get_export_func(KERNEL_PID, "SceSblAuthMgr", 0x7ABF5135, 0x89CCDA2C, (uintptr_t*)&sceSblAuthMgrLoadSelfSegmentForKernel);
	module_get_export_func(KERNEL_PID, "SceSblAuthMgr", 0x7ABF5135, 0xBC422443, (uintptr_t*)&sceSblAuthMgrLoadSelfBlockForKernel);
	
	if (module_get_export_func(KERNEL_PID, "SceSysmem", 0x63A519E5, 0xC0A4D2F3, (uintptr_t*)&sceSysmemMallocForKernel) < 0)
		module_get_export_func(KERNEL_PID, "SceSysmem", TAI_ANY_LIBRARY, 0x85571907, (uintptr_t*)&sceSysmemMallocForKernel);
	if (module_get_export_func(KERNEL_PID, "SceSysmem", 0x63A519E5, 0xABAB0FAB, (uintptr_t*)&sceSysmemFreeForKernel) < 0)
		module_get_export_func(KERNEL_PID, "SceSysmem", TAI_ANY_LIBRARY, 0x4233C16D, (uintptr_t*)&sceSysmemFreeForKernel);
	
	module_get_export_func(KERNEL_PID, "SceIofilemgr", 0x40FD29C7, 0x9C220246, (uintptr_t*)&sceIoGetPathIdExForDriver);

	status = ENTENTE_DONE;
	return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize argc, const void *args) {
	return SCE_KERNEL_STOP_SUCCESS;
}