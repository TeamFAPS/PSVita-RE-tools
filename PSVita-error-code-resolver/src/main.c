// ErrorResolver.c - PSVita error codes resolver by Silica and Princess Of Sleeping

#include <stdio.h>
#include <stdlib.h>


unsigned int facility_group_num;
unsigned int error_table_num;

int read_error_table(const char *path, void *buf) {
	FILE *f = fopen(path, "rb");
	if (f) {
		if (fread(buf, 1, 8, f) == 8) {
			facility_group_num = *((short *)buf + 2);
			if (facility_group_num <= 0x21) {
				if (fread(*((int *)buf + 2), 1, 8 * facility_group_num, f) == 8 * facility_group_num
				&& fread((char *)buf + 12, 1, 4, f) == 4) {
					error_table_num = *((short *)buf + 7);
					if (error_table_num <= 0x4e21) {
						if (fread(*((int *)buf + 4), 1, 4 * error_table_num, f) == 4 * error_table_num) {
							fclose(f);
							return 0;
						}
					} else
						fprintf(stderr, "[SceError] error_table_num = %d\n", error_table_num);
				}
			} else
				fprintf(stderr, "[SceError] facility_group_num = %d\n", facility_group_num);
		}
		fclose(f);
	} else
		printf("Failed to open error_table.bin\n");
	*(int *)buf = 0;
	*((int *)buf + 1) = 0;
	*((int *)buf + 2) = 0;
	*((int *)buf + 3) = 0;
	*((int *)buf + 4) = 0;
	*((int *)buf + 5) = 0;
	return -1;
}

int error_code_to_string(int error_table, char *output_buffer, unsigned int Error_Code) {
	unsigned int *error_list; // r5
	int i; // r4
	int ii; // r6
	int Shifted; // r3
	char *facility; // r3
	int *error_table_32; // r2
	int error_table_16; // r6
	int error_init; // r4
	int Devider; // r2
	int Remainder; // r0
	int lastBit; // r7

	if (!output_buffer)
		return 0;
	error_list = *(unsigned int **)(error_table + 0x10);
	if (!*(short *)(error_table + 0xE)) {
	NOT_IN_DB:
		snprintf(output_buffer, 0x10, "E-%08x", Error_Code);
		return 0;
	}
	if (*error_list != Error_Code) {
		i = 0;
		while (++i < *(unsigned __int16 *)(error_table + 0xE)) {
			if (error_list[1] == Error_Code) {
				if (*(unsigned __int16 *)(error_table + 6) >= i)
					goto CALC_FACILITY;
				snprintf(output_buffer, 0x10, "*-%08x");
				return 0;
			}
			++error_list;
		}
		goto NOT_IN_DB;
	}
	i = 0;
CALC_FACILITY:
	Shifted = (Error_Code >> 16) & 0xFFF;
	if ((unsigned int)(Shifted - 0x200) > 0xFF) {
		if (*(short *)(error_table + 4)) {
			error_table_32 = (int *)(*(int *)(error_table + 8) + 8);
			ii = 0;
			while (1) {
				error_table_16 = *((unsigned __int16 *)error_table_32 - 4);
				++ii;
				if (Shifted >= error_table_16 && Shifted < error_table_16 + *((unsigned __int16 *)error_table_32 - 3))
					break;
				error_table_32 += 2;
				if (ii >= *(unsigned __int16 *)(error_table + 4))
					goto Facility_Found;
			}
			facility = (char *)(error_table_32 - 1);
		} else {
		Facility_Found:
			facility = (char *)error_table;
		}
	}
	else
		facility = "NS";
	error_init = *(unsigned __int16 *)(error_table + 12) + i;
	if (error_init) {
		Devider = error_init;
		ii = 0;
		do {
			Remainder = Devider % 10;
			Devider /= 10;
			ii += Remainder;
		} while (Devider);
		lastBit = ii % 10;
	}
	else
		lastBit = 0;
	snprintf(output_buffer, 16, "%s-%d-%1d", facility, error_init, lastBit);
	return 0;
}


int main(int argc, char *argv[]) {
	char error_code[0x10];
	int hex_error;

	if (argc == 1) {
		printf("ErrorResolver by Silica (with help from Princess Of Sleeping)\n");
		printf("Reading error_table.bin\n");
	}
	
	int *error_table = calloc(1, 1024 * 1024);
	error_table[2] = calloc(1, 1024 * 1024);
	error_table[4] = calloc(1, 1024 * 1024);

	int res = read_error_table("error_table.bin", error_table);
	if (res != 0)
		return res;

	if (argc == 1) {
		printf("%d facility groups found in error_table.bin\n", facility_group_num);
		printf("%d errors found in error_table.bin\n", error_table_num);
		printf("Done!\n");

		printf("\nArguments: <mode> <input>\n");
		printf("\tModes:\n\t-d Decode hex to shortcode\n");
		printf("\t-b Bruteforce hex from shortcode\n");
		printf("No arguments = interactive\n\n\n");

		printf("--Interactive Mode!\n");
		printf("Resolve hex to shortcode = 1\n");
		printf("Bruteforce shortcode to hex = 2\n");

		printf("\nEnter choice: ");
		int choice = 0;
		scanf("%i", &choice);
		if (choice == 1) {
			while (1) {
				printf("Enter Hex Code: ");
				scanf("%x", &hex_error);

				error_code_to_string(error_table, error_code, hex_error);
				printf("Short Code: %s\n", error_code);
			}
		} else if(choice == 2) {
			while (1) {
				int found = 0;
				char target_error_code[0x10];

				printf("Enter Short Code: ");
				scanf("%16s",&target_error_code);

				unsigned int* possible_options = (unsigned int*)error_table[4];
				for (int i = 0; i < error_table_num; i++) {
					hex_error = possible_options[i];
					error_code_to_string(error_table, error_code, hex_error);

					if (strcmp(error_code, target_error_code) == 0) {
						printf("Hex Code: %x\n", hex_error);
						found = 1;
						break;
					}
				}
				if (found == 0)
					printf("%s Was not found in error_table.bin (perhaps its E- facility?)\n",target_error_code);
			}
		}
	} else if (argc == 3) {
		char *mode = argv[1];
		if (strcmp(mode, "-d") == 0) {
			sscanf(argv[2],"%x", &hex_error);
			error_code_to_string(error_table, error_code, hex_error);
			printf("%s\n", error_code);
		} else if (strcmp(mode, "-b") == 0) {
			char target_error_code[0x10];
			strncpy(target_error_code, argv[2], 0x10);

			unsigned int* possible_options = (unsigned int*)error_table[4];
			for (int i = 0; i < error_table_num; i++) {
				hex_error = possible_options[i];
				error_code_to_string(error_table, error_code, hex_error);
				
				if (strcmp(error_code, target_error_code) == 0) {
					printf("%x\n", hex_error);
					return 0;
				}
			}
			printf("%s Was not found in error_table.bin (perhaps its E- facility?)\n", target_error_code);
			return -1;
		} else
			printf("Invalid arguments!\n");
	} else 
		printf("Invalid arguments!\n");
    return 0;
}
