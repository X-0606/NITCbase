#include "Buffer/StaticBuffer.h"
#include "Cache/OpenRelTable.h"
#include "Cache/RelCacheTable.h"
#include "Cache/AttrCacheTable.h"
#include "Disk_Class/Disk.h"
#include "FrontendInterface/FrontendInterface.h"
#include <cstring>
#include <cstdio>

int main(int argc, char *argv[]) {

    Disk disk_run;   // initializes disk system
    StaticBuffer buffer;
    OpenRelTable cache;

/*
    // UPDATE ATTRIBUTE NAME 

    int updated = 0;
    int curr = ATTRCAT_BLOCK;

    while (curr != -1) {

        RecBuffer attrCatBuffer(curr);
        HeadInfo attrCatHeader;
        attrCatBuffer.getHeader(&attrCatHeader);

        unsigned char buffer[BLOCK_SIZE];
        Disk::readBlock(buffer, curr);

        int attrCount = attrCatHeader.numAttrs;
        int slotCount = attrCatHeader.numSlots;
        int recordSize = attrCount * ATTR_SIZE;

        for (int j = 0; j < attrCatHeader.numEntries; j++) {

            unsigned char *recPtr =
                buffer + HEADER_SIZE + slotCount + (recordSize * j);

            Attribute attrRec[ATTRCAT_NO_ATTRS];
            memcpy(attrRec, recPtr, recordSize);

            if (strcmp(attrRec[ATTRCAT_REL_NAME_INDEX].sVal, "Students") == 0 &&
                strcmp(attrRec[ATTRCAT_ATTR_NAME_INDEX].sVal, "Class") == 0) {

                strcpy(attrRec[ATTRCAT_ATTR_NAME_INDEX].sVal, "Batch");

                memcpy(recPtr, attrRec, recordSize);
                Disk::writeBlock(buffer, curr);

                updated = 1;
                break;
            }
        }

        if (updated) {
            printf("changed\n");
            break;
        }

        curr = attrCatHeader.rblock;
    }

    printf("\nStudent schema AFTER update\n\n");

    

    RecBuffer relCatBuffer(RELCAT_BLOCK);
    HeadInfo relCatHeader;
    relCatBuffer.getHeader(&relCatHeader);

    int totalRelations = relCatHeader.numEntries;

    for (int i = 0; i < totalRelations; i++) {

        Attribute relCatRecord[RELCAT_NO_ATTRS];
        relCatBuffer.getRecord(relCatRecord, i);

        char *relName = relCatRecord[RELCAT_REL_NAME_INDEX].sVal;
        printf("Relation: %s\n", relName);

        int currAttrBlock = ATTRCAT_BLOCK;

        while (currAttrBlock != -1) {

            RecBuffer attrCatBuffer(currAttrBlock);
            HeadInfo attrCatHeader;
            attrCatBuffer.getHeader(&attrCatHeader);

            int entries = attrCatHeader.numEntries;

            for (int j = 0; j < entries; j++) {

                Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
                attrCatBuffer.getRecord(attrCatRecord, j);

                if (strcmp(
                        attrCatRecord[ATTRCAT_REL_NAME_INDEX].sVal,
                        relName
                    ) == 0) {

                    const char *attrName =
                        attrCatRecord[ATTRCAT_ATTR_NAME_INDEX].sVal;

                    const char *attrType =
                        (attrCatRecord[ATTRCAT_ATTR_TYPE_INDEX].nVal == NUMBER)
                        ? "NUM"
                        : "STR";

                    printf("  %s: %s\n", attrName, attrType);
                }
            }

            currAttrBlock = attrCatHeader.rblock;
        }

        printf("\n");
    }
*/



    //Cache Implementation

//   RelCatEntry relCatEntry;
//   AttrCatEntry attrCatEntry;

//   for (int i = 0; i <= 2; i++) {

//     // get relation catalog entry from relation cache
//     int ret = RelCacheTable::getRelCatEntry(i, &relCatEntry);
//     if (ret != SUCCESS) {
//       printf("Error fetching relation catalog entry for relId %d\n", i);
//       continue;
//     }

//     printf("Relation: %s\n", relCatEntry.relName);

//     // for each attribute of the relation
//     for (int j = 0; j < relCatEntry.numAttrs; j++) {

//       ret = AttrCacheTable::getAttrCatEntry(i, j, &attrCatEntry);
//       if (ret != SUCCESS) {
//         printf("  Error fetching attribute at offset %d\n", j);
//         continue;
//       }

//       printf("  %s: %s\n",
//              attrCatEntry.attrName,
//              attrCatEntry.attrType?"STR":"NUM");
//     }
//   }
//   unsigned char blockBuffer[BLOCK_SIZE];
//   for(int i=0;i<4;i++){
//     Disk::readBlock(blockBuffer,i);
//     for(int j=0;j<BLOCK_SIZE;j++){
//      printf("%d",blockBuffer[j]);
//     }
//   }

    return FrontendInterface::handleFrontend(argc, argv);
}