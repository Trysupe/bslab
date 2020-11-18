//
// Created by user on 11.11.20.
//

#ifndef MYFS_DMAP_H
#define MYFS_DMAP_H


#include "blockdevice.h"
#include "myfs-structs.h"

class DMap {
private:
    BlockDevice *device;
    bool blocks[DATA_BLOCKS];  // array of all blocks which contains true if the block is in use
    int freeBlockCounter = DATA_BLOCKS;  // keep track of all blocks currently available to use

public:
    DMap(BlockDevice *device);
    ~DMap();

    int getNextFreeBlock();
    int getXAmountOfFreeBlocks(int amount);

    void setBlockState(int dataBlockNum, bool isUsed);
    bool getBlockState(int dataBlockNum);

    void increaseFreeBlockCounterBy(int amount);
    void decreaseFreeBlockCounterBy(int amount);
    int getFreeBlockCounter();

    bool persist();

    void initDMap();
    void initialInitDMap();

};



#endif //MYFS_DMAP_H
