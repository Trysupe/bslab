//
// Created by user on 11.11.20.
//

#include "RootDir.h"

// RootDir constructor
RootDir::RootDir(BlockDevice *device) {

}

// RootDir destructor
RootDir::~RootDir() {

}

// create a file from given path
rootFile *RootDir::createFile(char *path) {
    return nullptr;
}

// delete a file from given rootFile
void RootDir::deleteFile(rootFile *file) {

}

// get a file at given path
rootFile *RootDir::getFile(char *path) {
    return nullptr;
}

// get the counter of all files created
int RootDir::getFilesCount() {
    return 0;
}

// get the array of all existing files
rootFile **RootDir::getFiles() {
    return nullptr;
}

// load the file at given RootDir index
rootFile *RootDir::load(int index) {
    return nullptr;
}

// write the changes to disk
bool RootDir::persist(rootFile *file) {
    return false;
}

// load the files after opening an existing file container
void RootDir::initRootDir() {

}

// initialise the RootDir content for an empty filesystem
void RootDir::initialInitRootDir() {

}
