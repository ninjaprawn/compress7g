/* compress7g by George Dan */
/* Contains code from extract2g */
#define ADDR_DIR_3G      0x5000
#define LEN_HEADER 0x800

#define LEN_DIR sizeof(DIRECTORY)

typedef struct _DIRECTORY {
  char dev[4];				// Type of directory. NAND or ATA!
  char type[4];				// Name of the directory

  unsigned int id;			//
  unsigned int devOffset;	//
  unsigned int len;			//
  unsigned int addr;		//

  unsigned int entryOffset;	//
  unsigned int checksum;	//
  unsigned int version;		//
  unsigned int loadAddr;	//

} DIRECTORY;
