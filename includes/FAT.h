//
// Created by user on 11.11.20.
//

#ifndef MYFS_FAT_H
#define MYFS_FAT_H

#include "myfs-structs.h"

class FAT {
private:
    BlockDevice *device;
    int fatArray[NUM_DIR_ENTRIES][DATA_BLOCKS];  // keep track of the next data block assigned to this file
    int modifiedBlocks[];  // keep track of the modified blocks currently held in memory
public:
    FAT(BlockDevice *device);
    ~FAT();

    int setNextBlock(int currentBlock, int nextBlock);
    int getNextFreeBlock(int currentBlock);

    void freeBlock(int block);

    void persist();

    void initFAT();
    void initialInitFAT();

};


#endif //MYFS_FAT_H
