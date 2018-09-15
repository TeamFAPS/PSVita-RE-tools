#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "elf.h"
#include <inttypes.h>


//located at 0xE4
typedef struct
{
 uint32_t filename_offset;
 uint32_t elf_offset;
 uint32_t elf_size;
} ENTRY_TABLE;


// You must free the result if result is non-NULL.
char *str_replace(char *orig, char *rep, char *with) {
    char *result; // the return string
    char *ins;    // the next insert point
    char *tmp;    // varies
    int len_rep;  // length of rep (the string to remove)
    int len_with; // length of with (the string to replace rep with)
    int len_front; // distance between rep and end of last rep
    int count;    // number of replacements

    // sanity checks and initialization
    if (!orig || !rep)
        return NULL;
    len_rep = strlen(rep);
    if (len_rep == 0)
        return NULL; // empty rep causes infinite loop during count
    if (!with)
        with = "";
    len_with = strlen(with);

    // count the number of replacements needed
    ins = orig;
    for (count = 0; tmp = strstr(ins, rep); ++count) {
        ins = tmp + len_rep;
    }

    tmp = result = malloc(strlen(orig) + (len_with - len_rep) * count + 1);

    if (!result)
        return NULL;

    // first time through the loop, all the variable are set correctly
    // from here on,
    //    tmp points to the end of the result string
    //    ins points to the next occurrence of rep in orig
    //    orig points to the remainder of orig after "end of rep"
    while (count--) {
        ins = strstr(orig, rep);
        len_front = ins - orig;
        tmp = strncpy(tmp, orig, len_front) + len_front;
        tmp = strcpy(tmp, with) + len_with;
        orig += len_front + len_rep; // move to next "end of rep"
    }
    strcpy(tmp, orig);
    return result;
}


int main(int argc, char **argv){
	
	if(argc < 3){
		printf ("\nusage: bootimage_extract table.bin outdir/ \n");
		return -1;
	}
	
	FILE *fp = fopen(argv[1],"rb");
	
	
	unsigned char *input = (unsigned char*) malloc (0x400);
	fseek(fp, 0, SEEK_SET);
	fread(input,0x400,1,fp);
	Elf32_Ehdr *elf = (Elf32_Ehdr *)input;
	Elf32_Phdr *phdrs = input + elf->e_phoff;
	printf("Segment 0 off: %x\n",phdrs[0].p_offset);
	int i = 0;
	while(i!= 0x1C * 2){
		
		unsigned char *data_in_buf = (unsigned char*) malloc (12);
		unsigned char *data_in_buf_2 = (unsigned char*) malloc (27);
	
		fseek(fp, phdrs[0].p_offset + 0xE4 + (12*i), SEEK_SET);
		fread(data_in_buf,12,1,fp);
		ENTRY_TABLE* entry_table = (ENTRY_TABLE*)data_in_buf;
		
		printf("filename_offset:    0x%X\n", entry_table->filename_offset);
		printf("elf_offset:       0x%X\n", entry_table->elf_offset);
		printf("elf_size:     0x%X\n", entry_table->elf_size);
		printf("increment:     0x%X\n", i);
		
		entry_table->filename_offset = entry_table->filename_offset - 0x81000000;
		entry_table->elf_offset = entry_table->elf_offset - 0x81000000;
		
		fseek(fp, phdrs[0].p_offset + entry_table->filename_offset, SEEK_SET);
		fread(data_in_buf_2, 27, 1, fp);
		data_in_buf_2 = str_replace (data_in_buf_2, "os0:kd/", argv[2]);
		rmdir(argv[2]);
		mkdir(argv[2],777);
		printf("filename:    %s\n", data_in_buf_2);
		FILE *fout = fopen(data_in_buf_2, "wb");
		fseek(fp, phdrs[0].p_offset + entry_table->elf_offset, SEEK_SET);
		unsigned char *data_in_buf_3 = (unsigned char*) malloc (entry_table->elf_size);
		fread(data_in_buf_3, entry_table->elf_size, 1, fp);
		fwrite(data_in_buf_3, 1 ,entry_table->elf_size, fout);
		fclose(fout);
		free(data_in_buf);
		free(data_in_buf_2);
		free(data_in_buf_3);
		
		i++;
	}
	free(elf);
	fclose(fp);
	
	return 0;
}