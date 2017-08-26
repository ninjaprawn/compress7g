/*
 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License version 2.1 as
 published by the Free Software Foundation.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation Inc., 59 Temple Place, Suite 330, Boston MA 02111-1307 USA
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <limits.h>

#include "compress7g.h"

/* Print file position and log message */
void logpos(char *msg, FILE *f) {
	printf("[0x%8lX] %s", ftell(f), msg);
}

/* Change endian of a 4 byte memory zone */
void change_endian( char* buf ) {
	char tmp[4];

	tmp[3] = buf[0];
	tmp[2] = buf[1];
	tmp[1] = buf[2];
	tmp[0] = buf[3];
	
	memcpy(buf,tmp,4*sizeof(char));
}

void print_directory_infos(DIRECTORY* p_dir, char extended) {
  if( extended )
    printf("dev: %.4s type: %.4s\n\
id: %X\n\
devOffset: %X\n\
len: %X\n\
addr: %X\n\
entryOffset: %X\n\
checksum: %X\n\
version: %X\n\
loadAddr: %X\n\n", p_dir->dev, p_dir->type, p_dir->id, p_dir->devOffset, p_dir->len, p_dir->addr, p_dir->entryOffset, p_dir->checksum, p_dir->version, p_dir->loadAddr );

	else printf("dev: %.4s type: %.4s devOffset: %X len: %X\n", p_dir->dev, p_dir->type, p_dir->devOffset, p_dir->len);

}

int fsize(FILE *fp){
	int prev=ftell(fp);
	fseek(fp, 0L, SEEK_END);
	int sz=ftell(fp);
	fseek(fp,prev,SEEK_SET);
	return sz;
}

/* Display help */
void usage(int status) {
	// if (status != EXIT_SUCCESS) {}
	fputs("\ncompress7g 1.0_beta\n", stdout);
	fputs("Copyright (c) 2017 ninjaprawn.\n", stdout);
	fputs("Usage: compress7g [OPTION]\n", stdout);
	fputs("Build Firmware.MSE for iPod Nano 6G/7G.\n", stdout);
	fputs("Expects a directory named 'fw' containing firmware parts.\n", stdout);
    fprintf(stdout, "\n\
Options:\n\
        -6, --nano6g-compat      use iPod Nano 6G mode\n\
        -h, --help               display this help and exit\n\
        -v, --version            display version information and exit\n\n");
	exit(status);
}

int main(int argc, char **argv) {
	char nano6g_compat = 0;
	
	/* Getopt short options */
	static char const short_options[] = "6hv";

	/* Getopt long options */
	static struct option const long_options[] = {
		{"help", no_argument, NULL, 'h'},
		{"nano6g-compat", no_argument, NULL, '6'},
		{"version", no_argument, NULL, 'v'},
		{NULL, 0, NULL, 0}
	};
	
	/* Parse command line options */
	char c;
	while ((c = getopt_long(argc, argv, short_options, long_options, NULL)) != -1) {
		switch (c) {
		    case '6':
				nano6g_compat = 1;
				break;
		    case 'h':
				usage(EXIT_SUCCESS);
				break;
		    case 'v':
				printf("Compiled at %s %s.\n", __TIME__, __DATE__);
				usage(EXIT_SUCCESS);
				break;
			default:
				usage(EXIT_FAILURE);
				break;
		}
	}
	
	printf("\nUsing Nano 6G mode: %s \n\n", nano6g_compat?"Yes":"No");
	
//------------------------------------------------------------------------------

	printf("Building directory metadata... ");
	
	/*
	 dev: NAND
	 type: <name>
	 id:0
	 devOffset: Offset from beginning of file
	 len: size of file (bytes)
	 addr: 8000000 (disk,diag,fv00,osos), 0 (rest)
	 entryOffset: 400 (rsrc), 0 (rest)
	 checksum: 0
	 version: 0 (rsrc), 1E000 (rest)
	 loadAddr: FFFFFFFF
	*/
	
	/*
	 Create the DIRECTORIES so they are similar to the original.
	 Nano 7G firmware has 11 partitions.
	 Nano 6G firmware has 9 patitions.
	*/
	int directories_size = 0;
	DIRECTORY* directories = NULL;
	
	const char** dirs = (nano6g_compat) ?
		(const char*[]){ "disk", "diag", "appl", "lbat", "bdsw", "bdhw", "chrg", "rsrc", "osos" } :
		(const char*[]){ "disk", "diag", "fv00", "appl", "lbat", "bdsw", "bdhw", "chrg", "gpfw", "rsrc", "osos" };
	
	/*
	 extract2g output (Nano 7G)
	 Extracting from 0x    6000 to 0x   B2633 in disk.fw. (9cd)
	 Extracting from 0x   B3000 to 0x  2B4863 in diag.fw. (179d)
	 Extracting from 0x  2B6000 to 0x  2C7F63 in fv00.fw. (109d)
	 Extracting from 0x  2C9000 to 0x  2CCB13 in appl.fw. (14ed)
	 Extracting from 0x  2CE000 to 0x  2F9BA3 in lbat.fw. (145d)
	 Extracting from 0x  2FB000 to 0x  3487A3 in bdsw.fw. (85d)
	 Extracting from 0x  349000 to 0x  374BA3 in bdhw.fw. (145d)
	 Extracting from 0x  376000 to 0x  3A1BA3 in chrg.fw. (145d)
	 Extracting from 0x  3A3000 to 0x  3B31C3 in gpfw.fw. (e3d)
	 Extracting from 0x  3B4000 to 0x A2B5863 in rsrc.fw. (179d)
	 Extracting from 0x A2B7000 to 0x AC3EDC3 in osos.fw. (a3c)
	*/
	
	/*
	 extract2g -4 output (Nano 6G)
	 Extracting from 0x    7000 to 0x   AAA13 in disk.fw. (15ed)
	 Extracting from 0x   AC000 to 0x  22D863 in diag.fw. (179d)
	 Extracting from 0x  22F000 to 0x  232B13 in appl.fw. (14ed)
	 Extracting from 0x  234000 to 0x  25FBA3 in lbat.fw. (145d)
	 Extracting from 0x  261000 to 0x  28CBA3 in bdsw.fw. (145d)
	 Extracting from 0x  28E000 to 0x  2B9BA3 in bdhw.fw. (145d)
	 Extracting from 0x  2BB000 to 0x  2E6BA3 in chrg.fw. (145d)
	 Extracting from 0x  2E8000 to 0x 9FE9863 in rsrc.fw. (179d)
	 Extracting from 0x 9FEB000 to 0x A888B33 in osos.fw. (???)
	*/
		
	int *dirPadding = (nano6g_compat) ?
		(int[]){ 0xdec, 0xF9C, 0xcec, 0xc5c, 0xc5c, 0xc5c, 0xc5c, 0x0f9c, 0x4cc } :
		(int[]){ 0x1cc, 0xF9C, 0x89C, 0xCEC, 0xC5C, 0x5c, 0xC5C, 0xC5C, 0x63C, 0xF9C, 0xa3c };
	
	int currentOffset = 0x6000;
	
	int dirCount = (nano6g_compat) ? 9 : 11;
	
	// these are wrong endian
	unsigned int *loadAddrs = (nano6g_compat) ?
		(unsigned int[]){ 0x7FDF0008, 0x7FD10008, 0x006F0668, 0x006F0668, 0x006F0668, 0x006F0668, 0x006F0668, 0x634b0008, 0x7ee50008 } :
		(unsigned int[]){ 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff };
	
	for (int i = 0; i < dirCount; i++) {
		DIRECTORY dir;
		strncpy(dir.dev,"NAND", 4);
		strncpy(dir.type, dirs[i], 4);
		dir.id = 0;
		
		char *filename = (char*)malloc(11);
		strcpy(filename, "fw/");
		strcat(filename, dirs[i]);
		strcat(filename, ".fw");
		
		FILE *f = fopen(filename, "rb");
		if (f == NULL) {
			printf("Error: file not found: %s \n\n", filename);
			exit(0);
		}
		
		dir.len = fsize(f) - LEN_HEADER;
		
		dir.devOffset = currentOffset;
		currentOffset += dir.len+LEN_HEADER*2+1+dirPadding[i];
		if (!strcmp(dirs[i], "disk") || !strcmp(dirs[i], "diag") || !strcmp(dirs[i], "fv00") || !strcmp(dirs[i], "osos")) {
			dir.addr = 0x8000000;
		} else {
			dir.addr = 0x0;
		}
		if (!strcmp(dirs[i], "rsrc")) {
			dir.entryOffset = 0x400;
			dir.version = 0x0;
		} else {
			dir.entryOffset = 0x0;
			dir.version = 0x1E000;
		}
		dir.checksum = 0x0;
		dir.loadAddr = loadAddrs[i];
		change_endian(dir.dev);
		change_endian(dir.type);
		directories_size++;
		directories = (DIRECTORY*)realloc(directories, directories_size*sizeof(DIRECTORY));
		memcpy(directories + directories_size - 1, &dir, sizeof(DIRECTORY));
	}
	// printf("\nDEBUG ----------------------------------------\n");
	// for(int i=0;i<directories_size; i++) print_directory_infos( &(directories[i]), 1 ); //DEBUG
	// printf("\n----------------------------------------------\n\n");
	
	printf("Done\n");
	
//------------------------------------------------------------------------------

	printf("Building Firmware.MSE... \n");
	
	const char* header = "\x7B\x7B\x7E\x7E\x20\x20\x2F\x2D\x2D\x2D\x2D\x2D\x5C\x20\x20\x20\x7B\x7B\x7E\x7E\x20\x2F\x20\x20\x20\x20\x20\x20\x20\x5C\x20\x20\x7B\x7B\x7E\x7E\x7C\x20\x20\x20\x20\x20\x20\x20\x20\x20\x7C\x20\x7B\x7B\x7E\x7E\x7C\x20\x53\x20\x54\x20\x4F\x20\x50\x20\x7C\x20\x7B\x7B\x7E\x7E\x7C\x20\x20\x20\x20\x20\x20\x20\x20\x20\x7C\x20\x7B\x7B\x7E\x7E\x20\x5C\x20\x20\x20\x20\x20\x20\x20\x2F\x20\x20\x7B\x7B\x7E\x7E\x20\x20\x5C\x2D\x2D\x2D\x2D\x2D\x2F\x20\x20\x20\x43\x6F\x70\x79\x72\x69\x67\x68\x74\x28\x43\x29\x20\x32\x30\x30\x31\x20\x41\x70\x70\x6C\x65\x20\x43\x6F\x6D\x70\x75\x74\x65\x72\x2C\x20\x49\x6E\x63\x2E\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x00\x5D\x69\x68\x5B\x00\x40\x00\x00\x0C\x01\x03\x00";
	
	// Read certificate file
	char cert[2048];
	FILE *certF = fopen("verif", "rb");
	if (certF == NULL) {
		printf("Error: cert file not found \n\n");
		exit(0);
	}
	fread(cert, 2048, 1, certF);
	fclose(certF);
	
	
	// Start writing Firmware.MSE ...
	
	FILE *test = fopen("Firmware.MSE", "wb");
	
	// Write header
	logpos("writing header\n", test);
	fwrite(header, 268, 1, test);
	
	// Write padding
	logpos("writing padding\n", test);
	for (int i = 269; i<=0x5000; i++) {
		fputc(0,test);
	}
	
	// Write directory list
	logpos("writing directory list\n", test);
	for (int i=0;i<directories_size; i++) {
		fwrite(&directories[i], sizeof(DIRECTORY), 1, test);
	}
	
	// Write dotted lines
	logpos("writing dotted lines\n", test);
	int numDottedLines = nano6g_compat ? 7 : 5;
	for (int i=0; i<numDottedLines; i++) {
		fwrite("\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\xFF\xFF\xFF\xFF", sizeof(char), 40, test);
	}
	
	// Write some more padding
	logpos("writing more padding\n", test);
	for (int i=0; i<0xD80; i++) {
		fputc(0,test);
	}
	
	// Write some 6G-specific stuff here
	if (nano6g_compat) {
		logpos("writing some 6g stuff here\n", test);
		for (int i = ftell(test); i <= 0x6003; i++) {
			fputc(0, test);
		}
		fputc(4, test);
		fputc(0, test);
		fputc(0, test);
		fputc(0, test);
		fputc(0, test);
		fputc(0, test);
		fputc(0, test);
		fputc(0, test);
		fputc(0, test);
		fputc(0, test);
		fputc(0, test);
		fputc(0, test);
		fputc(0, test);
		fputc(0, test);
		fputc(0, test);
		fputc(0, test);
		
		fputc(0x20, test);
		fputc(0x32, test);
		fputc(0x0A, test);
		fputc(0, test);
		
		for (int i = ftell(test); i < 0x6200; i++) {
			fputc(0, test);
		}
		for (int i = ftell(test); i < 0x7000; i++) {
			fputc(0xFF, test);
		}
	}
	// we should be at [0x6000] for 7G, [0x7000] for 6G
	
	// 6G has a different style of padding
	unsigned int padding0[9] = { 0x7ed, 0x99d, 0x6ed, 0x65d, 0x65d, 0x65d, 0x65d, 0x99d, 0x4cd };
	unsigned int paddingF[9] = { 0xE00, 0xE00, 0xE00, 0xE00, 0xE00, 0xE00, 0xE00, 0xE00, 0 };

	// Write fw files
	for (int i=0; i < directories_size; i++) {
		logpos("", test);
		printf("writing part %d: ", i+1);
		char *filename = (char*)malloc(11);
		strcpy(filename, "fw/");
		strcat(filename, dirs[i]);
		strcat(filename, ".fw");
		FILE *f = fopen(filename, "rb");
		
		char *buff = (char*)malloc(directories[i].len+LEN_HEADER+1);
		fread(buff, sizeof(char), directories[i].len+LEN_HEADER, f);
		fwrite(buff, sizeof(char), directories[i].len+LEN_HEADER, test);
		printf("fw; ");

		// if (!(nano6g_compat && (i == (directories_size-1)))) {
		// 	fwrite(cert, sizeof(char), 2048, test);
		// 	printf("cert; ");
		// }
		
		if (nano6g_compat) {
			for (int j = 0; j < padding0[i]; j++) {
				fputc(0, test);
			}
			for (int j = 0; j < paddingF[i]; j++) {
				fputc(0xFF, test);
			}
		} else {
			fputc(0, test);
			for (int j = 0; j < dirPadding[i]; j++) {
				fputc(0,test);
			}
		}
		printf("padding\n");
		
		fclose(f);
	}
	float finalSize = ftell(test) / 1024.0 / 1024.0;
	fclose(test);
	
	printf("Wrote Firmware.MSE (%.1fMB)\n\n", finalSize);
	
	printf("Finished!\n\n");
	return 0;
}
