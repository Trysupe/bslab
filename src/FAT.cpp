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

// map the next block for the current file
int FAT::setNextBlock(int currentBlock, int nextBlock) {
    return 0;
}

// return the index of the next available block
int FAT::getNextFreeBlock(int currentBlock) {
    return 0;
}

// clean the content of a block by its index
void FAT::freeBlock(int block) {

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

