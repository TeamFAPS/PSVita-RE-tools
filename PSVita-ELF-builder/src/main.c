#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <zlib.h>

#include "file.h"

#define CHUNK 16384

/* Compress from file source to file dest until EOF on source.
   def() returns Z_OK on success, Z_MEM_ERROR if memory could not be
   allocated for processing, Z_STREAM_ERROR if an invalid compression
   level is supplied, Z_VERSION_ERROR if the version of zlib.h and the
   version of the library linked do not match, or Z_ERRNO if there is
   an error reading or writing the files. */
int def(FILE *source, FILE *dest, int level)
{
    int ret, flush;
    unsigned have;
    z_stream strm;
    unsigned char in[CHUNK];
    unsigned char out[CHUNK];

    /* allocate deflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    ret = deflateInit(&strm, level);
    if (ret != Z_OK)
        return ret;

    /* compress until end of file */
    do {
        strm.avail_in = fread(in, 1, CHUNK, source);
        if (ferror(source)) {
            (void)deflateEnd(&strm);
            return Z_ERRNO;
        }
        flush = feof(source) ? Z_FINISH : Z_NO_FLUSH;
        strm.next_in = in;

        /* run deflate() on input until output buffer not full, finish
           compression if all of source has been read in */
        do {
            strm.avail_out = CHUNK;
            strm.next_out = out;
            ret = deflate(&strm, flush);    /* no bad return value */
            assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
            have = CHUNK - strm.avail_out;
            if (fwrite(out, 1, have, dest) != have || ferror(dest)) {
                (void)deflateEnd(&strm);
                return Z_ERRNO;
            }
        } while (strm.avail_out == 0);
        assert(strm.avail_in == 0);     /* all input will be used */

        /* done when last data in file processed */
    } while (flush != Z_FINISH);
    assert(ret == Z_STREAM_END);        /* stream will be complete */

    /* clean up and return */
    (void)deflateEnd(&strm);
    return Z_OK;
}

/* Decompress from file source to file dest until stream ends or EOF.
   inf() returns Z_OK on success, Z_MEM_ERROR if memory could not be
   allocated for processing, Z_DATA_ERROR if the deflate data is
   invalid or incomplete, Z_VERSION_ERROR if the version of zlib.h and
   the version of the library linked do not match, or Z_ERRNO if there
   is an error reading or writing the files. */
int inf(FILE *source, FILE *dest)
{
    int ret;
    unsigned have;
    z_stream strm;
    unsigned char in[CHUNK];
    unsigned char out[CHUNK];

    /* allocate inflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;
    ret = inflateInit(&strm);
    if (ret != Z_OK)
        return ret;

    /* decompress until deflate stream ends or end of file */
    do {
        strm.avail_in = fread(in, 1, CHUNK, source);
        if (ferror(source)) {
            (void)inflateEnd(&strm);
            return Z_ERRNO;
        }
        if (strm.avail_in == 0)
            break;
        strm.next_in = in;

        /* run inflate() on input until output buffer not full */
        do {
            strm.avail_out = CHUNK;
            strm.next_out = out;
            ret = inflate(&strm, Z_NO_FLUSH);
            assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
            switch (ret) {
            case Z_NEED_DICT:
                ret = Z_DATA_ERROR;     /* and fall through */
            case Z_DATA_ERROR:
            case Z_MEM_ERROR:
                (void)inflateEnd(&strm);
                return ret;
            }
            have = CHUNK - strm.avail_out;
            if (fwrite(out, 1, have, dest) != have || ferror(dest)) {
                (void)inflateEnd(&strm);
                return Z_ERRNO;
            }
        } while (strm.avail_out == 0);

        /* done when inflate() says it's done */
    } while (ret != Z_STREAM_END);

    /* clean up and return */
    (void)inflateEnd(&strm);
    return ret == Z_STREAM_END ? Z_OK : Z_DATA_ERROR;
}

/* report a zlib or i/o error */
void zerr(int ret)
{
    fputs("zpipe: ", stderr);
    switch (ret) {
    case Z_ERRNO:
        if (ferror(stdin))
            fputs("error reading stdin\n", stderr);
        if (ferror(stdout))
            fputs("error writing stdout\n", stderr);
        break;
    case Z_STREAM_ERROR:
        fputs("invalid compression level\n", stderr);
        break;
    case Z_DATA_ERROR:
        fputs("invalid or incomplete deflate data\n", stderr);
        break;
    case Z_MEM_ERROR:
        fputs("out of memory\n", stderr);
        break;
    case Z_VERSION_ERROR:
        fputs("zlib version mismatch!\n", stderr);
    }
}

int zlib_compress(const char *inputFile_path, const char *outputFile_path) {
	int ret;
	FILE *fp, *fs;
	fp = fopen(inputFile_path, "rb");
	fs = fopen(outputFile_path, "w+b");
	ret = def(fp, fs, Z_DEFAULT_COMPRESSION);
	if (ret != Z_OK)
		zerr(ret);
	fclose(fp);
	fclose(fs);
	return ret;
}

int zlib_decompress(const char *inputFile_path, const char *outputFile_path) {
	int ret;
	FILE *fp, *fs;
	fp = fopen(inputFile_path, "rb");
	if (!fp) return -3;
	fs = fopen(outputFile_path, "w+b");
	ret = inf(fp, fs);
	if (ret != Z_OK)
		zerr(ret);
	fclose(fp);
	fclose(fs);
	return ret;
}


int makeElf(const char* originalPath, const char* segmentsPath, const char* outputPath) {

	// PRELIMINARY PART : CHECK INPUT FILES EXISTENCE
	
	// Check that original SELF/SPRX file exists and is not empty
	FILE *fcheck_original = fopen(originalPath, "rb");
	if (fcheck_original == NULL) {
		printf("Original file has not be found : %s\nCannot make elf...", originalPath);
		fclose(fcheck_original);
		return 1;
	}
	fclose(fcheck_original);
	
	// Check that segments files exist and are not empty
	// And count of the segments number
	int segmentsNumber = 0;
	for (int current=0; current<=4; current++) {
		char currentSegmentPath[1024];
		sprintf(currentSegmentPath, "%s.seg%i", segmentsPath, current);
		FILE *fcheck_currentCompressedSegment = fopen(currentSegmentPath, "rb");
		if (fcheck_currentCompressedSegment == NULL) {
			printf("seg%i file has not be found : %s\n", current, currentSegmentPath);
			if (current == 0) {
				printf("Cannot make elf...\n");
				return 1;
			}
		} else segmentsNumber++;
		fclose(fcheck_currentCompressedSegment);
	}
	printf("segments number : %i\n", segmentsNumber);
	
	// FIRST PART : MAKE HEADER FROM ORIGINAL SELF/SPRX AND GET SOME INFOS
	
	FILE *foriginal, *foutputelf;
	foriginal = fopen(originalPath, "rb");
	foutputelf = fopen(outputPath, "w+b");
	int sizeOriginal, lSize, offset;
	char *buffer;
	char *buffer_byte = malloc(sizeof(char)*1);
	
	int bytee4 = read_byte(foriginal, 0xE4, &buffer_byte);
	printf("original SELF/SPRX bytee4 : %X\n", bytee4);
	int bytee5 = read_byte(foriginal, 0xE5, &buffer_byte);
	printf("original SELF/SPRX bytee5 : %X\n", bytee5);
	int bytee6 = read_byte(foriginal, 0xE6, &buffer_byte);
	printf("original SELF/SPRX bytee6 : %X\n", bytee6);
	int firstSegmentOffset = bytee4 * 0x1 + bytee5 * 0x100 + bytee6 * 0x10000;
	printf("--> original SELF/SPRX first segment offset : %X\n", firstSegmentOffset);
	
	int byte104 = read_byte(foriginal, 0x104, &buffer_byte);
	printf("original SELF/SPRX byte104 : %X\n", byte104);
	int byte105 = read_byte(foriginal, 0x105, &buffer_byte);
	printf("original SELF/SPRX byte105 : %X\n", byte105);
	int byte106 = read_byte(foriginal, 0x106, &buffer_byte);
	printf("original SELF/SPRX byte106 : %X\n", byte106);
	int secondSegmentOffset = byte104 * 0x1 + byte105 * 0x100 + byte106 * 0x10000;
	printf("--> original SELF/SPRX second segment offset : %X\n", secondSegmentOffset);
	
	int byte124 = read_byte(foriginal, 0x124, &buffer_byte);
	printf("original SELF/SPRX byte124 : %X\n", byte124);
	int byte125 = read_byte(foriginal, 0x125, &buffer_byte);
	printf("original SELF/SPRX byte125 : %X\n", byte125);
	int byte126 = read_byte(foriginal, 0x126, &buffer_byte);
	printf("original SELF/SPRX byte126 : %X\n", byte126);
	int thirdSegmentOffset = byte124 * 0x1 + byte125 * 0x100 + byte126 * 0x10000;
	printf("--> original SELF/SPRX third segment offset : %X\n", thirdSegmentOffset);
	
	int byte144 = read_byte(foriginal, 0x144, &buffer_byte);
	printf("original SELF/SPRX byte144 : %X\n", byte144);
	int byte145 = read_byte(foriginal, 0x145, &buffer_byte);
	printf("original SELF/SPRX byte145 : %X\n", byte145);
	int byte146 = read_byte(foriginal, 0x146, &buffer_byte);
	printf("original SELF/SPRX byte146 : %X\n", byte146);
	int fourthSegmentOffset = byte144 * 0x1 + byte145 * 0x100 + byte146 * 0x10000;
	printf("--> original SELF/SPRX fourth segment offset : %X\n", fourthSegmentOffset);
	
	// Get original SELF/SPRX file size
	sizeOriginal = fsize(foriginal);
	printf("original SELF/SPRX size : %d\n", sizeOriginal);
	
	// Copy first part of the header
	rewind(foriginal);
	offset = 0xA0;
	fseek(foriginal, offset, SEEK_SET);
	lSize = 0x30;
	buffer = (char*) malloc(sizeof(char)*lSize); // Allocate memory to contain the good size.
	fread(buffer, sizeof(char), lSize, foriginal);
	fwrite(buffer, sizeof(char), lSize, foutputelf);
	
	// Copy second part of the header
	rewind(foriginal);
	offset = 0xDC;
	fseek(foriginal, offset, SEEK_SET);
	lSize = firstSegmentOffset - 0x3C;
	buffer = (char*) malloc(sizeof(char)*lSize); // Allocate memory to contain the good size.
	fread(buffer, sizeof(char), lSize, foriginal);
	fwrite(buffer, sizeof(char), lSize, foutputelf);
	printf("2nd part length : %X\n", lSize);
	
	// AUTHENTICATION ID BACKUP
	uint64_t authid;
	rewind(foriginal);
	fseek(foriginal, 0x80, SEEK_SET);
	fread(&authid, sizeof(char), sizeof(uint64_t), foriginal);
	printf("AUTH ID : %llX\n", authid);
	
	fclose(foriginal); // No need for the original SELF/SPRX file anymore
	
	// Write needed bytes of zeroes (the 0xC previously removed bytes of zeroes from original ELF header)
	for (int i=0; i<0xC; i++)
		fwrite((char[1]){0x00}, sizeof(char), sizeof(char[1]), foutputelf);
	
	// SECOND PART : ADD SEGMENTS (dumped by zecoxao's vitadump)
	
	char currentSegmentPath[256];
	char currentSegmentOutputPath[256];
	FILE *fcurrentSegment;
	int currentSegmentSize;
	for (int current=0; current<segmentsNumber; current++) {
		
		// Get current segment paths
		sprintf(currentSegmentPath, "%s.seg%i", segmentsPath, current);
		sprintf(currentSegmentOutputPath, "%s.bin", currentSegmentPath);
		
		// Decompress current segment
		printf("Decompressing %s to %s.\n", currentSegmentPath, currentSegmentOutputPath);
		
		// Check if current segment was already uncompressed
		if (zlib_decompress(currentSegmentPath, currentSegmentOutputPath) == -3) { // zlib_decompress returns -3 in case the file is already uncompressed or empty
			printf("Current segment (%s) is already uncompressed or empty.\n", currentSegmentPath);
			sprintf(currentSegmentOutputPath, "%s", currentSegmentPath);
		}
		
		fcurrentSegment = fopen(currentSegmentOutputPath, "rb");
		
		// Check that current uncompressed segment exists
		if (fcurrentSegment == NULL) {
			printf("Uncompressed seg%i has not be found : %s\nCannot make elf...", current, currentSegmentOutputPath);
			fclose(fcurrentSegment);
			return 1;
		}
		
		// Get current uncompressed segment file size
		currentSegmentSize = fsize(fcurrentSegment);
		printf("Uncompressed seg%i file size : %d\n", current, currentSegmentSize);
		
		// Read current uncompressed segment
		buffer = (char*) malloc(sizeof(char)*currentSegmentSize);// allocate memory to contain the whole file.
		fread(buffer, sizeof(char), currentSegmentSize, fcurrentSegment);
		
		// Write current uncompressed segment to the output .elf
		fwrite(buffer, sizeof(char), currentSegmentSize, foutputelf);
		fclose(fcurrentSegment);
		
		// Write needed bytes of zeroes (to have a good padding for the next segment)
		if (current == 0) {
			int secondSegmentInterval = secondSegmentOffset - (firstSegmentOffset + currentSegmentSize);
			printf("Before second segment blank interval : %X\n", secondSegmentInterval);
			for (int i=0; i < secondSegmentInterval; i++)
				fwrite((char[1]){0x00}, sizeof(char), sizeof(char[1]), foutputelf);
		} else if (current == 1) {
			int thirdSegmentInterval = thirdSegmentOffset - (secondSegmentOffset + currentSegmentSize);
			printf("Before third segment blank interval : %X\n", thirdSegmentInterval);
			for (int i=0; i < thirdSegmentInterval; i++)
				fwrite((char[1]){0x00}, sizeof(char), sizeof(char[1]), foutputelf);
		} else if (current == 2) {
			int fourthSegmentInterval = fourthSegmentOffset - (thirdSegmentOffset + currentSegmentSize);
			printf("Before fourth segment blank interval : %X\n", fourthSegmentInterval);
			for (int i=0; i < fourthSegmentInterval; i++)
				fwrite((char[1]){0x00}, sizeof(char), sizeof(char[1]), foutputelf);
		}
	}
	fclose(foutputelf);
	
	// GENERATION OF THE MAKE-FSELF BATCH SCRIPT
	FILE *fmakefselfbat;
	fmakefselfbat = fopen("BATCH_MAKE_FSELF.BAT", "a");
	char text[256];
	sprintf(text, "\nvita-make-fself -c -a 0x%llX %s %s.self", authid, outputPath, outputPath);
	fwrite(text, sizeof(char), strlen(text), fmakefselfbat);
	fclose(fmakefselfbat);
	
	// BACKUP OF MODULE NAME, AUTH ID A?D PATH TO A FILE
	foutputelf = fopen(outputPath, "rb");
	fseek(foutputelf, 0x18, SEEK_SET);
	int elf_entrypoint = 0;
	fread(&elf_entrypoint, sizeof(char), sizeof(int), foutputelf);
	rewind(foutputelf);
	fseek(foutputelf, firstSegmentOffset+elf_entrypoint+4, SEEK_SET);
	char module_name[256];
	fread(module_name, sizeof(char), 256, foutputelf);
	fclose(foutputelf);
	FILE *fself_info_backup_list;
	fself_info_backup_list = fopen("module_info_backup_list.txt", "a");
	char text_2[512];
	sprintf(text_2, "\n|-\n| [[%s]] || 0x%llX || %s", module_name, authid, originalPath);
	fwrite(text_2, sizeof(char), strlen(text_2), fself_info_backup_list);
	fclose(fself_info_backup_list);
	
	return 0;
}


int main(int argc, char *argv[]) {
	if (argc == 3 && strcmp(argv[1], "") != 0) {
		char *original = argv[1];
		char *input = argv[2];
		char output[1024];
		sprintf(output, "%s.elf", argv[2]);
		printf("Making elf from %s to %s\n", original, input);
		if (!makeElf(original, input, output))
			printf("Made elf from %s to %s.", original, input);
		else
			printf("Error : makeElf from %s to %s aborted.", original, input);
        return 0;
    } else {
        fputs("Usage: PSVita-ELF-builder [original SELF/SPRX file path] [segments files path]\n", stderr);
        return 1;
    }
	return 1;
}