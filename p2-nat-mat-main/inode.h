/**
 * Inode header file for file system implementation.
 * 
 * Defines the inode structure and related functions for managing file and
 * directory metadata within the file system. This includes creation, deletion,
 * and manipulation of inodes which store critical file metadata.
 */

 #ifndef INODE_H
 #define INODE_H
 
 #include "helpers/blocks.h"
 #include <sys/time.h>
 
 /** Total number of inodes supported by the file system */
 #define INODE_COUNT 256
 /** Size of the inode bitmap in bytes */
 #define INODE_BITMAP_SIZE (INODE_COUNT / 8)
 
 /**
  * Inode structure representing file metadata.
  * 
  * This structure stores all metadata for a file or directory,
  * including permission mode, size, block pointers, and timestamps.
  */
 typedef struct inode {
   int inum;      // Inode number - unique identifier
   int refs;      // Reference count - number of directory entries pointing to this inode
   int mode;      // Permission bits and file type flags
   int size;      // Size in bytes
   int block;     // Single block pointer (simplified implementation)
   time_t atime;  // Last access time
   time_t mtime;  // Last modification time
   time_t ctime;  // Creation time
 } inode_t;
 
 /**
  * Prints the contents of an inode for debugging.
  * 
  * @param node Pointer to the inode to print
  */
 void print_inode(inode_t *node);
 
 /**
  * Retrieves an inode by its inode number.
  * 
  * @param inum The inode number to retrieve
  * @return Pointer to the inode structure, or NULL if the inode number is invalid
  */
 inode_t *get_inode(int inum);
 
 /**
  * Allocates a new inode from the inode bitmap.
  * 
  * @return The inode number of the newly allocated inode, or -1 if no free inodes are available
  */
 int alloc_inode();
 
 /**
  * Frees an inode and any associated blocks.
  * 
  * @param inum The inode number to free
  */
 void free_inode(int inum);
 
 /**
  * Increases the size of an inode, allocating new blocks if necessary.
  * 
  * @param node Pointer to the inode to grow
  * @param size The new size in bytes
  * @return 0 on success, negative error code on failure
  */
 int grow_inode(inode_t *node, int size);
 
 /**
  * Decreases the size of an inode, potentially freeing blocks.
  * 
  * @param node Pointer to the inode to shrink
  * @param size The new size in bytes
  * @return 0 on success, negative error code on failure
  */
 int shrink_inode(inode_t *node, int size);
 
 /**
  * Maps a file block number to a filesystem block number.
  * 
  * @param node Pointer to the inode
  * @param file_bnum The file block number to look up
  * @return The filesystem block number, or -1 if invalid
  */
 int inode_get_bnum(inode_t *node, int file_bnum);
 
 #endif