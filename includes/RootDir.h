//
// Created by user on 11.11.20.
//
#include <FShelper.h>
#include "myfs-structs.h"

#ifndef MYFS_ROOTDIR_H
#define MYFS_ROOTDIR_H

class RootDir {
private:
    // Existing files
    rootFile* files[NUM_DIR_ENTRIES];
    int existingFilesCounter = 0;
    BlockDevice* device;

public:
    RootDir(BlockDevice *device);
    ~RootDir();

    rootFile* createFile(const char *path);
    void deleteFile(rootFile *file);

    rootFile* getFile(const char *path);
    rootFile** getFiles();

    rootFile* load(int index);

    bool persist(rootFile *file);

    void initRootDir();
    void initialInitRootDir();
};

#endif //MYFS_ROOTDIR_H
