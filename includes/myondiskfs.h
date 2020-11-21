//
// Created by Oliver Waldhorst on 20.03.20.
// Copyright © 2017-2020 Oliver Waldhorst. All rights reserved.
//

#ifndef MYFS_MYONDISKFS_H
#define MYFS_MYONDISKFS_H

#include "myfs.h"

#include "RootDir.h"
#include "DMap.h"
#include "FAT.h"

/// @brief On-disk implementation of a simple file system.
class MyOnDiskFS : public MyFS {
protected:
    BlockDevice *blockDevice;  // added pointer because error from CLion
    char *buffer;
    RootDir *rootDir;
    DMap *dMap;
    FAT *fat;

    int openFileCount = 0;
    openFile *openFiles[NUM_OPEN_FILES];


public:
    static MyOnDiskFS *Instance();

    // TODO: [PART 1] Add attributes of your file system here

    MyOnDiskFS();
    ~MyOnDiskFS();

    static void SetInstance();

    // --- Methods called by FUSE ---
    // For Documentation see https://libfuse.github.io/doxygen/structfuse__operations.html
    virtual int fuseGetattr(const char *path, struct stat *statbuf);
    virtual int fuseMknod(const char *path, mode_t mode, dev_t dev);
    virtual int fuseUnlink(const char *path);
    virtual int fuseRename(const char *path, const char *newpath);
    virtual int fuseChmod(const char *path, mode_t mode);
    virtual int fuseChown(const char *path, uid_t uid, gid_t gid);
    virtual int fuseTruncate(const char *path, off_t newSize);
    virtual int fuseOpen(const char *path, struct fuse_file_info *fileInfo);
    virtual int fuseRead(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fileInfo);
    virtual int fuseWrite(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fileInfo);
    virtual int fuseRelease(const char *path, struct fuse_file_info *fileInfo);
    virtual void* fuseInit(struct fuse_conn_info *conn);
    virtual int fuseReaddir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fileInfo);
    virtual int fuseTruncate(const char *path, off_t offset, struct fuse_file_info *fileInfo);
    virtual void fuseDestroy();

    // TODO: Add methods of your file system here
    int getNextFreeIndexOpenFiles();
    int readFile(int *blocks, int blockCount, int offset, size_t size, char *buf, openFile *file);
    int writeFile(int *blocks, int blockCount, int offset, size_t size, const char *buf, openFile *file);

};

#endif //MYFS_MYONDISKFS_H
