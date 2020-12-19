//
// Created by user on 11.11.20.
//

#include "RootDir.h"

// RootDir constructor
RootDir::RootDir(BlockDevice *device) {
    this->device = device;
    for (int i = 0; i < NUM_DIR_ENTRIES; i++) {
        files[i] = nullptr;
    }
}

// RootDir destructor
RootDir::~RootDir() {

}

// create a file from given path
rootFile* RootDir::createFile(const char *path) {
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
    int index = 0;
    while (files[index] != file) {
        index++;
    }
    delete file;
    // set to nullptr in files array
    files[index] = nullptr;
    existingFilesCounter--;
}

// get a file at given path
rootFile* RootDir::getFile(const char *path) {
    path++;  // shove dir slash
    for (int i = 0; i < NUM_DIR_ENTRIES; i++) {
        if (files[i] != nullptr && strcmp(path, files[i]->name) == 0) {
            return files[i];
        }
    }
    return nullptr;  // return if not found
}


// get the array of all existing files
rootFile** RootDir::getFiles() {
    return files;
}

// load the file at given RootDir index
rootFile* RootDir::load(int index) {
    char buff[BLOCK_SIZE];
    memset(buff, 0, BLOCK_SIZE);
    this->device->read(ROOT_DIR_OFFSET + index, buff);

    if (!FShelper::checkBlockContent(buff)) {
        rootFile *file = new rootFile();
        std::memcpy(file, buff, sizeof(rootFile));
        this->files[index] = file;
        return file;
    } else {
        return nullptr;
    }
}

// write the changes to disk
bool RootDir::persist(rootFile *file) {
    char buff[BLOCK_SIZE];
    memset(buff, 0, BLOCK_SIZE);
    std::memcpy(buff, file, sizeof(rootFile));
    this->device->write(ROOT_DIR_OFFSET + file->rootDirBlock, buff);
    return true;
}

// load the files after opening an existing file container
void RootDir::initRootDir() {
    for (int i = 0; i < NUM_DIR_ENTRIES; i++) {
        rootFile *file = load(i);
        files[i] = file;
        if (file != nullptr) {
            existingFilesCounter++;
        }
    }
}

// initialise the RootDir content for an empty filesystem
void RootDir::initialInitRootDir() {
    char buffer[BLOCK_SIZE];
    for (int i = 0; i < NUM_DIR_ENTRIES; i++) {
        memset(buffer, 0, BLOCK_SIZE);
        device->write(ROOT_DIR_OFFSET + i, buffer);
    }
}
