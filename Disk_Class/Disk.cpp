#include "Disk.h"

#include <fstream>
#include <iostream>

#include <cstring>

#include "../define/constants.h"

/*
 * Used to make a temporary copy of the disk contents before the starting of a new session.
 * This ensures that if the system has a forced shutdown during the course of the session,
 * the previous state of the disk is not lost.
 */
Disk::Disk() {
  /* An efficient method to copy files */
  /* Copy Disk to Disk Run Copy */
  std::ifstream src(DISK_PATH, std::ios::binary);
  std::ofstream dst(DISK_RUN_COPY_PATH, std::ios::binary);

  dst << src.rdbuf();
  src.close();
  dst.close();
}

/*
 * Used to update the changes made to the disk on graceful termination of the latest session.
 * This ensures that these changes are visible in future sessions.
 */
Disk::~Disk() {
  /* An efficient method to copy files */
  /* Copy Disk Run Copy to Disk */
  std::ifstream src(DISK_RUN_COPY_PATH, std::ios::binary);
  std::ofstream dst(DISK_PATH, std::ios::binary);

  dst << src.rdbuf();
  src.close();
  dst.close();
}

int Disk::createDisk() {
	FILE *disk = fopen(&DISK_PATH[0], "wb+");
	if(disk == nullptr)
		return FAILURE;
	fseek(disk, 0, SEEK_SET);

	// 16 MB
	for(int i=0; i < DISK_SIZE; i++){
		fputc(0,disk);
	}

	fclose(disk);
	return SUCCESS;
}

/*
 * Used to Read a specified block from disk
 * block - Memory pointer of the buffer to which the block contents is to be loaded/read.
 *         (MUST be Allocated by caller)
 * blockNum - Block number of the disk block to be read.
 */
int Disk::readBlock(unsigned char *block, int blockNum) {
  FILE *disk = fopen(DISK_RUN_COPY_PATH, "rb");
  if (blockNum < 0 || blockNum > DISK_BLOCKS - 1) {
    return E_OUTOFBOUND;
  }
  const int offset = blockNum * BLOCK_SIZE;
  fseek(disk, offset, SEEK_SET);
  fread(block, BLOCK_SIZE, 1, disk);
  fclose(disk);
  return SUCCESS;
}

/*
 * Used to Write a specified block from disk
 * block - Memory pointer of the buffer to which contain the contents to be written.
 *         (MUST be Allocated by caller)
 * blockNum - Block number of the disk block to be written into.
 */
int Disk::writeBlock(unsigned char *block, int blockNum) {
  FILE *disk = fopen(DISK_RUN_COPY_PATH, "rb+");
  if (blockNum < 0 || blockNum > DISK_BLOCKS - 1) {
    return E_OUTOFBOUND;
  }
  const int offset = blockNum * BLOCK_SIZE;
  fseek(disk, offset, SEEK_SET);
  fwrite(block, BLOCK_SIZE, 1, disk);
  fclose(disk);
  return SUCCESS;
}


void Disk::formatDisk() {
	FILE *disk = fopen(&DISK_PATH[0], "wb+");
	const int reserved_blocks = 6;
	const int offset = DISK_SIZE;

	fseek(disk, 0, SEEK_SET);
	unsigned char blockAllocationMap[BLOCK_SIZE * BLOCK_ALLOCATION_MAP_SIZE];

	// reserved_blocks Entries in Block Allocation Map (Used)
	for (int i = 0; i < reserved_blocks; i++) {
		if (i >= 0 && i <= 3)
            blockAllocationMap[i] = (unsigned char) BMAP;
		else
            blockAllocationMap[i] = (unsigned char) REC;
	}

	// Remaining Entries in Block Allocation Map are marked Unused
	for (int i = reserved_blocks; i < BLOCK_SIZE * BLOCK_ALLOCATION_MAP_SIZE; i++)
        blockAllocationMap[i] = (unsigned char) UNUSED_BLK;
	fwrite(blockAllocationMap, BLOCK_SIZE * BLOCK_ALLOCATION_MAP_SIZE, 1, disk);

	// Remaining Locations of Disk initialised to 0
	for (int i = BLOCK_SIZE * BLOCK_ALLOCATION_MAP_SIZE; i < offset; i++) {
		fputc(0, disk);
	}
	fclose(disk);

    Disk::add_disk_metainfo();
}


static void writeHeader(FILE* disk, int blockNum, HeadInfo* head) {
    fseek(disk, blockNum * BLOCK_SIZE, SEEK_SET);
    fwrite(head, sizeof(HeadInfo), 1, disk);
}


static void writeSlotmap(FILE* disk, int blockNum, unsigned char* slotmap) {
    fseek(disk, blockNum * BLOCK_SIZE + sizeof(HeadInfo), SEEK_SET);
    fwrite(slotmap, SLOTMAP_SIZE_RELCAT_ATTRCAT, 1, disk);
}


static void writeRecord(FILE* disk, int blockNum, int slotNum, Attribute* rec) {
    int offset = blockNum * BLOCK_SIZE
               + sizeof(HeadInfo)
               + SLOTMAP_SIZE_RELCAT_ATTRCAT
               + slotNum * NO_OF_ATTRS_RELCAT_ATTRCAT * sizeof(Attribute);
    fseek(disk, offset, SEEK_SET);
    fwrite(rec, NO_OF_ATTRS_RELCAT_ATTRCAT * sizeof(Attribute), 1, disk);
}

void Disk::add_disk_metainfo() {
    FILE* disk = fopen(DISK_PATH, "rb+");

    HeadInfo head;
    Attribute rec[NO_OF_ATTRS_RELCAT_ATTRCAT];
    unsigned char slotmap[SLOTMAP_SIZE_RELCAT_ATTRCAT];

    /*BLOCK 4 : RELATION CATALOG  */
    head.blockType  = REC;
    head.pblock     = -1;
    head.lblock     = -1;
    head.rblock     = -1;
    head.numEntries = 2;
    head.numAttrs   = NO_OF_ATTRS_RELCAT_ATTRCAT;
    head.numSlots   = SLOTMAP_SIZE_RELCAT_ATTRCAT;
    writeHeader(disk, 4, &head);

    slotmap[0] = SLOT_OCCUPIED;
    slotmap[1] = SLOT_OCCUPIED;
    for (int i = 2; i < SLOTMAP_SIZE_RELCAT_ATTRCAT; i++)
        slotmap[i] = SLOT_UNOCCUPIED;
    writeSlotmap(disk, 4, slotmap);

    // Slot 0: entry for RELATIONCAT
    memset(rec, 0, sizeof(rec));
    strcpy(rec[0].sVal, "RELATIONCAT");
    rec[1].nVal = 6;
    rec[2].nVal = 2;
    rec[3].nVal = 4;
    rec[4].nVal = 4;
    rec[5].nVal = 20;
    writeRecord(disk, 4, 0, rec);

    // Slot 1: entry for ATTRIBUTECAT
    memset(rec, 0, sizeof(rec));
    strcpy(rec[0].sVal, "ATTRIBUTECAT");
    rec[1].nVal = 6;
    rec[2].nVal = 2;
    rec[3].nVal = 5;
    rec[4].nVal = 5;
    rec[5].nVal = 20;
    writeRecord(disk, 4, 1, rec);

    // BLOCK 5 : ATTRIBUTE CATALOG
    head.blockType  = REC;
    head.pblock     = -1;
    head.lblock     = -1;
    head.rblock     = -1;
    head.numEntries = 12;
    head.numAttrs   = 6;
    head.numSlots   = 20;
    writeHeader(disk, 5, &head);

    for (int i = 0; i < SLOTMAP_SIZE_RELCAT_ATTRCAT; i++)
        slotmap[i] = (i < 12) ? SLOT_OCCUPIED : SLOT_UNOCCUPIED;
    writeSlotmap(disk, 5, slotmap);

    // RELATIONCAT attributes
    memset(rec, 0, sizeof(rec));
    strcpy(rec[0].sVal, "RELATIONCAT");
     strcpy(rec[1].sVal, "RelName");
    rec[2].nVal = STRING;
    rec[3].nVal = -1;
    rec[4].nVal = -1;
    rec[5].nVal = 0;
    writeRecord(disk, 5, 0, rec);

    strcpy(rec[1].sVal, "#Attributes");
    rec[5].nVal = 1;
    writeRecord(disk, 5, 1, rec);

    strcpy(rec[1].sVal, "#Records");
    rec[2].nVal = NUMBER;
    rec[5].nVal = 2;
    writeRecord(disk, 5, 2, rec);

    strcpy(rec[1].sVal, "FirstBlock");
    rec[5].nVal = 3;
    writeRecord(disk, 5, 3, rec);

    strcpy(rec[1].sVal, "LastBlock");
    rec[5].nVal = 4;
    writeRecord(disk, 5, 4, rec);

    strcpy(rec[1].sVal, "#Slots");
    rec[5].nVal = 5;
    writeRecord(disk, 5, 5, rec);

    //ATTRIBUTECAT attributes
    memset(rec, 0, sizeof(rec));
    strcpy(rec[0].sVal, "ATTRIBUTECAT"); 
    strcpy(rec[1].sVal, "RelName");
    rec[2].nVal = STRING;
    rec[3].nVal = -1; 
    rec[4].nVal = -1;
    rec[5].nVal = 0;
    writeRecord(disk, 5, 6, rec);

    strcpy(rec[1].sVal, "AttributeName");
    rec[5].nVal = 1;
    writeRecord(disk, 5, 7, rec);

    strcpy(rec[1].sVal, "AttributeType");
    rec[2].nVal = NUMBER;
    rec[5].nVal = 2;
    writeRecord(disk, 5, 8, rec);

    strcpy(rec[1].sVal, "PrimaryFlag");
    rec[5].nVal = 3;
    writeRecord(disk, 5, 9, rec);

    strcpy(rec[1].sVal, "RootBlock");
    rec[2].nVal = NUMBER;
    rec[5].nVal = 4;
    writeRecord(disk, 5, 10, rec);

    strcpy(rec[1].sVal, "Offset");
    rec[5].nVal = 5;
    writeRecord(disk, 5, 11, rec);

    fclose(disk);
}