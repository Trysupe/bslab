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
rootFile *RootDir::createFile(const char *path) {
    path++;  // remove path slash

    int index = 0;
    while (files[index] != nullptr) {
        index++;
    }

    // create new file with default values
    rootFile *file = new rootFile();

    strcpy(file->name, path);
    file->stat.st_mode = S_IFREG | 0644;
    file->stat.st_blksize = BLOCK_SIZE;
    file->stat.st_size = 0;
    file->stat.st_blocks = 0;
    file->stat.st_nlink = 1;
    file->stat.st_atime = time(nullptr);
    file->stat.st_mtime = time(nullptr);
    file->stat.st_ctime = time(nullptr);
    file->stat.st_uid = getuid();
    file->stat.st_gid = getgid();
    file->firstBlock = -1;

    file->rootDirBlock = index;
    files[index] = file;
    existingFilesCounter++;

    return file;
}

// delete a file from given rootFile
void RootDir::deleteFile(rootFile *file) {

}

// get a file at given path
rootFile *RootDir::getFile(const char *path) {
    path++;  // shove dir slash
    for (int i = 0; i < NUM_DIR_ENTRIES; i++) {
        if (files[i] != nullptr && strcmp(path, files[i]->name) == 0) {
            return files[i];
        }
    }
    return nullptr;  // return if not found
}

// get the counter of all files created
int RootDir::getFilesCount() {
    return 0;
}

// get the array of all existing files
rootFile **RootDir::getFiles() {
    return files;
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
