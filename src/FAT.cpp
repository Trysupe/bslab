//
// Created by user on 11.11.20.
//

#include <FAT.h>

// Constructor for the FAT
FAT::FAT(BlockDevice *device) {

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

}

// initialise a FAT for an empty filesystem
void FAT::initialInitFAT() {

}
