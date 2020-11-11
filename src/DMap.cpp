#include <DMap.h>


// DMap Constructor
DMap::DMap(BlockDevice *device) {

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

}

// return the value of the total amount of free data blocks available
int DMap::getFreeBlockCounter() {
    return 0;
}

// write the changes to the disk
bool DMap::persist() {
    return false;
}

// initialise the (existing) DMap and check the current available blocks
void DMap::initDMap() {

}

// initialise a DMap for an empty filesystem
void DMap::initialInitDMap() {

}
