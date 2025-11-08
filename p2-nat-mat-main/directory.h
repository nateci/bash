/**
 * @file directory.h
 * @brief Header file for directory manipulation functions
 *
 * This file declares the directory entry structure and functions for
 * manipulating directories in the filesystem.
 */

 #ifndef DIRECTORY_H
 #define DIRECTORY_H
 
 #include "helpers/blocks.h"
 #include "inode.h"
 #include "helpers/slist.h"
 
 /**
  * Maximum length of a directory entry name
  */
 #define DIR_NAME_LENGTH 48
 
 /**
  * @struct dir_entry
  * @brief Directory entry structure
  *
  * Each entry in a directory contains a filename and associated inode number.
  * The structure is padded to be exactly 64 bytes for storage efficiency.
  */
 typedef struct dir_entry {
     char name[DIR_NAME_LENGTH];  /* Entry name */
     int inum;                    /* Inode number */
     char _reserved[12];          /* Padding to make 64 bytes total */
 } dir_entry_t;
 
 /**
  * @brief Initialize the root directory
  *
  * Sets up the root directory of the filesystem.
  * Actual implementation is handled in storage_init().
  */
 void directory_init();
 
 /**
  * @brief Look up an entry in a directory
  *
  * @param di Pointer to the directory's inode
  * @param name Name of the entry to find
  * @return The inode number of the found entry, or negative error code:
  *         -EINVAL if arguments are invalid
  *         -ENOENT if the entry is not found
  */
 int directory_lookup(inode_t *di, const char *name);
 
 /**
  * @brief Add a new entry to a directory
  *
  * @param di Pointer to the directory's inode
  * @param name Name for the new directory entry
  * @param inum Inode number to associate with the entry
  * @return 0 on success, or negative error code:
  *         -ENOTDIR if di is not a directory
  *         -ENOSPC if there's no space left in the directory block
  */
 int directory_put(inode_t *di, const char *name, int inum);
 
 /**
  * @brief Remove an entry from a directory
  *
  * @param di Pointer to the directory's inode
  * @param name Name of the entry to delete
  * @return 0 on success, or negative error code:
  *         -EINVAL if arguments are invalid
  *         -ENOTDIR if di is not a directory
  *         -ENOENT if the entry is not found
  */
 int directory_delete(inode_t *di, const char *name);
 
 /**
  * @brief Return a list of files in the directory
  *
  * @param dir Pointer to the directory's inode
  * @return A linked list of entry names (strings), or NULL if invalid
  *         The caller is responsible for freeing this list.
  */
 slist_t *directory_list(inode_t *dir);
 
 /**
  * @brief Print the contents of a directory for debugging
  *
  * @param dd Pointer to the directory's inode
  */
 void print_directory(inode_t *dd);
 
 #endif /* DIRECTORY_H */