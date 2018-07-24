#include <stdio.h>
#include <stdlib.h>
#include <zlib.h>
#include <sys/syslimits.h>

#include <vitasdk.h>
#include <taihen.h>

#include <userAllied.h>
#include <kentente.h>
#include "menu.h"
#include "../common/elf.h"
#include "../common/self.h"
#include "dump.h"
#include "game.h"
#include "status.h"
#include "utils.h"
#include "sha256.h"
#include "inflate.h"


static Menu *dump_menu = NULL, *current_dump_menu;
static GameEntry *current_game;
static SceUID dumpThread_tid = -1, decompressThread_tid, decompress_flag, populateThread_tid, force_end = 0, uplugin_modid = -1;
static char *current_file, *current_elf, *current_self;
static int dump_type = DUMP_ELF;

#define OUT_FOLDER "ux0:/FAGDec"

#define DECOMPRESS_NEW   0x01
#define DECOMPRESS_DONE  0x02
#define DECOMPRESS_SEGOK 0x04

//int _newlib_heap_size_user = 128 * 1024 * 1024;


int checkSceHeader(const char *file) {
	int res;
	uint32_t self_buffer;
	if((res=ReadFile(file, &self_buffer, sizeof(self_buffer)))>0) {	
		res = (self_buffer == SCE_MAGIC);
	}
	return res; 
	
}



static Menu *parent_menu;
void *dumpModuleEntryBackCB(void *menu, int index) { 
	force_end = 1;
	sceKernelWaitThreadEnd(populateThread_tid, 0, NULL);
	MenuDelete(current_dump_menu);
	return parent_menu;
}

void *dumpModuleEntryCB(void *menu, int index) {
	if(dump_menu->entry_selected && dump_menu->entry_selected->type == MENU_WARNING)
		dump_menu->entry_selected->type = MENU_NONE;
	return menu;
}

void dumpAddEntry(void *menu, int index) {
	MenuEntry *title = NULL;
	if(strcmp(MenuFindType(dump_menu, MENU_DISABLE)->text,current_game->titleid)!=0)
		title = MenuAddEntry(dump_menu, MENU_SUBTITLE, current_game->titleid, NULL);
	printf("index %i\n", index);
	MenuAddEntry(dump_menu, MENU_NORMAL, MenuFindEntry(menu,index)->text, dumpModuleEntryCB);
	if(dump_menu->entry_selected->index == dump_menu->entry_end->prev->index || (title && dump_menu->entry_selected->index == title->prev->index))
		dump_menu->entry_selected = dump_menu->entry_end;
}
void *dumpModuleMenuEntryCB(void *menu, int index) {
	printf("adding\n");
	if(menu!=NULL)
		dumpAddEntry(menu, ((Menu*)menu)->entry_selected->index);
	return menu;
}

void *dumpAllCB(void *menu, int index) {
	MenuEntry *menu_entry = ((Menu*)menu)->entry_selected->next;
	while(menu_entry != NULL) {
		if(menu_entry->type == MENU_NONE)
			 dumpAddEntry(menu, menu_entry->index);
		menu_entry = menu_entry->next;
	}
	return menu;
}

int hasEndSlash(const char *path) {
	return path[strlen(path) - 1] == '/';
}

//vitashell code again
int dumpParseFolder(Menu *menu, const char *path) {
	if(force_end)
		return -1;
	SceUID dfd = sceIoDopen(path);
	if (dfd >= 0) {
		int res = 0;

		do {
			SceIoDirent dir;
			memset(&dir, 0, sizeof(SceIoDirent));
			res = sceIoDread(dfd, &dir);
			if (res > 0) {
				if(force_end)
					return -1;
				char *new_path = malloc(strlen(path) + strlen(dir.d_name) + 2);
				snprintf(new_path, PATH_MAX - 1, "%s%s%s", path, hasEndSlash(path) ? "" : "/", dir.d_name);
				if (SCE_S_ISDIR(dir.d_stat.st_mode)) {
					if(dumpParseFolder(menu, new_path)<0)
						break;
				} else {
					if(checkSceHeader(new_path)==1 && !force_end && menu)
						MenuAddEntry(menu, MENU_NORMAL, new_path, dumpModuleMenuEntryCB);
				}
				free(new_path);
			}
		} while (res > 0);

		sceIoDclose(dfd);
	} else {
		if(checkSceHeader(path)==1 && !force_end && menu)
			MenuAddEntry(menu, MENU_NORMAL, path, dumpModuleMenuEntryCB);
	}

	return 1;
}

int gamePfsDecrypt(const char *titleid, char *mount_point) {
	char current_path[PATH_MAX];
	int res; 
	for(int i = 0; i < DEVICES_AMT; i++) {
			snprintf(current_path, PATH_MAX, "%s/app/%s", DEVICES[i], current_game->titleid);
			if((res = sceAppMgrGameDataMount(current_path, 0, 0, mount_point))>=0)
				return res;
	}
	return res;
}

int dumpPopulateThread(unsigned int argc,  void *argv) {
	force_end = 0;
	MenuAddEntry(current_dump_menu, MENU_NORMAL, "DECRYPT ALL (CURRENTLY LOOKING FOR MODULES...)", dumpAllCB)->type = MENU_WARNING;
	MenuAddEntry(current_dump_menu, MENU_NORMAL, "Back to main menu", dumpModuleEntryBackCB)->type = MENU_BACK;
	char current_path[PATH_MAX];
	char mount_point[0x11];
	if(current_game->type == SELF_SYSTEM) {
		dumpParseFolder(current_dump_menu, current_game->title);
	} else if(gamePfsDecrypt(current_game->titleid, mount_point) >=0) {
		for(int i = 0; i < DEVICES_AMT; i++) {
			snprintf(current_path, PATH_MAX, "%s/patch/%s", DEVICES[i], current_game->titleid);
			dumpParseFolder(current_dump_menu, current_path);
			snprintf(current_path, PATH_MAX, "%s/app/%s", DEVICES[i], current_game->titleid);
			dumpParseFolder(current_dump_menu, current_path);
		}
		sceAppMgrUmount(mount_point);
	}
	
	MenuEntry *entry = MenuFindType(current_dump_menu, MENU_WARNING);
	entry->type = MENU_NONE;
	strncpy(entry->text, "DECRYPT ALL(DONE)", sizeof(entry->text));
	sceKernelExitDeleteThread(0);
	return 0;
}

Menu *dumpGenerateModuleMenu(Menu *p_menu, int index) {
	parent_menu = p_menu;
	current_game = gameGetGameFromIndex(index);
	current_dump_menu = MenuCreate(PANE_LEFT);
	MenuAddEntry(current_dump_menu, MENU_TITLE, current_game->title, NULL);
	MenuAddEntry(current_dump_menu, MENU_SUBTITLE, "Choose a module to decypt", NULL);	
	populateThread_tid = sceKernelCreateThread("populateThread_tid", dumpPopulateThread, 0x10000100, 0x10000, 0, 0, NULL);
	if(populateThread_tid>0)
		sceKernelStartThread(populateThread_tid, 0, NULL);

	
	return current_dump_menu;
}

Menu *dumpGenerateMenu() {
	dump_menu = MenuCreate(PANE_RIGHT);
	MenuAddEntry(dump_menu, MENU_TITLE, "Modules to be decrypted", NULL);
	return dump_menu;
}

MenuEntry *dumpSkipGame(MenuEntry *c_entry) {
	MenuEntry *menu_entry = c_entry->next;
	while(menu_entry != NULL && menu_entry->type != MENU_DISABLE) {
		menu_entry = menu_entry->next;
	}
	return menu_entry;
}
char *dumpFindPath(char *path) {
	char *new_path_file = (char *)path;
	new_path_file = strchr(new_path_file, ':') + 1;
	if(new_path_file[0] == '/')
		new_path_file++;
	return new_path_file;
}
int decompressThread(unsigned int argc,  void *argv) {
	while(1) {
		unsigned int outbits;
		sceKernelWaitEventFlag(decompress_flag, (DECOMPRESS_NEW | DECOMPRESS_DONE), (SCE_EVENT_WAITOR | SCE_EVENT_WAITCLEAR), &outbits, 0);
		if(outbits & DECOMPRESS_DONE)
			break;
		sceKernelClearEventFlag(decompress_flag, DECOMPRESS_SEGOK);
		SCE_header *shdr = (SCE_header*)(current_self);
		Elf32_Ehdr *ehdr = (Elf32_Ehdr*)(current_self + shdr->elf_offset);
		Elf32_Phdr *phdrs = (Elf32_Phdr*)(current_self + shdr->phdr_offset);
		printf("starting decompress\n");
		for(int i = 0; i < ehdr->e_phnum; i++) {
			char current_path[PATH_MAX];
			snprintf(current_path, PATH_MAX, "%s.seg%i", current_elf, i);
			printf("checking %s\n",current_path);
			if(checkExists(current_path)==0) {
				printf("found\n");
				size_t seg_sz = phdrs[i].p_filesz;
				int res;
				if(seg_sz <= 0) {
					statusAddLog(MENU_WARNING, "Empty segment, will skip and delete %i", i);
					if(i > 0) {
						uint64_t padding_off =  (phdrs[i-1].p_offset + phdrs[i-1].p_filesz);
						size_t padding_sz = phdrs[i].p_offset - padding_off;
						if(padding_sz > 0) {
							char *padding = malloc(padding_sz);
							if(padding) {
								memset(padding, 0, padding_sz);
								if((res = WriteFileSeek(current_elf, padding, padding_off, padding_sz))<0) 
										statusAddLog(MENU_BAD, "Could not write padding: %i,%i", i, res);
								free(padding);
							} else
								statusAddLog(MENU_BAD, "Could not generate padding: %i,%x,%x", i, padding_sz, padding_off);
						}
					}
					sceIoRemove(current_path);
					continue;
				}
				size_t sz = getFileSize(current_path);
				char *file_buf = malloc(sz);
				if(!file_buf) {
					statusAddLog(MENU_BAD, "Could not allocate memory for decompression(file) %i", i);
					break;
				}
				void *dest_buf = malloc(phdrs[i].p_filesz);
				if(!dest_buf) {
					statusAddLog(MENU_BAD, "Could not allocate memory for decompression(dest) %i", i);
					free(file_buf);
					break;
				}
				if((res = ReadFile(current_path, file_buf,sz))<0) {
					statusAddLog(MENU_BAD, "Could not read decrypted segment: %i,%x", i, res);
					free(file_buf);
					free(dest_buf);
					break;
				}
				
				if((res = uncompress(dest_buf, (long unsigned int *)&seg_sz, file_buf, sz))!= Z_OK) {
					seg_sz =  phdrs[i].p_filesz;
					statusAddLog(MENU_BAD, "Could not decompress segment, will attempt inflate: %i,%i,%x", i, res, phdrs[i].p_type);
					if((res = test_large_inflate(file_buf, sz, dest_buf, seg_sz)) != Z_OK) {
						statusAddLog(MENU_BAD, "That also failed, will write uncompressed: %i,%i,%x", i, res, phdrs[i].p_type);
						if((res = WriteFileSeek(current_elf, file_buf, phdrs[i].p_offset, phdrs[i].p_filesz))<0) 
							statusAddLog(MENU_BAD, "Could not write segment: %i,%i", i, res);
						free(file_buf);
						free(dest_buf);
						break;
					}
				}
				if(dump_type == DUMP_SELF) {
					statusAddLog(MENU_NONE, "Compressing segment for self: %i", i);
					if((res = compress(file_buf, (long unsigned int *)&sz, dest_buf, seg_sz))!= Z_OK) {
						statusAddLog(MENU_BAD, "Could not compress: %i,%i,%x", i, res, phdrs[i].p_type);
						free(file_buf);
						free(dest_buf);
						break;
					}
					segment_info *seg = (segment_info*)(current_self + shdr->section_info_offset);
					seg[i].encryption = 2;
					if((res = WriteFileSeek(current_file, file_buf, seg[i].offset, sz)) < 0)
						statusAddLog(MENU_BAD, "Could not write decrypted segment to self: %i,%x", i, res);
				}
				
				free(file_buf);
				if((res = WriteFileSeek(current_elf, dest_buf, phdrs[i].p_offset, phdrs[i].p_filesz))<0) 
					statusAddLog(MENU_BAD, "Could not write decrypted segment to elf: %i,%x", i, res);
				free(dest_buf);
				if((res = sceIoRemove(current_path))<0) 
					statusAddLog(MENU_BAD, "Could not remove decrypted segment to elf: %i,%x", i, res);
				statusAddLog(MENU_NONE, "Finished segment %i", i);
			}
		}
		sceKernelSetEventFlag(decompress_flag, DECOMPRESS_SEGOK);

	}
	sceKernelSetEventFlag(decompress_flag, DECOMPRESS_SEGOK);
	sceClibPrintf("would exit\n");
	sceKernelExitDeleteThread(0);
	return 0;
}
void dumpExtractTitleId(const char *path, char *titleid) {
	char temp_path[PATH_MAX];
	strcpy(temp_path, path);
	char *folder = (char *)&temp_path;
	folder = strchr(folder, ':') + 1;
	if(folder[0] == '/')
		folder++;
	folder = strchr(folder, '/') + 1;
	char *end_path  = strchr(folder, '/');
	*end_path = 0;
	strncpy(titleid, folder, 20);
}

int dumpVerifyElf(const char *path,  uint8_t *orig_elf_digest) {
	int res =0;
	size_t sz = getFileSize(path);
	if(sz < 0)
		return 0;
	uint8_t *elf = (uint8_t*)malloc(sz);
	uint8_t elf_digest[0x20];
	if(elf) {
		if(ReadFile(path, elf, sz) <0) {
			free(elf);
			return 0;
		}
		SHA256_CTX ctx;
		sha256_init(&ctx);
		sha256_update(&ctx, elf, sz);
		sha256_final(&ctx, elf_digest);
		res = !memcmp(orig_elf_digest, &elf_digest, sizeof(elf_digest));
		free(elf);
		
	} else
		return 0;
	return res;
}

int dumpThread(unsigned int argc,  void *argv) {
	char aid[8];
	int res;
	statusAddLog(MENU_WARNING, "Starting kuEntente plugin: %x", taiLoadStartKernelModule("ux0:/app/VDEC00001/kentente.skprx", 0, NULL, 0));
	if(uplugin_modid < 0)
		statusAddLog(MENU_WARNING, "Starting userAllied plugin: %x",  uplugin_modid = sceKernelLoadStartModule("ux0:/app/VDEC00001/userAllied.suprx", 0, NULL, 0, NULL, NULL));
	sceKernelDelayThread(175 * 1000);
	sceIoMkdir(OUT_FOLDER"",6);
	if(uplugin_modid<0) {
		statusAddLog(MENU_BAD, "ERROR starting userplugin");
		return 0;
	}
	if(userAlliedStatus() != ENTENTE_DONE) {
		statusAddLog(MENU_BAD, "ERROR kuEntente is busy");
	}
	if((res=sceRegMgrGetKeyBin("/CONFIG/NP", "account_id", aid, sizeof(aid)))<0) {
		statusAddLog(MENU_BAD, "Obtaining AID: %x", res);
		return 0;
	}
	statusAddLog(MENU_NONE, "Obtaining AID: %x", res);
	char rif_name[0x30];
	if((res=_sceNpDrmGetFixedRifName(rif_name, 0, 0LL))<0) {
		statusAddLog(MENU_BAD, "Obtaining Fixed Rif name: %x", res);
		return 0;
	}
	statusAddLog(MENU_NONE, "Obtaining Fixed Rif name: %s", rif_name);
	MenuEntry *menu_entry = dump_menu->entry_start;
	char titleid[20];
	char rif_path[PATH_MAX];
	char out_path[PATH_MAX];
	char elf_path[PATH_MAX];
	char mount_point[0x11];
	int auth_type = 0;
	int system = 0;
	decompress_flag = sceKernelCreateEventFlag( "DecompressEvent", 0, 0, NULL );
	decompressThread_tid = sceKernelCreateThread("decompress_thread", decompressThread, 0x10000100, 0x10000, 0, 0, NULL);
	if(decompressThread_tid>0)
		sceKernelStartThread(decompressThread_tid, 0, NULL);
	while(menu_entry != NULL) {
		dump_menu->active = 0;
		if(menu_entry->draw_type == MENU_SUBTITLE) {
			auth_type = 1;
			strncpy(titleid, menu_entry->text, sizeof(titleid));
			statusAddLog(MENU_NONE, "Switching to: %s", titleid);
			if(strncmp(titleid, "SYSTEM", sizeof("SYSTEM"))==0) {
				statusAddLog(MENU_NONE, "System, Skipping rif check");
				auth_type = 0;
				system = 1;
				menu_entry = menu_entry->next ;
				continue;
			} 
			system = 0;
			res=gamePfsDecrypt(titleid, mount_point);
			if(res < 0) {
				statusAddLog(MENU_BAD, "Mounting PFS: %x", res);
				menu_entry = dumpSkipGame(menu_entry);
				continue;
			}
			statusAddLog(MENU_NONE, "Mounting PFS: %x", res);
			for(int i = 0; i < DEVICES_AMT; i++) {
				snprintf(rif_path, PATH_MAX, "%slicense/app/%s/%s", DEVICES[i], titleid, rif_name);
				if((res = checkExists(rif_path))==0) 
					break;
			}
			
			if(res!=0) {
				for(int i = 0; i < DEVICES_AMT; i++) {
					snprintf(rif_path, PATH_MAX, "%slicense/app/%s", DEVICES[i], titleid);
					SceUID dfd = sceIoDopen(rif_path);
					if (dfd >= 0) {
						SceIoDirent dir;
						memset(&dir, 0, sizeof(SceIoDirent));
						res = sceIoDread(dfd, &dir);
						if(res > 0) {
							snprintf(rif_path, PATH_MAX, "%slicense/app/%s/%s", DEVICES[i], titleid, dir.d_name);
							sceIoDclose(dfd);
							res = 0;
							break;
						}
						sceIoDclose(dfd);
					}	
						
				}
			}
			
			if(res != 0) {
				statusAddLog(MENU_BAD, "License not found!");
				menu_entry = dumpSkipGame(menu_entry);
				continue;
			}
			statusAddLog(MENU_NONE, "Using rif: %s", rif_path);
		}
		if(menu_entry->draw_type == MENU_NORMAL) {
			statusAddLog(MENU_NONE, "Decrypting: %s", menu_entry->text);
			generateFolders(menu_entry->text, OUT_FOLDER"");
			snprintf(elf_path, PATH_MAX, OUT_FOLDER"/%s.elf",dumpFindPath(menu_entry->text)); 
			current_file = current_elf = elf_path;
			if(dump_type == DUMP_SELF) {
				snprintf(out_path, PATH_MAX, OUT_FOLDER"/%s",dumpFindPath(menu_entry->text)); 
				current_file = out_path;
				current_elf = elf_path;
			}
			statusAddLog(MENU_NONE, "Outpath: %s", current_file);
			
			current_self = malloc(HEADER_LEN);
			if(!current_self) {
				statusAddLog(MENU_BAD, "Could not allocate memory");
				menu_entry = menu_entry->next ;
			}
				
			if((res = ReadFile(menu_entry->text, current_self, HEADER_LEN)) < 0) {
				statusAddLog(MENU_BAD, "Could not read original self: %x", res);
				free(current_self);
				menu_entry = menu_entry->next ;
			}
			
			char temp_outpath[PATH_MAX];
			snprintf(temp_outpath, PATH_MAX, "%s.temp", current_elf);
			current_elf = temp_outpath;
			ententeParams param;
			param.rifpath = rif_path;
			param.path = menu_entry->text;
			param.outpath = temp_outpath;
			param.path_id = -1;
			param.self_type = 1;
			if(system) {
				if(strstr(menu_entry->text,"os0")!=NULL && strstr(menu_entry->text,".skprx")!=NULL) {
					param.self_type = 0;
					statusAddLog(MENU_NONE, "Setting to system self");
				}
				if(strstr(menu_entry->text,"app_em")  != NULL)
					param.path_id = 23;
				else if(strstr(menu_entry->text,"patch_em")  != NULL) 
					param.path_id = 24;
				else if(strstr(menu_entry->text,"vs0_em") != NULL) 
					param.path_id = 3;
				else if(strstr(menu_entry->text,"os0_em") != NULL) 
					param.path_id = 2; 
				
				if(param.path_id == 24||param.path_id == 23) {
					auth_type = 1;
					dumpExtractTitleId(menu_entry->text, titleid);
					statusAddLog(MENU_NONE, "Setting title id to: %s", titleid);
					snprintf(rif_path, PATH_MAX, "ux0:app_em/%s/sce_sys/package/work.bin",titleid);
					if(checkExists(rif_path)!=0) {
						statusAddLog(MENU_BAD, "work.bin not found: %s", rif_path);
						param.rifpath = NULL;
					} else 
						statusAddLog(MENU_NONE, "Using work.bin: %s", rif_path);
					
				} else
					param.rifpath = NULL;
				statusAddLog(MENU_NONE, "Setting path id to %i", param.path_id);
			}
			
			char auth_outpath[PATH_MAX];
			char old_path[PATH_MAX];
			strcpy(old_path, old_path);
			snprintf(old_path, PATH_MAX, "%s.auth", current_elf);
			SCE_header *shdr = (SCE_header*)(current_self);
			Elf32_Ehdr *ehdr = (Elf32_Ehdr*)(current_self + shdr->elf_offset);
			SCE_appinfo *appinfo = (SCE_appinfo*)(current_self + shdr->appinfo_offset);
			if(!auth_type) {
				
				strncpy(auth_outpath, old_path, PATH_MAX);
				strcpy(strstr(auth_outpath, ".temp"), ".auth");
			} else {
				if(appinfo->self_type != 0x8) {
					statusAddLog(MENU_NONE, "Skipping because not needed");
					free(current_self);
					menu_entry = menu_entry->next ;
					continue;
				}
				strncpy(auth_outpath, old_path, PATH_MAX);					
				char *ending = strstr(auth_outpath, titleid) + strlen(titleid) + 1;
				*ending = 0;
				snprintf(auth_outpath, PATH_MAX, "%sself_auth.bin", auth_outpath);
				printf("auth path: %s\n", auth_outpath);
			}
			param.self_auth =  (checkExists(auth_outpath) < 0);
			userAlliedDecryptSelf(&param);
			statusAddLog(MENU_NONE, "Waiting for decrypter");
			char buffer[128];
			while(userAlliedStatus() != ENTENTE_UPDATE);
			while(1) {
				if(userAlliedStatus() == ENTENTE_DONESEG) {
					sceKernelSetEventFlag(decompress_flag, DECOMPRESS_NEW);
					userAlliedGetLogs(buffer);
					statusAddLog(MENU_NONE, "kuEntente: %s", buffer);
				}
				if(userAlliedStatus() == ENTENTE_UPDATE) {
					userAlliedGetLogs(buffer);
					statusAddLog(MENU_NONE, "kuEntente: %s", buffer);
				} else if(userAlliedStatus() == ENTENTE_DONE)
					break;
			}
			userAlliedGetLogs(buffer);
			statusAddLog(MENU_NONE, "kuEntente: %s", buffer);
			sceKernelWaitEventFlag(decompress_flag, DECOMPRESS_SEGOK, SCE_EVENT_WAITAND, 0, 0);
			sceKernelSetEventFlag(decompress_flag, DECOMPRESS_NEW);
			sceKernelWaitEventFlag(decompress_flag, DECOMPRESS_SEGOK, SCE_EVENT_WAITAND, 0, 0);
			
			ehdr->e_shnum = 0;
			ehdr->e_shoff = 0;
			
			if((res = WriteFileSeek(current_elf, current_self + shdr->elf_offset, 0, sizeof(Elf32_Ehdr))) < 0) {
				statusAddLog(MENU_BAD, "Could not write original elfhdr to elf: %x", res);
				free(current_self);
				menu_entry = menu_entry->next ;
				continue;
			}
			if((res = WriteFileSeek(current_elf, current_self + shdr->phdr_offset, ehdr->e_phoff, ehdr->e_phentsize * ehdr->e_phnum)) < 0) {
				statusAddLog(MENU_BAD, "Could not write original phdrs to elf: %x", res);
				free(current_self);
				menu_entry = menu_entry->next ;
				continue;
			}
			if(param.self_auth) {
				if((res = sceIoRename(old_path, auth_outpath)) < 0)
					statusAddLog(MENU_BAD, "Could not properly save self auth: %x", res);
				else
					statusAddLog(MENU_NONE, "Saved self auth: %s", auth_outpath);
			}
			statusAddLog(MENU_NONE, "Stripping NpDRM from header...");
			uint8_t orig_elf_digest[0x20];
			PSVita_CONTROL_INFO *control_info = (PSVita_CONTROL_INFO *)(current_self + shdr->controlinfo_offset);
			while(control_info->next) {
				switch(control_info->type) {
					case 4:
						control_info->PSVita_elf_digest_info.min_required_fw = 0x0;
						memcpy(&orig_elf_digest, control_info->PSVita_elf_digest_info.elf_digest, sizeof(orig_elf_digest));
						statusAddLog(MENU_NONE, "Got elf digest");
						break;
					case 5:
						memset(&control_info->PSVita_npdrm_info, 0, sizeof(control_info->PSVita_npdrm_info));
						break;
				}
				control_info = (PSVita_CONTROL_INFO*)((char*)control_info + control_info->size);
			}
			
			
			if(dump_type == DUMP_SELF) {
				if((res = WriteFileSeek(current_file, current_self + shdr->elf_offset, shdr->header_len, sizeof(Elf32_Ehdr))) < 0) {
					statusAddLog(MENU_BAD, "Could not write original elfhdr to self: %x", res);
					free(current_self);
					menu_entry = menu_entry->next ;
					continue;
				}
				if((res = WriteFileSeek(current_file, current_self + shdr->phdr_offset, shdr->header_len + ehdr->e_phoff, ehdr->e_phentsize * ehdr->e_phnum)) < 0) {
					statusAddLog(MENU_BAD, "Could not write original phdrs to self: %x", res);
					free(current_self);
					menu_entry = menu_entry->next ;
					continue;
				}
				appinfo->version = 0;
				shdr->sdk_type = 0xc0;
				if((res = WriteFileSeek(current_file, current_self, 0, 0x600)) < 0) {
					statusAddLog(MENU_BAD, "Could not write original shdr to self: %x", res);
					free(current_self);
					menu_entry = menu_entry->next ;
					continue;
				}
				if(dumpVerifyElf(current_elf, orig_elf_digest)) {
					statusAddLog(MENU_NONE, "ELF VERIFIED. PURIST REJOICE!");
					if((res = sceIoRemove(current_elf)) < 0)
						statusAddLog(MENU_BAD, "Could not remove temp elf");
				} else
					statusAddLog(MENU_BAD, "ELF IS CORRUPTED. IT IS AS BAD AS MAI. Or its too big.");
			} else {
				if((res = sceIoRename(current_elf, elf_path)) < 0) {
						statusAddLog(MENU_BAD, "Could not rename to output: %x", res);
						free(current_self);
						menu_entry = menu_entry->next ;
						continue;
				}
				snprintf(temp_outpath, PATH_MAX, "%s.sha256", elf_path);
				statusAddLog(MENU_NONE, "Saving digest to: %s", temp_outpath);
				if((res = WriteFile(temp_outpath, &orig_elf_digest, sizeof(orig_elf_digest))) < 0)
					statusAddLog(MENU_BAD, "Error saving digest: %x", res);
			}

			free(current_self);
			statusAddLog(MENU_NONE, "Module done");
		}
		menu_entry = menu_entry->next ;
		if(menu_entry == NULL || menu_entry->draw_type == MENU_SUBTITLE) {
			sceKernelDelayThread(100 * 1000);
			statusAddLog(MENU_NONE, "Unmounting PFS: %x", sceAppMgrUmount(mount_point));
		}
		if(menu_entry)
			dump_menu->entry_selected = menu_entry;
		
	}
	sceKernelSetEventFlag(decompress_flag, DECOMPRESS_DONE);
	sceKernelWaitEventFlag(decompress_flag, DECOMPRESS_SEGOK, SCE_EVENT_WAITAND, 0, 0);
	sceKernelWaitThreadEnd(decompressThread_tid, &res, NULL);
	statusAddLog(MENU_WARNING, "Done");
	dumpThread_tid = -1;
	sceKernelExitDeleteThread(0);
	return 0;
}

void dumpStart(int type) {
	statusAddLog(MENU_NONE, "Starting dump...");
	if(dumpThread_tid < 0) {
		dump_type = type;
		dumpThread_tid = sceKernelCreateThread("dump_thread", dumpThread, 0x10000100, 0x10000, 0, 0, NULL);
		if(dumpThread_tid>0)
			sceKernelStartThread(dumpThread_tid, 0, NULL);
		else
			statusAddLog(MENU_BAD, "Dump thread failed to start");
	}
}