//
// Created by user on 11.11.20.
//

#ifndef MYFS_FAT_H
#define MYFS_FAT_H

#include "myfs-structs.h"

class FAT {
private:
    BlockDevice *device;
    // map the entry to the next corresponding data block of a file
    // and write 'FAT_EOF' to mark the last block of a file
    int32_t fatArray[DATA_BLOCKS];
    int modifiedBlocks[DATA_BLOCKS];  // keep track of the modified blocks currently held in memory
    int modifiedBlocksCounter = 0;
public:
    FAT(BlockDevice *device);
    ~FAT();

    void insertModifiedBlock(int index);
    void clearModifiedBlocks();

    int setNextBlock(int currentBlock, int nextBlock);
    int getNextBlock(int currentBlock);

    void freeBlock(int index);

    void persist();

    void initFAT();
    void initialInitFAT();

};


#endif //MYFS_FAT_H
