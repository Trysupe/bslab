//
//  myfs-structs.h
//  myfs
//
//  Created by Oliver Waldhorst on 07.09.17.
//  Copyright © 2017 Oliver Waldhorst. All rights reserved.
//

#ifndef myfs_structs_h
#define myfs_structs_h

#define NAME_LENGTH 255
#define BLOCK_SIZE 512
#define NUM_DIR_ENTRIES 64
#define NUM_OPEN_FILES 64

typedef struct {
    char name[NAME_LENGTH];
    int size = 0;
    uid_t uid; // cf. https://man7.org/linux/man-pages//man3/uid_t.3.html
    gid_t gid;
    mode_t permission = S_IFREG | 0777;  // cf. mknod
    char* data;

    time_t atime;
    time_t ctime;
    time_t mtime;

} MyFSFileInfo;

typedef struct {
    char readCache[BLOCK_SIZE];
    int readCacheBlock = -1;
    char writeCache[BLOCK_SIZE];
    int writeCacheBlock = -1;
} openFile;


#endif /* myfs_structs_h */
