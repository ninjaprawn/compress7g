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


/* Change endian of a 4 bytes memory zone */
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

int main(int argc, char **argv) {
	int directories_size = 0;
	DIRECTORY* directories = NULL;

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
	 Create the DIRECTORIES so they are similar to the original
	 */
	char dirs[][5] = {"disk", "diag", "fv00", "appl", "lbat", "bdsw", "bdhw", "chrg", "gpfw", "rsrc", "osos"};
	/*Extracting from 0x    6000 to 0x   B2633 in disk.fw. (9cd)
	 Extracting from 0x   B3000 to 0x  2B4863 in diag.fw. (179d)
	 Extracting from 0x  2B6000 to 0x  2C7F63 in fv00.fw. (109d)
	 Extracting from 0x  2C9000 to 0x  2CCB13 in appl.fw. (14ed)
	 Extracting from 0x  2CE000 to 0x  2F9BA3 in lbat.fw. (145d)
	 Extracting from 0x  2FB000 to 0x  3487A3 in bdsw.fw. (85d)
	 Extracting from 0x  349000 to 0x  374BA3 in bdhw.fw. (145d)
	 Extracting from 0x  376000 to 0x  3A1BA3 in chrg.fw. (145d)
	 Extracting from 0x  3A3000 to 0x  3B31C3 in gpfw.fw. (e3d)
	 Extracting from 0x  3B4000 to 0x A2B5863 in rsrc.fw. (179d)
	 Extracting from 0x A2B7000 to 0x AC3EDC3 in osos.fw. (a3c)*/
	unsigned int dirsCount[11] = {0x1cc, 0xF9C, 0x89C, 0xCEC, 0xC5C, 0x5c, 0xC5C, 0xC5C, 0x63C, 0xF9C, 0xa3c};
	
	unsigned int currentOffset = 0x6000;
	
	for (int i = 0; i<11; i++) {
		DIRECTORY dir;
		strncpy(dir.dev,"NAND", 4);
		strncpy(dir.type,dirs[i], 4);
		dir.id = 0;
		
		char *filename = (char*)malloc(11);
		strcpy(filename, "fw/");
		strcat(filename, dirs[i]);
		strcat(filename, ".fw");
		FILE *f = fopen(filename, "rb");
		dir.len = fsize(f) - LEN_HEADER;
		
		dir.devOffset = currentOffset;
		currentOffset += dir.len+LEN_HEADER*2+1+dirsCount[i];
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
		dir.loadAddr = 0xFFFFFFFF;
		
		change_endian(dir.dev);
		change_endian(dir.type);
		directories_size++;
		directories = (DIRECTORY*)realloc(directories, directories_size*sizeof(DIRECTORY));
		memcpy(directories + directories_size - 1, &dir, sizeof(DIRECTORY));
	}
 
//	for(int i=0;i<directories_size; i++) print_directory_infos( &(directories[i]), 1 );
	
	const char* header = "\x7B\x7B\x7E\x7E\x20\x20\x2F\x2D\x2D\x2D\x2D\x2D\x5C\x20\x20\x20\x7B\x7B\x7E\x7E\x20\x2F\x20\x20\x20\x20\x20\x20\x20\x5C\x20\x20\x7B\x7B\x7E\x7E\x7C\x20\x20\x20\x20\x20\x20\x20\x20\x20\x7C\x20\x7B\x7B\x7E\x7E\x7C\x20\x53\x20\x54\x20\x4F\x20\x50\x20\x7C\x20\x7B\x7B\x7E\x7E\x7C\x20\x20\x20\x20\x20\x20\x20\x20\x20\x7C\x20\x7B\x7B\x7E\x7E\x20\x5C\x20\x20\x20\x20\x20\x20\x20\x2F\x20\x20\x7B\x7B\x7E\x7E\x20\x20\x5C\x2D\x2D\x2D\x2D\x2D\x2F\x20\x20\x20\x43\x6F\x70\x79\x72\x69\x67\x68\x74\x28\x43\x29\x20\x32\x30\x30\x31\x20\x41\x70\x70\x6C\x65\x20\x43\x6F\x6D\x70\x75\x74\x65\x72\x2C\x20\x49\x6E\x63\x2E\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x2D\x00\x5D\x69\x68\x5B\x00\x40\x00\x00\x0C\x01\x03\x00";
	
	char cert[2048];
	FILE *certF = fopen("verif", "rb");
	fread(cert, 2048, 1, certF);
	fclose(certF);
	
	FILE *test = fopen("Firmware.MSE", "wb");
	
	fwrite(header, 268,1,test); // Write header
	// Write padding
	for (int i = 269; i<=0x5000; i++) {
		fputc(0,test);
	}
	for (int i=0;i<directories_size; i++) {
		fwrite(&directories[i], sizeof(DIRECTORY), 1, test);
	}
	for (int i=0; i<5; i++) {
		fwrite("\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\xFF\xFF\xFF\xFF", sizeof(char), 40, test);
	}
	for (int i=0; i<0xD80; i++) {
		fputc(0,test);
	}
	for (int i=0;i<directories_size; i++) {
		char *filename = (char*)malloc(11);
		strcpy(filename, "fw/");
		strcat(filename, dirs[i]);
		strcat(filename, ".fw");
		FILE *f = fopen(filename, "rb");
		
		char *buff = (char*)malloc(directories[i].len+LEN_HEADER+1);
		
		fread(buff, sizeof(char), directories[i].len+LEN_HEADER, f);
		fwrite(buff, sizeof(char), directories[i].len+LEN_HEADER, test);
		fwrite(cert, sizeof(char), 2048, test);
		fputc(0,test);
		for (int j = 0; j <dirsCount[i];j++) {
			fputc(0,test);
		}
		fclose(f);
	}
	fclose(test);
	
	printf("done!\n");
	return 0;
}
