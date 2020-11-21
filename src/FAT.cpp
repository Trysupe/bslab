//
// Created by user on 11.11.20.
//

#include <FAT.h>

// Constructor for the FAT
FAT::FAT(BlockDevice *device) {
    this->device = device;
    for (int i = 0; i < DATA_BLOCKS; i++) {
        fatArray[i] = FAT_EOF;
    }
}

// Destructor for the FAT
FAT::~FAT() {

}

void FAT::insertModifiedBlock(int index) {

    // 1. verify that index is not present already
    bool indexIsPresent = false;
    for (int i = 0; i < modifiedBlocksCounter; i++) {
        if (modifiedBlocks[i] == index) {
            indexIsPresent = true;
        }
    }

    // then track the change
    if (!indexIsPresent) {
        modifiedBlocks[modifiedBlocksCounter + 1] = index;
        modifiedBlocksCounter++;
    }
}

void FAT::clearModifiedBlocks() {
    for (int i = 0; i < modifiedBlocksCounter; i++) {
        modifiedBlocks[i] = 0;
        modifiedBlocksCounter = 0;
    }
}



// map the next block for the current file
int FAT::setNextBlock(int currentBlock, int nextBlock) {

    if (currentBlock == nextBlock) {
        return -1;
    }

    this->fatArray[currentBlock] = nextBlock;
    insertModifiedBlock(currentBlock);
    insertModifiedBlock(nextBlock);
    return 0;
}

// return the index of the next block in the chain
int FAT::getNextBlock(int currentBlock) {
    return this->fatArray[currentBlock];
}

// clean the content of a block by its index
void FAT::freeBlock(int index) {
    this->fatArray[index] = FAT_EOF;
    insertModifiedBlock(index);
}

// write the changes to the disk
void FAT::persist() {

}

// initialise the (existing) FAT
void FAT::initFAT() {
    char buffer[BLOCK_SIZE];
    for (int i = FAT_OFFSET; i < FAT_OFFSET + FAT_SIZE; i++) {
        device->read(i, buffer);
        memcpy(fatArray, buffer, BLOCK_SIZE);
    }
}

// initialise a FAT for an empty filesystem
void FAT::initialInitFAT() {
    char buffer[BLOCK_SIZE];
    for (int i = 0; i < FAT_SIZE; i++) {
        memcpy(buffer, fatArray, BLOCK_SIZE);
        device->write(FAT_OFFSET + i, buffer);
    }
}

