/**
 * Storage header file for file system implementation.
 * 
 * Provides abstract interfaces for disk storage operations including file
 * and directory management. This module serves as the main API for file system
 * operations, handling file creation, reading, writing, and directory management.
 * 
 * Based on cs3650 starter code.
 */

 #ifndef NUFS_STORAGE_H
 #define NUFS_STORAGE_H
 
 #include <sys/stat.h>
 #include <sys/types.h>
 #include <time.h>
 #include <unistd.h>
 
 #include "helpers/slist.h"
 
 /**
  * Initializes the storage system.
  * 
  * @param path The path to the storage file/device
  */
 void storage_init(const char *path);
 
 /**
  * Gets metadata about a file or directory.
  * 
  * @param path The path to the file or directory
  * @param st Pointer to a stat structure to fill with metadata
  * @return 0 on success, negative error code on failure
  */
 int storage_stat(const char *path, struct stat *st);
 
 /**
  * Reads data from a file.
  * 
  * @param path The path to the file
  * @param buf Buffer to store the read data
  * @param size Number of bytes to read
  * @param offset Starting position for reading
  * @return Number of bytes read on success, negative error code on failure
  */
 int storage_read(const char *path, char *buf, size_t size, off_t offset);
 
 /**
  * Writes data to a file.
  * 
  * @param path The path to the file
  * @param buf The data to write
  * @param size Number of bytes to write
  * @param offset Starting position for writing
  * @return Number of bytes written on success, negative error code on failure
  */
 int storage_write(const char *path, const char *buf, size_t size, off_t offset);
 
 /**
  * Changes the size of a file.
  * 
  * @param path The path to the file
  * @param size The new size for the file
  * @return 0 on success, negative error code on failure
  */
 int storage_truncate(const char *path, off_t size);
 
 /**
  * Creates a new file.
  * 
  * @param path The path to the new file
  * @param mode File permissions and type
  * @return 0 on success, negative error code on failure
  */
 int storage_mknod(const char *path, mode_t mode);
 
 /**
  * Removes a file.
  * 
  * @param path The path to the file to remove
  * @return 0 on success, negative error code on failure
  */
 int storage_unlink(const char *path);
 
 /**
  * Creates a hard link between files.
  * 
  * @param from Path to the existing file
  * @param to Path for the new link
  * @return 0 on success, negative error code on failure
  */
 int storage_link(const char *from, const char *to);
 
 /**
  * Renames a file or directory.
  * 
  * @param from Current path of the file/directory
  * @param to New path for the file/directory
  * @return 0 on success, negative error code on failure
  */
 int storage_rename(const char *from, const char *to);
 
 /**
  * Sets access and modification times for a file.
  * 
  * @param path The path to the file
  * @param ts Array of timespec structures (access time at index 0, modification time at index 1)
  * @return 0 on success, negative error code on failure
  */
 int storage_set_time(const char *path, const struct timespec ts[2]);
 
 /**
  * Creates a new directory.
  * 
  * @param path The path to the new directory
  * @param mode Directory permissions
  * @return 0 on success, negative error code on failure
  */
 int storage_mkdir(const char *path, mode_t mode);
 
 /**
  * Looks up an inode number by path.
  * 
  * @param path The path to look up
  * @return Inode number on success, negative error code on failure
  */
 int storage_lookup_path(const char *path);
 
 /**
  * Creates a new directory in a specified parent directory.
  * 
  * @param parent_inum The inode number of the parent directory
  * @param name The name of the new directory
  * @param mode Directory permissions
  * @return 0 on success, negative error code on failure
  */
 int storage_mkdir_at(int parent_inum, const char *name, mode_t mode);
 
 /**
  * Lists the contents of a directory.
  * 
  * @param path The path to the directory
  * @return A linked list of directory entries, or NULL on failure
  */
 slist_t *storage_list(const char *path);
 
 #endif