//
// Created by Oliver Waldhorst on 20.03.20.
// Copyright © 2017-2020 Oliver Waldhorst. All rights reserved.
//

#include <myondiskfs.h>

// For documentation of FUSE methods see https://libfuse.github.io/doxygen/structfuse__operations.html

#undef DEBUG

/// Comment lines to reduce debug messages
#define DEBUG
#define DEBUG_METHODS
#define DEBUG_RETURN_VALUES

#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "macros.h"
#include "myfs.h"
#include "myfs-info.h"
#include "blockdevice.h"

/// @brief Constructor of the on-disk file system class.
///
/// You may add your own constructor code here.
MyOnDiskFS::MyOnDiskFS() : MyFS()
{
    // create a block device object
    this->blockDevice = new BlockDevice(BLOCK_SIZE);

    buffer = new char[BLOCK_SIZE];      //For creating blocks
    dMap = new DMap(blockDevice);       //checks if a block if free or used
    fat = new FAT(blockDevice);         //Location of next block
    rootDir = new RootDir(blockDevice); //Metadata for files
}

/// @brief Destructor of the on-disk file system class.
///
/// You may add your own destructor code here.
MyOnDiskFS::~MyOnDiskFS()
{ //Delete everything from memory since everything is saved in the BlockDevice
    // free block device object
    delete this->blockDevice;

    delete rootDir;
    delete fat;
    delete dMap;
}

/// @brief Create a new file.
///
/// Create a new file with given name and permissions.
/// You do not have to check file permissions, but can assume that it is always ok to access the file.
/// \param [in] path Name of the file, starting with "/".
/// \param [in] mode Permissions for file access.
/// \param [in] dev Can be ignored.
/// \return 0 on success, -ERRNO on failure.
int MyOnDiskFS::fuseMknod(const char *path, mode_t mode, dev_t dev)
{
    LOGM();

    rootFile *file = rootDir->createFile(path); //New pointer to new file
    this->rootDir->persist(file);               //Persist the file -> write into the container file

    RETURN(0);
}

/// @brief Delete a file.
///
/// Delete a file with given name from the file system.
/// You do not have to check file permissions, but can assume that it is always ok to access the file.
/// \param [in] path Name of the file, starting with "/".
/// \return 0 on success, -ERRNO on failure.
int MyOnDiskFS::fuseUnlink(const char *path)
{
    LOGM();

    rootFile *file = rootDir->getFile(path); //get the strucute of the file

    // couldn't find file at that path
    if (file == nullptr)
    { //getFile returns nullptr if file does not exist
        return -ENOENT;
    }

    // go through fat and dmap and delete the chain
    if (file->firstBlock != -1)
    {
        int currentBlock = file->firstBlock;
        int nextBlock = FAT_EOF; //For while loop, since outwise it would run forever
        do
        {
            // Gets the next block
            nextBlock = fat->getNextBlock(currentBlock);

            // Sets bit to 0 to represent emtpy space
            dMap->setBlockState(currentBlock, false);

            // Deletes block
            fat->freeBlock(currentBlock);

            // Continues as long as fat is not at the end
            currentBlock = nextBlock;
        } while (nextBlock != FAT_EOF);
    }

    dMap->persist(); //Write into the blockdevice
    fat->persist();

    // clear the metadata block of the file
    char buff[BLOCK_SIZE];
    memset(buff, 0, BLOCK_SIZE);
    this->blockDevice->write(ROOT_DIR_OFFSET + file->rootDirBlock, buff); //Delte file in block device by overwriting everyting with 0's

    // clear file from rootdir
    rootDir->deleteFile(file);
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
int MyOnDiskFS::fuseRename(const char *path, const char *newpath)
{
    LOGM();

    // verify name length
    if (strlen((newpath + 1)) > NAME_LENGTH)
    {
        return -ENAMETOOLONG;
    }

    rootFile *file = rootDir->getFile(path);
    // file does not exist
    if (file == nullptr)
    {
        return -ENOENT;
    }

    // check if file already exists
    if (rootDir->getFile(newpath) != nullptr)
    {
        return -EEXIST;
    }

    strcpy(file->name, newpath + 1);
    rootDir->persist(file); // persist the new file (only the filename changed)
    RETURN(0);
}

/// @brief Get file meta data.
///
/// Get the metadata of a file (user & group id, modification times, permissions, ...).
/// \param [in] path Name of the file, starting with "/".
/// \param [out] statbuf Structure containing the meta data, for details type "man 2 stat" in a terminal.
/// \return 0 on success, -ERRNO on failure.
int MyOnDiskFS::fuseGetattr(const char *path, struct stat *statbuf)
{
    LOGM();

    // Get metadata from root folder
    if (strcmp(path, "/") == 0)
    {
        statbuf->st_mode = S_IFDIR | 0755;
        statbuf->st_nlink = 2;
        statbuf->st_uid = getuid();
        statbuf->st_gid = getgid();
        statbuf->st_atime = time(nullptr);
        statbuf->st_mtime = time(nullptr);
        return 0;
    }

    // If path requested is not the root ('/') get the file via path
    rootFile *file = rootDir->getFile(path); //get the meta data from the rootFile

    if (file == nullptr)
    {
        return -ENOENT;
    }

    // Copy new struct into buffer
    memcpy(statbuf, &file->stat, sizeof(*statbuf));

    RETURN(0);
}

/// @brief Change file permissions.
///
/// Set new permissions for a file.
/// You do not have to check file permissions, but can assume that it is always ok to access the file.
/// \param [in] path Name of the file, starting with "/".
/// \param [in] mode New mode of the file.
/// \return 0 on success, -ERRNO on failure.
int MyOnDiskFS::fuseChmod(const char *path, mode_t mode)
{
    LOGM();

    rootFile *file = rootDir->getFile(path);
    if (file == nullptr)
    {
        return -ENOENT;
    }

    file->stat.st_mode = mode;
    rootDir->persist(file);

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
int MyOnDiskFS::fuseChown(const char *path, uid_t uid, gid_t gid)
{
    LOGM();

    rootFile *file = rootDir->getFile(path);
    if (file == nullptr)
    {
        return -ENOENT;
    }

    file->stat.st_uid = uid;
    file->stat.st_gid = gid;
    rootDir->persist(file);

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
int MyOnDiskFS::fuseOpen(const char *path, struct fuse_file_info *fileInfo)
{
    LOGM();

    // Check if another open file is allowed
    if (openFileCount == NUM_OPEN_FILES)
    {
        return -EMFILE;
    }

    rootFile *file = rootDir->getFile(path);
    if (file == nullptr)
    {
        return -ENOENT;
    }

    openFileCount++;

    int openFileIndex = getNextFreeIndexOpenFiles();

    // Create a new openFile object from the file specified by the path
    openFile *openFile = new ::openFile();
    openFile->file = file;

    // Add it to the array
    openFiles[openFileIndex] = openFile;

    // Update the index in the fuse file object
    fileInfo->fh = openFileIndex;

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
int MyOnDiskFS::fuseRead(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fileInfo)
{
    LOGM();

    // check whether there is an open file
    if (openFiles[fileInfo->fh] == nullptr)
    {
        return -EBADF;
    }

    // If so write it to the root file obj
    rootFile *file = openFiles[fileInfo->fh]->file;

    // Nothing to read because the user wants to read outside of the file content
    // Example: file has 2 lines, user wants to start to read at line 5
    if (offset >= file->stat.st_size)
    {
        return 0;
    }

    // offset is in range but size (the amount of bytes to read) is too big,
    // we only want to read in the defined file and not outside
    // Example: file has 3 lines, user wants to read lines 2 to 7. User only gets to read lines 2 and 3
    if ((off_t)(offset + size) > file->stat.st_size)
    {
        size = file->stat.st_size - offset;
    }

    // number of blocks to read
    int blockCount;

    // calculate the number of blocks to read
    if (size % BLOCK_SIZE == 0)
    { // filesize is exactly multiple of BLOCK_SIZE
        blockCount = size / BLOCK_SIZE;
    }
    else
    { //+1 to read the next block too
        blockCount = size / BLOCK_SIZE + 1;
    }

    // If offset is bigger than BLOCK_SIZE we read some block in the chain,
    // determine how many blocks we need to skip
    int blockOffset = offset / BLOCK_SIZE;

    // now skip the blocks
    int currentBlock = file->firstBlock;
    for (int i = 0; i < blockOffset; i++)
    {
        currentBlock = fat->getNextBlock(currentBlock);
    }

    // construct chain of blocks that need to be read
    int *blocks = new int[blockCount];
    blocks[0] = currentBlock;
    for (int i = 1; i < blockCount; i++)
    {
        currentBlock = fat->getNextBlock(currentBlock); //get the next block from the FAR
        blocks[i] = currentBlock;
    }

    // read the file
    int err = readFile(blocks, blockCount, offset % BLOCK_SIZE, size, buf, openFiles[fileInfo->fh]);

    // if an error occurred return error
    if (err != 0)
    {
        return -EIO;
    }

    // update the file because of the new a_time
    file->stat.st_atime = time(nullptr);

    this->rootDir->persist(file); //Update the a_time in the rootDir

    // We dont want to store the temp blocks
    delete[] blocks;

    // return the amount of bytes to read
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
int MyOnDiskFS::fuseWrite(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fileInfo)
{
    LOGM();

    // verify that file is opened
    if (openFiles[fileInfo->fh] == nullptr)
    {
        return -EBADF;
    }
    rootFile *file = openFiles[fileInfo->fh]->file;

    // first, collect information about how many & which blocks
    // to use to store the data to be written

    // Number of blocks to write
    int blockCount;
    if (size % BLOCK_SIZE == 0)
    {
        blockCount = size / BLOCK_SIZE;
    }
    else
    {
        blockCount = size / BLOCK_SIZE + 1;
    }

    // offset block to start writing on existing files
    int blockOffset = offset / BLOCK_SIZE;
    // if the existing file gets bigger, append to existing block
    int blockCountDelta = blockOffset + blockCount - file->stat.st_blocks;

    if (blockCountDelta > 0)
    { // if we need a new block

        // create array of free blocks, according to size of the data we're writing
        // also mark the blocks as used already
        int *newBlocks = dMap->getXAmountOfFreeBlocks(blockCount);

        if (file->firstBlock == -1)
        { // file is new or empty
            file->firstBlock = newBlocks[0];
        }
        else
        { // we are editing an existing file
            // connect the last block of the file to new blocks
            int currentBlock = file->firstBlock;
            // get end of chain
            while (fat->getNextBlock(currentBlock) != FAT_EOF)
            {
                currentBlock = fat->getNextBlock(currentBlock);
            }
            fat->setNextBlock(currentBlock, newBlocks[0]);
        }

        // store the chain of blocks in fat
        for (int i = 1; i < blockCount; i++)
        {
            fat->setNextBlock(newBlocks[i - 1], newBlocks[i]);
        }

        delete[] newBlocks;
        file->stat.st_blocks = blockCount + blockOffset;
    }

    // write new file size
    if ((off_t)(offset + size) > file->stat.st_size)
    {
        file->stat.st_size = offset + size;
    }

    int currentBlock = file->firstBlock;
    // Move to offset - in case of writing to existing file
    for (int i = 1; i <= blockOffset; i++)
    {
        int nextBlockIndex = fat->getNextBlock(currentBlock);
        if (nextBlockIndex != FAT_EOF) {
            currentBlock = nextBlockIndex;
        }
    }

    // Collect blocks to write
    int *blocks = new int[blockCount];
    blocks[0] = currentBlock;
    for (int i = 1; i < blockCountDelta; i++)
    {
        currentBlock = fat->getNextBlock(currentBlock);
        blocks[i] = currentBlock;
    }

    // pass to function to write blocks
    int err = writeFile(blocks, blockCount, offset % BLOCK_SIZE, size, buf, openFiles[fileInfo->fh]);

    // verify
    if (err != 0)
    {
        return -EIO;
    }
    // and set the new timestamps
    file->stat.st_mtime = time(nullptr);
    file->stat.st_ctime = time(nullptr);

    // persist the changes
    dMap->persist();
    fat->persist();
    rootDir->persist(file);

    // return the actual written number of bytes
    RETURN(size);
}

/// @brief Close a file.
///
/// \param [in] path Name of the file, starting with "/".
/// \param [in] File handel for the file set by fuseOpen.
/// \return 0 on success, -ERRNO on failure.
int MyOnDiskFS::fuseRelease(const char *path, struct fuse_file_info *fileInfo)
{
    LOGM();

    int openFileIndex = fileInfo->fh;
    delete openFiles[openFileIndex];
    openFiles[openFileIndex] = nullptr;
    openFileCount--;

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
int MyOnDiskFS::fuseTruncate(const char *path, off_t newSize)
{
    LOGM();

    // why is newSize 0?  -> truncate calls with newsize 0 to 'remove' a file

    // truncate 'only' works because fuseWrite is called by fuse afterwards.
    // fuseTruncate basically just takes care of the fat/dmap handling after removing/adding more data
    struct fuse_file_info fileInfo = {};

    // open the file: return errno - or - 0 if successful
    int ret = fuseOpen(path, &fileInfo);

    // prematurely return errno if found
    if (ret != 0)
    {
        return ret;
    }

    // resize the file
    ret = fuseTruncate(path, newSize, &fileInfo);

    // return error messages
    if (ret != 0)
    {
        return ret;
    }

    // Close the opened file
    ret = fuseRelease(path, &fileInfo);
    // and return the final error code - or - 0 on success
    RETURN(ret);
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
int MyOnDiskFS::fuseTruncate(const char *path, off_t newSize, struct fuse_file_info *fileInfo)
{
    LOGM();

    // verify that the file correct file has been opened
    if (openFiles[fileInfo->fh] == nullptr)
    {
        return -EBADF;
    }
    // grab file information in our rootFile structure
    rootFile *file = openFiles[fileInfo->fh]->file;

    // file size is unchanged. do nothing and return 0
    if (file->stat.st_size == newSize)
    {
        return 0;
    }


    // FIXME: free used blocks from blockdevice when shoving datablocks
    uint32_t blockCount;
    // count the required amount of blocks to
    if (newSize % BLOCK_SIZE == 0)
    {
        blockCount = newSize / BLOCK_SIZE;
    }
    else
    {
        blockCount = newSize / BLOCK_SIZE + 1;
    }

    // count the difference in blocks (to be removed later on)
    uint32_t blockDelta = file->stat.st_blocks - blockCount;

    // should always be >= 1 unless we are wiping the file, (in which case we should not end up here in the first place)
    if (blockDelta > 0)
    {
        uint32_t offsetBlock = file->stat.st_blocks - blockDelta;
        int32_t currentBlock = file->firstBlock;
        // move to offset block
        for (uint32_t i = 1; i < offsetBlock; i++)
        {
            currentBlock = fat->getNextBlock(currentBlock);
        }

        int32_t nextBlock = fat->getNextBlock(currentBlock);

        // go through dmap and fat chain and set them all to unused/free
        while (nextBlock != FAT_EOF)
        {
            fat->freeBlock(currentBlock);
            dMap->setBlockState(nextBlock, false);
            currentBlock = nextBlock;
            nextBlock = fat->getNextBlock(currentBlock);
        }

        if (blockCount == 0)
        {
            // If the file was truncated to zero the starting block wasn't freed yet
            dMap->setBlockState(file->firstBlock, false);
            file->firstBlock = -1;
        }
        file->stat.st_blocks = blockCount;
    }

    // determine metadata
    file->stat.st_size = newSize;
    file->stat.st_mtime = time(nullptr);
    file->stat.st_ctime = time(nullptr);

    // persist changes
    this->dMap->persist();
    fat->persist();
    this->rootDir->persist(file);

    RETURN(0);
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
int MyOnDiskFS::fuseReaddir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fileInfo)
{
    LOGM();

    // Dir self reference
    filler(buf, ".", NULL, 0);
    // Reference to parent directory
    filler(buf, "..", NULL, 0);

    // Since we only have one root directory we do not have to check for a directory
    rootFile **files = rootDir->getFiles();
    for (int i = 0; i < NUM_DIR_ENTRIES; i++)
    {
        if (files[i] != nullptr)
        {
            struct stat s = {};
            fuseGetattr(files[i]->name, &s);
            filler(buf, files[i]->name, &s, 0);
        }
    }
    RETURN(0);
}

/// Initialize a file system.
///
/// This function is called when the file system is mounted. You may add some initializing code here.
/// \param [in] conn Can be ignored.
/// \return 0.
void *MyOnDiskFS::fuseInit(struct fuse_conn_info *conn)
{
    // Open logfile
    this->logFile = fopen(((MyFsInfo *)fuse_get_context()->private_data)->logFile, "w+");
    if (this->logFile == NULL)
    {
        fprintf(stderr, "ERROR: Cannot open logfile %s\n", ((MyFsInfo *)fuse_get_context()->private_data)->logFile);
    }
    else
    {
        // turn off logfile buffering
        setvbuf(this->logFile, NULL, _IOLBF, 0);

        LOG("Starting logging...\n");

        LOG("Using on-disk mode");

        LOGF("Container file name: %s", ((MyFsInfo *)fuse_get_context()->private_data)->contFile);

        int ret = this->blockDevice->open(((MyFsInfo *)fuse_get_context()->private_data)->contFile);

        // Init the open files array
        for (int i = 0; i < NUM_OPEN_FILES; i++)
        {
            openFiles[i] = nullptr;
        }

        if (ret >= 0)
        {
            LOG("Container file does exist, reading");

            dMap->initDMap();
            fat->initFAT();
            rootDir->initRootDir();
        }
        else if (ret == -ENOENT)
        {
            LOG("Container file does not exist, creating a new one");

            ret = this->blockDevice->create(((MyFsInfo *)fuse_get_context()->private_data)->contFile);

            if (ret >= 0)
            {

                dMap->initialInitDMap();
                fat->initialInitFAT();
                rootDir->initialInitRootDir();
            }
        }

        if (ret < 0)
        {
            LOGF("ERROR: Access to container file failed with error %d", ret);
        }
    }

    return 0;
}

/// @brief Clean up a file system.
///
/// This function is called when the file system is unmounted. You may add some cleanup code here.
void MyOnDiskFS::fuseDestroy()
{
    LOGM();
}

// You may add your own additional methods here!

/// @param [*blocks] is an array of blocks with data
/// @param [blockCount] how many blocks are in *blocks
/// @param [size] how many bytes do we need to write
/// @brief Helper method to write a file on disk
/// @return 0 on success, -1 on failure
int MyOnDiskFS::writeFile(int *blocks, int blockCount, int offset, size_t size, const char *buf, openFile *file)
{

    // write all blocks given by the counter
    for (int i = 0; i < blockCount; i++)
    {
        memset(buffer, 0, BLOCK_SIZE);

        // grab the already present data inside the block into the buffer
        if (i == 0 && file->writeCacheBlock == ((int32_t)blocks[i]))
        {
            memcpy(buffer, file->writeCache, BLOCK_SIZE);
        }
        else
        {
            blockDevice->read(DATA_OFFSET + blocks[i], buffer);
        }

        size_t bufOffset = i * BLOCK_SIZE;
        size_t tempSize;

        // First block
        if (i == 0)
        {
            // if there's enough space in this block for the data
            if ((size_t)(offset + size) < (size_t)(BLOCK_SIZE))
            {
                tempSize = size;
            }
            else
            {
                tempSize = BLOCK_SIZE - offset;
            }
            // remaining space in this block
            memcpy(buffer + offset, buf, tempSize);

            // Other blocks
        }
        else
        {
            // if remaining data is smaller than block_size
            if (size < BLOCK_SIZE)
            {
                tempSize = size;
            }
            else
            {
                tempSize = BLOCK_SIZE;
            }
            memcpy(buffer, buf + bufOffset, tempSize);
        }

        // Store new size
        size -= tempSize;

        // last block
        if (i == blockCount - 1)
        {
            // Caching last written block
            file->writeCacheBlock = blocks[i];
            memcpy(file->writeCache, buffer, BLOCK_SIZE);
        }

        blockDevice->write(blocks[i] + DATA_OFFSET, buffer);
    }

    return 0;
}

/// @brief Helper method to read a file from disk
/// @return 0 on success, -1 on failure
int MyOnDiskFS::readFile(int *blocks, int blockCount, int offset, size_t size, char *buf, openFile *openFile)
{

    // Read all blocks given by the block counter
    for (int i = 0; i < blockCount; i++)
    {

        // make sure the buffer is not filled with old stuff
        memset(buffer, 0, BLOCK_SIZE);

        // is the data that needs to be read available in cache?
        if (i == 0 && openFile->readCacheBlock == ((int)blocks[0]))
        {
            memcpy(buffer, openFile->readCache, BLOCK_SIZE);
        }
        else
        {
            blockDevice->read(DATA_OFFSET + blocks[i], buffer);
        }

        size_t bufferOffset = i * BLOCK_SIZE;
        size_t temporarySize;
        // first block
        if (i == 0)
        {
            // does blocks[0] fit in current block?
            if ((size_t)(offset + size) < (size_t)(BLOCK_SIZE - offset))
            {
                temporarySize = size;
            }
            else
            {
                temporarySize = BLOCK_SIZE - offset;
            }

            memcpy(buf, buffer + offset, size);

            // remaining blocks
        }
        else
        {
            // if remaining data fits in one block
            if (size < BLOCK_SIZE)
            {
                temporarySize = size;
            }
            else
            {
                temporarySize = BLOCK_SIZE;
            }
            memcpy(buf + bufferOffset, buffer, temporarySize);
        }
        // update the remaining bytes to read
        size -= temporarySize;

        // last block
        if (i == blockCount - 1)
        {
            // and store it in cache
            openFile->readCacheBlock = blocks[i];
            memcpy(openFile->readCache, buffer, BLOCK_SIZE);
        }
    }

    // if the file could not be read correctly and the size is not set to 0 return error
    if (size != 0)
    {
        return -1;
    }

    return 0;
}

/// @brief return the next index to track opened files
int MyOnDiskFS::getNextFreeIndexOpenFiles()
{
    for (int i = 0; i < NUM_OPEN_FILES; i++)
    {
        if (openFiles[i] == nullptr)
        {
            return i;
        }
    }
    return -1;
}

// DO NOT EDIT ANYTHING BELOW THIS LINE!!!

/// @brief Set the static instance of the file system.
///
/// Do not edit this method!
void MyOnDiskFS::SetInstance()
{
    MyFS::_instance = new MyOnDiskFS();
}
