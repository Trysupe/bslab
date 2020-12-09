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

    // get the index from the highest modified block (in global scope)
    int index_last_modified_block_index_in_fs = modifiedBlocks[modifiedBlocksCounter - 1];

    // 4 because of int32_t which equals 4 bytes
    int iterate_x_times_to_reach_last_block_index = index_last_modified_block_index_in_fs / (BLOCK_SIZE / 4);

    // reach the last block
    for (int i = 0; i <= iterate_x_times_to_reach_last_block_index; i++) {

        // init the buffer with '-1' everywhere
        // FF FF FF FF equals -1 in two's complement....
        // this breaks the purpose of 'FAT_EOF' but oh well ¯\_(ツ)_/¯
        for (int j = 0; j < BLOCK_SIZE / 4; j++) {
            buffer[(4 * j) + 0] = 255;
            buffer[(4 * j) + 1] = 255;
            buffer[(4 * j) + 2] = 255;
            buffer[(4 * j) + 3] = 255;
        }

        // go over each modified block (while doing so redundantly)
        // start at the second entry since the first entry always point to the second modified
        // value and does not point anywhere if only one blockdevice is used
        for (int j = 1; j < modifiedBlocksCounter; j++) {

            int current_block_index = modifiedBlocks[j];

            // write the updated FAT
            for (int k = 0; k < BLOCK_SIZE; k++) {

                // if the iteration matches the modified block
                //
                if (k == modifiedBlocks[j-1]) {
                    // write each byte to the buffer, start from the left
                    for (int l = 3; l >= 0; l--) {
                        // write the next block pointer
                        int this_byte = (current_block_index >> (8*l)) & 0xff;
                        int buffer_offset = (k*4) + l;
                        buffer[buffer_offset] = this_byte;

                    }
                }
            }
        }
        device->write(FAT_OFFSET + i, buffer);
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

