#include <DMap.h>


// DMap Constructor
// init blockdevice for current object
DMap::DMap(BlockDevice *device) {
    this->device = device;
    for (int i = 0; i < DATA_BLOCKS; i++) {
        this->blocks[i] = false;
    }
}

// DMap Destructor
DMap::~DMap() {

}

// return index of the next free data block available
int DMap::getNextFreeBlock() {
    return 0;
}

// return an array containing indexes of a requested amount of data blocks
int DMap::getXAmountOfFreeBlocks(int amount) {
    return 0;
}

// set the usage status of a data block to true/false
void DMap::setBlockState(int dataBlockNum, bool isUsed) {

}

// return the usage status of a data block
bool DMap::getBlockState(int dataBlockNum) {
    return false;
}

// increase the counter which keeps track of the total of free blocks by
// the requested amount
void DMap::increaseFreeBlockCounterBy(int amount) {
    this->freeBlockCounter += amount;
}

// decrease the counter which keeps track of the total of free blocks by
// the requested amount
void DMap::decreaseFreeBlockCounterBy(int amount) {
    this->freeBlockCounter -= amount;
}

// return the value of the total amount of free data blocks available
int DMap::getFreeBlockCounter() {
    return this->freeBlockCounter;
}

// write the changes to the disk
bool DMap::persist() {
    return false;
}

// initialise the (existing) DMap and check the current available blocks
void DMap::initDMap() {

    char buff[BLOCK_SIZE];

    // iterate over every block assigned to the dmap
    for (int block_index = 0; block_index < DMAP_SIZE; block_index++) {

        // grab the block data
        memset(buff, 0, BLOCK_SIZE);
        this->device->read(DMAP_OFFSET + block_index, buff);

        // go over every byte from the blockdevice
        for (int char_index = 0; char_index < BLOCK_SIZE; char_index++) {
            // check every bit of given byte
            for (int bit_index = 0; bit_index < 8; bit_index++) {

                int index = DMAP_OFFSET + block_index;

                // if anything is written here, then this block is not free
                if ((buff[char_index] & (1 << (7 - bit_index))) != 0) {
                    this->blocks[index] = true;
                    // decrease by 1 and go onto the next block
                    decreaseFreeBlockCounterBy(1);
                    bit_index = 8;
                    char_index = BLOCK_SIZE;
                } else {
                    this->blocks[index] = false;
                }
            }
        }
    }
}

// initialise a DMap for an empty filesystem
void DMap::initialInitDMap() {
    char buffer[BLOCK_SIZE];
    for (int i = 0; i < DMAP_SIZE; i++) {
        memset(buffer, 0, BLOCK_SIZE);
        device->write(DMAP_OFFSET + i, buffer);
    }
}
