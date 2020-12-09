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
    for (int i = 0; i < DATA_BLOCKS; i++) {
        if (!getBlockState(i)) {
            setBlockState(i, true);  // set the new state
            return i;
        }
    }
    return -1;
}

// return an array containing indexes of a requested amount of data blocks
int* DMap::getXAmountOfFreeBlocks(int amount) {

    int* freeBlockArray = new int[amount];

    for (int i = 0; i < amount; i++) {
        int freeBlockIndex = getNextFreeBlock();
        if (freeBlockIndex >= 0) {
            freeBlockArray[i] = freeBlockIndex;
        } else {
            return nullptr;
        }
    }

    return freeBlockArray;
}

// set the usage status of a data block to true/false
void DMap::setBlockState(int dataBlockNum, bool isUsed) {
    this->blocks[dataBlockNum] = isUsed;
    if (isUsed) {
        decreaseFreeBlockCounterBy(1);
    } else {
        increaseFreeBlockCounterBy(1);
    }
}

// return the usage status of a data block
bool DMap::getBlockState(int dataBlockNum) {
    return this->blocks[dataBlockNum];
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

    char buff[BLOCK_SIZE];

    // for blocks that need to be stored within DMap
    for (int block_index = 0; block_index < DMAP_SIZE; block_index++) {

        // store 1 bit for every written block
        for (int blockdevice_byte_index = 0; blockdevice_byte_index < BLOCK_SIZE; blockdevice_byte_index++) {

            int block_track_array_index = (block_index * BLOCK_SIZE) + blockdevice_byte_index;

            // verify that the index from the array which tracks all available blocks
            // is within bounds
            if (block_track_array_index < DATA_BLOCKS) {
                buff[blockdevice_byte_index] = blocks[block_track_array_index];
            } else {  // fill up with zeroes after reaching end of blocks array
                buff[blockdevice_byte_index] = 0;
            }

        }
        this->device->write(DMAP_OFFSET + block_index, buff);
    }
    return true;
}

// initialise the (existing) DMap and check the current available blocks
void DMap::initDMap() {

    char buff[BLOCK_SIZE];

    // iterate over every block assigned to the dmap
    for (int block_index = DMAP_OFFSET; block_index < DMAP_OFFSET + DMAP_SIZE; block_index++) {

        // grab the block data
        memset(buff, 0, BLOCK_SIZE);
        this->device->read(block_index, buff);

        if (!FShelper::checkBlockContent(buff)) {
            this->blocks[block_index] = true;
            decreaseFreeBlockCounterBy(1);
        } else {
            this->blocks[block_index] = false;
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
