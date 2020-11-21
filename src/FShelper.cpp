//
// Created by user on 21.11.20.
//

#include "FShelper.h"

// pass over a block buffer to iterate over all of the blocks bits and check if content is zero
// return true if the block is empty (full of 0s) or false once a `1` is found
bool FShelper::checkBlockContent(char buff[]) {

    // go over every byte from the blockdevice
    for (int char_index = 0; char_index < BLOCK_SIZE; char_index++) {
        // check every bit of given byte
        for (int bit_index = 0; bit_index < 8; bit_index++) {
            // if anything is written here, then this block is not free
            if ((buff[char_index] & (1 << (7 - bit_index))) == 1) {
                return false;
            }
        }
    }
    return true;
}
