//
// Created by Oliver Waldhorst on 20.03.20.
//  Copyright © 2017-2020 Oliver Waldhorst. All rights reserved.
//

#include "myinmemoryfs.h"

// The functions fuseGettattr(), fuseRead(), and fuseReadDir() are taken from
// an example by Mohammed Q. Hussain. Here are original copyrights & licence:

/**
 * Simple & Stupid Filesystem.
 *
 * Mohammed Q. Hussain - http://www.maastaar.net
 *
 * This is an example of using FUSE to build a simple filesystem. It is a part of a tutorial in MQH Blog with the title
 * "Writing a Simple Filesystem Using FUSE in C":
 * http://www.maastaar.net/fuse/linux/filesystem/c/2016/05/21/writing-a-simple-filesystem-using-fuse/
 *
 * License: GNU GPL
 */

// For documentation of FUSE methods see https://libfuse.github.io/doxygen/structfuse__operations.html

#undef DEBUG

// NOTE: Comment lines to reduce debug messages
#define DEBUG
#define DEBUG_METHODS
#define DEBUG_RETURN_VALUES

#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <iterator>

#include "macros.h"
#include "myfs.h"
#include "myfs-info.h"
#include "blockdevice.h"

/// @brief Constructor of the in-memory file system class.
///
/// You may add your own constructor code here.
MyInMemoryFS::MyInMemoryFS() : MyFS()
{

    files = new MyFSFileInfo[NUM_DIR_ENTRIES];
    openFiles = new int32_t[NUM_OPEN_FILES];
}

/// @brief Destructor of the in-memory file system class.
///
/// You may add your own destructor code here.
MyInMemoryFS::~MyInMemoryFS()
{

    delete[] files;
    delete[] openFiles;
}

/// @brief Create a new file.
///
/// Create a new file with given name and permissions.
/// You do not have to check file permissions, but can assume that it is always ok to access the file.
/// \param [in] path Name of the file, starting with "/".
/// \param [in] mode Permissions for file access.
/// \param [in] dev Can be ignored.
/// \return 0 on success, -ERRNO on failure.
int MyInMemoryFS::fuseMknod(const char *path, mode_t mode, dev_t dev)
{
    LOGM();

    // verify the file path
    if (getIndex(path) != -1)
    {
        return -EEXIST;
    }
    if (strlen(path) > NAME_LENGTH)
    {
        return -ENAMETOOLONG;
    }

    MyFSFileInfo *file = new MyFSFileInfo();
    strcpy(file->name, path + 1); // +1 to shove the path slash `/`
    file->uid = getuid();         // get uid from current system user
    file->gid = getgid();
    time(&(file->atime));
    time(&(file->mtime));
    time(&(file->ctime));
    files[getNextFreeIndex()] = *file;

    RETURN(0);
}

/// @brief Delete a file.
///
/// Delete a file with given name from the file system.
/// You do not have to check file permissions, but can assume that it is always ok to access the file.
/// \param [in] path Name of the file, starting with "/".
/// \return 0 on success, -ERRNO on failure.
int MyInMemoryFS::fuseUnlink(const char *path)
{
    LOGM();

    int index = getIndex(path);
    if (index == -1)
    {
        return -ENOENT;
    }

    free(files[index].data);     // free the memory
    files[index].name[0] = '\0'; // reset the name nullpointer hack

    RETURN(0);
}

/// @brief Rename a file.
///
/// Rename the file with with a given name to a new name.
/// Note that if a file with the new name already exists it is replaced (i.e., removed
/// before renaming the file.
/// You do not have to check file permissions, but can assume that it is always ok to access the file.
/// \param [in] path Name of the file, starting with "/".
/// \param [in] newpath  New name of the file, starting with "/".
/// \return 0 on success, -ERRNO on failure.
int MyInMemoryFS::fuseRename(const char *path, const char *newpath)
{
    LOGM();

    // check if new name is valid
    if (strlen((newpath + 1)) > NAME_LENGTH)
    {
        return -ENAMETOOLONG;
    }
    // make sure the file does not already exist
    if (getIndex(newpath) != -1)
    {
        return -EEXIST;
    }

    // if there's a file at path, start renaming
    int index = getIndex(path);
    if (index != -1)
    {
        strcpy(files[index].name, newpath + 1);
        return 0;
    }

    return -ENOENT;
}

/// @brief Get file meta data.
///
/// Get the metadata of a file (user & group id, modification times, permissions, ...).
/// \param [in] path Name of the file, starting with "/".
/// \param [out] statbuf Structure containing the meta data, for details type "man 2 stat" in a terminal.
/// \return 0 on success, -ERRNO on failure.
int MyInMemoryFS::fuseGetattr(const char *path, struct stat *statbuf)
{
    LOGM();

    // statbuf: Structure containing the meta data. cf. original code template

    LOGF("\tAttributes of %s requested\n", path);

    statbuf->st_uid = getuid();
    statbuf->st_gid = getgid();
    statbuf->st_atime = time(NULL);
    statbuf->st_mtime = time(NULL);

    // check parent directory
    if (strcmp(path, "/") == 0)
    {
        statbuf->st_nlink = 2;
        statbuf->st_mode = S_IFDIR | 0755;
        RETURN(0);
    }

    int index = getIndex(path);
    if (index == -1)
    {
        RETURN(-ENOENT);
    }

    // write values to statbuf
    statbuf->st_mode = S_IFREG | files[index].permission;
    MyFSFileInfo file = files[index];
    statbuf->st_size = file.size;
    statbuf->st_atime = file.atime;
    statbuf->st_mtime = file.mtime;
    statbuf->st_ctime = file.ctime;
    statbuf->st_nlink = 1;

    RETURN(0);
}

/// @brief Change file permissions.
///
/// Set new permissions for a file.
/// You do not have to check file permissions, but can assume that it is always ok to access the file.
/// \param [in] path Name of the file, starting with "/".
/// \param [in] mode New mode of the file.
/// \return 0 on success, -ERRNO on failure.
int MyInMemoryFS::fuseChmod(const char *path, mode_t mode)
{
    LOGM();

    int index = getIndex(path);
    if (index == -1)
    {
        return -ENOENT;
    }

    files[index].permission = mode;

    RETURN(0);
}

/// @brief Change the owner of a file.
///
/// Change the user and group identifier in the meta data of a file.
/// You do not have to check file permissions, but can assume that it is always ok to access the file.
/// \param [in] path Name of the file, starting with "/".
/// \param [in] uid New user id.
/// \param [in] gid New group id.
/// \return 0 on success, -ERRNO on failure.
int MyInMemoryFS::fuseChown(const char *path, uid_t uid, gid_t gid)
{
    LOGM();
    int index = getIndex(path);
    if (index == -1)
    {
        return -ENOENT;
    }

    files[index].uid = uid;

    if (gid <= 100000)
    { // If GID is more than 100000 no group was given to chown so don't edit the edit the gid of the file
        files[index].gid = gid;
    }

    RETURN(0);
}

/// @brief Open a file.
///
/// Open a file for reading or writing. This includes checking the permissions of the current user and incrementing the
/// open file count.
/// You do not have to check file permissions, but can assume that it is always ok to access the file.
/// \param [in] path Name of the file, starting with "/".
/// \param [out] fileInfo Can be ignored in Part 1
/// \return 0 on success, -ERRNO on failure.
int MyInMemoryFS::fuseOpen(const char *path, struct fuse_file_info *fileInfo)
{
    LOGM();

    // if too many files are open
    if (openFileCount == NUM_OPEN_FILES)
    {
        return -EMFILE;
    }

    int pathIndex = getIndex(path);
    // if the path does not exist
    if (pathIndex == -1)
    {
        return -ENOENT;
    }

    int openFileIndex = getNextFreeIndexOpenFiles();
    openFiles[openFileIndex] = pathIndex;
    openFileCount++;
    fileInfo->fh = openFileIndex; // update filehandler

    RETURN(0);
}

/// @brief Read from a file.
///
/// Read a given number of bytes from a file starting form a given position.
/// You do not have to check file permissions, but can assume that it is always ok to access the file.
/// Note that the file content is an array of bytes, not a string. I.e., it is not (!) necessarily terminated by '\0'
/// and may contain an arbitrary number of '\0'at any position. Thus, you should not use strlen(), strcpy(), strcmp(),
/// ... on both the file content and buf, but explicitly store the length of the file and all buffers somewhere and use
/// memcpy(), memcmp(), ... to process the content.
/// \param [in] path Name of the file, starting with "/".
/// \param [out] buf The data read from the file is stored in this array. You can assume that the size of buffer is at
/// least 'size'
/// \param [in] size Number of bytes to read
/// \param [in] offset Starting position in the file, i.e., number of the first byte to read relative to the first byte of
/// the file
/// \param [in] fileInfo Can be ignored in Part 1
/// \return The Number of bytes read on success. This may be less than size if the file does not contain sufficient bytes.
/// -ERRNO on failure.
int MyInMemoryFS::fuseRead(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fileInfo)
{
    LOGM();
    LOGF("--> Trying to read %s, %lu, %lu\n", path, (unsigned long)offset, size);

    int index = openFiles[fileInfo->fh]; // cf. page 40 of `BSLab-Teil1.pdf`

    // if the offset is bigger than the file, return 0 for error
    if (offset >= files[index].size)
    {
        return 0;
    }

    if ((offset + size) > files[index].size)
    {
        size = files[index].size - offset;
    }
    memcpy(buf, files[index].data + offset, size);

    time(&(files[index].atime)); // update time

    RETURN(size);
}

/// @brief Write to a file.
///
/// Write a given number of bytes to a file starting at a given position.
/// You do not have to check file permissions, but can assume that it is always ok to access the file.
/// Note that the file content is an array of bytes, not a string. I.e., it is not (!) necessarily terminated by '\0'
/// and may contain an arbitrary number of '\0'at any position. Thus, you should not use strlen(), strcpy(), strcmp(),
/// ... on both the file content and buf, but explicitly store the length of the file and all buffers somewhere and use
/// memcpy(), memcmp(), ... to process the content.
/// \param [in] path Name of the file, starting with "/".
/// \param [in] buf An array containing the bytes that should be written.
/// \param [in] size Number of bytes to write.
/// \param [in] offset Starting position in the file, i.e., number of the first byte to read relative to the first byte of
/// the file.
/// \param [in] fileInfo Can be ignored in Part 1 .
/// \return Number of bytes written on success, -ERRNO on failure.
int MyInMemoryFS::fuseWrite(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fileInfo)
{
    LOGM();

    // verify that file from file handler exists
    if (openFiles[fileInfo->fh] == -ENOENT)
    {
        return -EBADF;
    }

    int fileIndex = openFiles[fileInfo->fh]; // cf. page 40 of `BSLab-Teil1.pdf`

    // determine new file size as the max value of old size or new content
    int newSize = 0;
    if (files[fileIndex].size < (offset + size))
    {
        newSize = offset + size;
    }
    else
    {
        newSize = files[fileIndex].size;
    }
    files[fileIndex].size = newSize;

    // change size of data to new size and write new contents
    files[fileIndex].data = (char *)(realloc(files[fileIndex].data, files[fileIndex].size + 1));
    memcpy(files[fileIndex].data + offset, buf, size);

    // update time
    time(&(files[fileIndex].mtime));
    time(&(files[fileIndex].ctime));

    RETURN(size);
}

/// @brief Close a file.
///
/// In Part 1 this includes decrementing the open file count.
/// \param [in] path Name of the file, starting with "/".
/// \param [in] fileInfo Can be ignored in Part 1 .
/// \return 0 on success, -ERRNO on failure.
int MyInMemoryFS::fuseRelease(const char *path, struct fuse_file_info *fileInfo)
{
    LOGM();

    int fileIndex = getIndex(path);
    if (fileIndex != -1)
    { // does the file exist?

        // look for file in openfiles
        for (int i = 0; i < NUM_OPEN_FILES; i++)
        {
            int curIndex = openFiles[i];

            // found file in openfiles
            if (curIndex >= 0)
            {
                if (strcmp(path + 1, files[curIndex].name) == 0)
                {
                    openFiles[i] = -ENOENT;
                    openFileCount--;
                    break; // exit the for loop
                }
            }
        }
    }
    else
    {
        return -ENOENT;
    }

    RETURN(0);
}

/// @brief Truncate a file.
///
/// Set the size of a file to the new size. If the new size is smaller than the old size, spare bytes are removed. If
/// the new size is larger than the old size, the new bytes may be random.
/// You do not have to check file permissions, but can assume that it is always ok to access the file.
/// \param [in] path Name of the file, starting with "/".
/// \param [in] newSize New size of the file.
/// \return 0 on success, -ERRNO on failure.
int MyInMemoryFS::fuseTruncate(const char *path, off_t newSize)
{
    LOGM();

    int fileIndex = getIndex(path);
    int ret = truncate(fileIndex, newSize);

    return ret;
}

/// @brief Truncate a file.
///
/// Set the size of a file to the new size. If the new size is smaller than the old size, spare bytes are removed. If
/// the new size is larger than the old size, the new bytes may be random. This function is called for files that are
/// open.
/// You do not have to check file permissions, but can assume that it is always ok to access the file.
/// \param [in] path Name of the file, starting with "/".
/// \param [in] newSize New size of the file.
/// \param [in] fileInfo Can be ignored in Part 1.
/// \return 0 on success, -ERRNO on failure.
int MyInMemoryFS::fuseTruncate(const char *path, off_t newSize, struct fuse_file_info *fileInfo)
{
    LOGM();

    int fileIndex = openFiles[fileInfo->fh];
    int ret = truncate(fileIndex, newSize);

    RETURN(ret);
}

/// @brief Read a directory.
///
/// Read the content of the (only) directory.
/// You do not have to check file permissions, but can assume that it is always ok to access the directory.
/// \param [in] path Path of the directory. Should be "/" in our case.
/// \param [out] buf A buffer for storing the directory entries.
/// \param [in] filler A function for putting entries into the buffer.
/// \param [in] offset Can be ignored.
/// \param [in] fileInfo Can be ignored.
/// \return 0 on success, -ERRNO on failure.
int MyInMemoryFS::fuseReaddir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fileInfo)
{
    LOGM();

    LOGF("--> Getting The List of Files of %s\n", path);

    filler(buf, ".", NULL, 0);  // Current Directory
    filler(buf, "..", NULL, 0); // Parent Directory

    // iterate over dir
    if (strcmp(path, "/") == 0)
    {
        for (int i = 0; i < NUM_DIR_ENTRIES; i++)
        {
            if (files[i].name[0] != '\0')
            {
                filler(buf, files[i].name, NULL, 0);
            }
        }
    }

    RETURN(0);
}

/// Initialize a file system.
///
/// This function is called when the file system is mounted. You may add some initializing code here.
/// \param [in] conn Can be ignored.
/// \return 0.
void *MyInMemoryFS::fuseInit(struct fuse_conn_info *conn)
{
    // Open logfile
    this->logFile = fopen(((MyFsInfo *)fuse_get_context()->private_data)->logFile, "w+");
    if (this->logFile == NULL)
    {
        fprintf(stderr, "ERROR: Cannot open logfile %s\n", ((MyFsInfo *)fuse_get_context()->private_data)->logFile);
    }
    else
    {
        // turn of logfile buffering
        setvbuf(this->logFile, NULL, _IOLBF, 0);

        LOG("Starting logging...\n");

        LOG("Using in-memory mode");

        // initialize the arrays to track the files during runtime
        for (int i = 0; i < NUM_DIR_ENTRIES; i++)
        {
            files[i].name[0] = '\0';
        }
        for (int i = 0; i < NUM_OPEN_FILES; i++)
        {
            openFiles[i] = -ENOENT;
        }
    }

    RETURN(0);
}

/// @brief Clean up a file system.
///
/// This function is called when the file system is unmounted. You may add some cleanup code here.
void MyInMemoryFS::fuseDestroy()
{
    LOGM();
}

// NOTE: [PART 1] You may add your own additional methods here!

// return the index of a given file
int MyInMemoryFS::getIndex(const char *path)
{
    path++; // Ignore '/'
            //    std::cout << "Trying to find: " << path << std::endl;
    for (int i = 0; i < NUM_DIR_ENTRIES; i++)
    {
        if (files[i].name[0] != '\0' && strcmp(path, files[i].name) == 0)
        {
            //            std::cout << "Found: " << files[i].name << std::endl;
            return i;
        }
    }
    return -1;
}

// return the index of the next free slot available to create an item
int MyInMemoryFS::getNextFreeIndex()
{
    for (int i = 0; i < NUM_DIR_ENTRIES; i++)
    {
        if (files[i].name[0] == '\0')
        {
            return i;
        }
    }
    return -1;
}

int MyInMemoryFS::getNextFreeIndexOpenFiles()
{
    for (int i = 0; i < NUM_OPEN_FILES; i++)
    {
        if (openFiles[i] == -ENOENT)
        {
            return i;
        }
    }
    return -1;
}

int MyInMemoryFS::truncate(int fileIndex, off_t newSize)
{
    // If the file is already of the desired size, nothing needs to be done
    if (files[fileIndex].size == newSize)
    {
        return 0;
    }

    int oldSize = files[fileIndex].size;

    files[fileIndex].size = newSize;
    files[fileIndex].data = (char *)(realloc(files[fileIndex].data, newSize));

    // If new size is bigger than old size, fill new space with dummy data
    if (newSize > oldSize)
    {
        int space = newSize - oldSize;
        int offset = newSize - space;
        for (int i = 0; i < space; i++)
        {
            files[fileIndex].data[offset + i] = '\0';
        }
    }
    time(&(files[fileIndex].mtime));
    time(&(files[fileIndex].ctime));

    return 0;
}

// DO NOT EDIT ANYTHING BELOW THIS LINE!!!

/// @brief Set the static instance of the file system.
///
/// Do not edit this method!
void MyInMemoryFS::SetInstance()
{
    MyFS::_instance = new MyInMemoryFS();
}
