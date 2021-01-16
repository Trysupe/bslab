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
        modifiedBlocks[modifiedBlocksCounter] = index;
        modifiedBlocksCounter++;
    }
}

void FAT::clearModifiedBlocks() {
    for (int i = 0; i < modifiedBlocksCounter; i++) {
        modifiedBlocks[i] = 0;
    }
    modifiedBlocksCounter = 0;
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
    char buffer[BLOCK_SIZE];

    // go over each modified block
    // start at the second entry since the first entry always point to the second modified...
    // ...value and does not point anywhere if only one blockdevice is used
    for (int j = 1; j < modifiedBlocksCounter; j++) {

        // check which position we are at inside the blockdevices allocated for the FAT
        int blockdevice_offset = (modifiedBlocks[j-1]) / (BLOCK_SIZE / 4);

        // read the current targeted blockdata
        device->read(FAT_OFFSET + blockdevice_offset, buffer);

        int current_block_index = modifiedBlocks[j];


        // track iterations from 0 to 512 inside the current blockdevice
        int this_iteration = 0;


        // write the updated FAT, keep track over the blockdevice offset
        for (int k = 0 + ((BLOCK_SIZE / 4) * blockdevice_offset);
        k < BLOCK_SIZE + (BLOCK_SIZE * current_block_index); k++) {

            // if the iteration matches the modified block
            if (k == modifiedBlocks[j-1]) {
                // write each byte to the buffer, start from the left
                for (int l = 3; l >= 0; l--) {
                    // write the next block pointer
                    int this_byte = (current_block_index >> (8*l)) & 0xff;
                    int buffer_offset = (this_iteration*4) + l;
                    buffer[buffer_offset] = this_byte;

                }
                break;
            }

            this_iteration++;
        }
        device->write(FAT_OFFSET + blockdevice_offset, buffer);
    }

    clearModifiedBlocks();
}

// initialise the (existing) FAT
void FAT::initFAT() {
    char buffer[BLOCK_SIZE];
    for (int i = 0; i < FAT_SIZE; i++) {
            int i_offset = i + FAT_OFFSET;
            device->read(i_offset, buffer);
            memcpy(fatArray+(i * BLOCK_SIZE / 4), buffer, BLOCK_SIZE);
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

