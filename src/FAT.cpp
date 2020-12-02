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

    int data_offset = DATA_OFFSET;
    this->fatArray[currentBlock - data_offset] = nextBlock;
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
    // get the index from the highest modified block (in fat list scope)
    int data_offset = DATA_OFFSET;
    int index_last_modified_block_index_in_data_block_scope = index_last_modified_block_index_in_fs - data_offset;

    // 4 because of int32_t which equals 4 bytes
    int iterate_x_times_to_reach_last_block_index = index_last_modified_block_index_in_data_block_scope / (BLOCK_SIZE / 4);

    // reach the last block
    for (int i = 0; i <= iterate_x_times_to_reach_last_block_index; i++) {

        // go over each modified block (while doing so redundantly)
        for (int j = 0; j < modifiedBlocksCounter; j++) {

            int current_block_index = modifiedBlocks[j] - data_offset;

            // write the updated FAT
            for (int k = 0; k < BLOCK_SIZE; k++) {

                // if the iteration matches the modified block
                if (k == current_block_index) {
                    int counter_l = 0;  // counter count to assign last byte to the left
                    // write each byte to the buffer, start from the left
                    for (int l = 3; l >= 0; l--) {
                        int this_byte = (current_block_index >> (8*l)) & 0xff;
                        int buffer_offset = (k*4) + counter_l;
                        buffer[buffer_offset] = this_byte;
                        counter_l++;
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

