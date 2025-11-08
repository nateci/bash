/**
 * @file directory.c
 * @brief Implementation of directory manipulation functions for a filesystem
 *
 * This file contains functions for managing directories within the filesystem,
 * including looking up entries, adding entries, deleting entries, and listing
 * directory contents.
 */

 #define _DEFAULT_SOURCE
 #include <string.h>
 #include <stdio.h>
 #include <stdlib.h>
 #include <errno.h>
 #include <fcntl.h>
 #include <sys/stat.h>
 #include "directory.h"
 #include "helpers/bitmap.h"
 #include "helpers/blocks.h"
 #include "inode.h"
 #include "storage.h"
 #include "helpers/slist.h"
 
 /**
  * @brief Initialize directory subsystem
  * 
  * This function is a placeholder as the actual initialization
  * is handled in storage_init().
  */
 void directory_init() {
     // Handled in storage_init()
 }
 
 /**
  * @brief Look up an entry in a directory by name
  * 
  * @param dir Pointer to the directory's inode
  * @param name Name of the entry to look up
  * @return int The inode number of the found entry, or negative error code:
  *         -EINVAL if arguments are invalid
  *         -ENOENT if the entry is not found
  * 
  * @assumption Directory block contains contiguous directory entries
  */
 int directory_lookup(inode_t *dir, const char *name) {
     if (!dir || !name) return -EINVAL;
     
     dir_entry_t *entries = blocks_get_block(dir->block);
     int count = dir->size / sizeof(dir_entry_t);
     
     // Iterate through all entries in the directory
     for (int i = 0; i < count; i++) {
         if (strcmp(entries[i].name, name) == 0) {
             return entries[i].inum;
         }
     }
     return -ENOENT;
 }
 
 /**
  * @brief Add or update an entry in a directory
  * 
  * @param dir Pointer to the directory's inode
  * @param name Name for the new directory entry
  * @param inum Inode number to associate with the entry
  * @return int 0 on success, or negative error code:
  *         -ENOTDIR if dir is not a directory
  *         -ENOSPC if there's no space left in the directory block
  * 
  * @assumption The function first tries to reuse empty slots before appending
  * @assumption Directory entries are stored in a single block (max entries = BLOCK_SIZE/sizeof(dir_entry_t))
  */
 int directory_put(inode_t *dir, const char *name, int inum) {
     if (!dir || !S_ISDIR(dir->mode)) return -ENOTDIR;
 
     dir_entry_t *entries = blocks_get_block(dir->block);
     int count = dir->size / sizeof(dir_entry_t);
 
     // Look for empty slot to reuse
     for (int i = 0; i < count; i++) {
         if (entries[i].name[0] == '\0') {
             strncpy(entries[i].name, name, DIR_NAME_LENGTH - 1);
             entries[i].name[DIR_NAME_LENGTH - 1] = '\0'; // Ensure null-termination
             entries[i].inum = inum;
             return 0;
         }
     }
 
     // No empty slot — append new entry
     if (count >= (BLOCK_SIZE / sizeof(dir_entry_t))) {
         return -ENOSPC;  // Directory block is full
     }
 
     // Add the entry at the end of the directory
     strncpy(entries[count].name, name, DIR_NAME_LENGTH - 1);
     entries[count].name[DIR_NAME_LENGTH - 1] = '\0'; // Ensure null-termination
     entries[count].inum = inum;
     dir->size += sizeof(dir_entry_t);
     dir->mtime = time(NULL);
 
     return 0;
 }
 
 /**
  * @brief Delete an entry from a directory
  * 
  * @param dir Pointer to the directory's inode
  * @param name Name of the entry to delete
  * @return int 0 on success, or negative error code:
  *         -EINVAL if arguments are invalid
  *         -ENOTDIR if dir is not a directory
  *         -ENOENT if the entry is not found
  * 
  * @assumption After deletion, remaining entries are shifted to maintain contiguity
  */
 int directory_delete(inode_t *dir, const char *name) {
     if (!dir || !name) return -EINVAL;
     if (!S_ISDIR(dir->mode)) return -ENOTDIR;
 
     dir_entry_t *entries = blocks_get_block(dir->block);
     int count = dir->size / sizeof(dir_entry_t);
 
     // Find and remove the entry
     for (int i = 0; i < count; i++) {
         if (entries[i].name[0] != '\0' && strcmp(entries[i].name, name) == 0) {
             // Clear the name
             memset(entries[i].name, 0, sizeof(entries[i].name));
 
             // Shift remaining entries to fill the gap
             if (i < count - 1) {
                 memmove(&entries[i], &entries[i+1],
                         (count - i - 1) * sizeof(dir_entry_t));
             }
 
             // Zero out the last slot which is now redundant
             memset(&entries[count - 1], 0, sizeof(dir_entry_t));
             dir->size -= sizeof(dir_entry_t);
             dir->mtime = time(NULL);
             return 0;
         }
     }
 
     return -ENOENT;
 }
 
 /**
  * @brief List all entries in a directory except for "." and ".."
  * 
  * @param dir Pointer to the directory's inode
  * @return slist_t* A linked list of entry names (strings), or NULL if invalid
  * 
  * @assumption The caller is responsible for freeing the returned list
  * @assumption Special entries "." and ".." are filtered out
  */
 slist_t *directory_list(inode_t *dir) {
     if (!dir || !S_ISDIR(dir->mode)) return NULL;
 
     slist_t *list = NULL;
     dir_entry_t *entries = blocks_get_block(dir->block);
     int count = dir->size / sizeof(dir_entry_t);
 
     // Create a list of all entry names, excluding "." and ".."
     for (int i = 0; i < count; i++) {
         if (strcmp(entries[i].name, ".") != 0 && 
             strcmp(entries[i].name, "..") != 0) {
             list = s_cons(entries[i].name, list);
         }
     }
 
     return list;
 }
 
 /**
  * @brief Print the contents of a directory to stdout for debugging
  * 
  * @param dir Pointer to the directory's inode
  * 
  * @assumption The directory is valid and accessible
  */
 void print_directory(inode_t *dir) {
     if (!dir || !S_ISDIR(dir->mode)) {
         printf("Invalid directory\n");
         return;
     }
 
     dir_entry_t *entries = blocks_get_block(dir->block);
     int count = dir->size / sizeof(dir_entry_t);
 
     printf("Directory (inode %d, size %d):\n", dir->inum, dir->size);
     for (int i = 0; i < count; i++) {
         printf("  %-12s → inode %d\n", entries[i].name, entries[i].inum);
     }
 }