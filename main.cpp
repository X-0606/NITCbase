#include "Buffer/StaticBuffer.h"
#include "Cache/OpenRelTable.h"
#include "Cache/RelCacheTable.h"
#include "Cache/AttrCacheTable.h"
#include "Disk_Class/Disk.h"
#include "FrontendInterface/FrontendInterface.h"
#include <cstring>
#include <cstdio>

int main(int argc, char *argv[]) {

    Disk disk_run;
    StaticBuffer buffer;
    OpenRelTable cache;

  // unsigned char blockBuffer[BLOCK_SIZE];
  // for(int i=0;i<4;i++){
  //   Disk::readBlock(blockBuffer,i);
  //   for(int j=0;j<BLOCK_SIZE;j++){
  //    printf("%d",blockBuffer[j]);
  //   }
  // }

    return FrontendInterface::handleFrontend(argc, argv);
}