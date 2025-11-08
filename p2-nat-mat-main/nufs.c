/**
 * NUFS (Null File System) implementation using FUSE
 * 
 * This file contains the main FUSE operations for our simple filesystem.
 * Each function corresponds to a standard filesystem operation.
 */

 #include <features.h>
 #include <sys/stat.h>
 #include <fcntl.h>
 #include <dirent.h>
 #include <unistd.h>
 #include <stdlib.h>
 
 #define FUSE_USE_VERSION 26
 #include <fuse.h>
 
 #include <assert.h>
 #include <errno.h>
 #include <stdio.h>
 #include <string.h>
 #include <sys/types.h>
 
 #include "storage.h"
 #include "inode.h"
 #include "directory.h"
 
 /* ====================== FUSE OPERATION IMPLEMENTATIONS ===================== */
 
 /**
  * Check if a file exists and is accessible with given permissions
  * 
  * @param path The file path to check
  * @param mask The access mode (R_OK, W_OK, X_OK)
  * @return 0 if accessible, -errno on error
  * 
  * Assumes: Only root directory and "/hello.txt" exist in this simple implementation
  */
 int nufs_access(const char *path, int mask) {
     int rv = 0;
 
     if (strcmp(path, "/") == 0 || strcmp(path, "/hello.txt") == 0) {
         rv = 0;  // File exists and is accessible
     } else {
         rv = -ENOENT;  // File doesn't exist
     }
 
     printf("access(%s, %04o) -> %d\n", path, mask, rv);
     return rv;
 }
 
 /**
  * Get file attributes (metadata)
  * 
  * @param path The file path to examine
  * @param st Pointer to stat struct to fill with attributes
  * @return 0 on success, -errno on error
  * 
  * Fills in: mode, size, nlink, ino, uid
  * Special handling for directories to count subdirectory links
  */
 int nufs_getattr(const char *path, struct stat *st) {
     memset(st, 0, sizeof(struct stat));
     st->st_uid = getuid();
     
     int inum = storage_lookup_path(path);
     if (inum < 0) return -ENOENT;
     
     inode_t *node = get_inode(inum);
     if (!node) return -ENOENT;
     
     st->st_mode = node->mode;
     st->st_size = node->size;
     st->st_nlink = 1;
     st->st_ino = inum;
     
     // Special handling for directories
     if (S_ISDIR(node->mode)) {
         st->st_nlink = 2;  // Default for directories (. and ..)
         
         // Count subdirectories (each adds 1 to nlink)
         dir_entry_t *entries = blocks_get_block(node->block);
         int count = node->size / sizeof(dir_entry_t);
         
         for (int i = 0; i < count; i++) {
             if (strcmp(entries[i].name, ".") != 0 && 
                 strcmp(entries[i].name, "..") != 0) {
                 inode_t *child = get_inode(entries[i].inum);
                 if (child && S_ISDIR(child->mode)) {
                     st->st_nlink++;
                 }
             }
         }
     }
     
     return 0;
 }
 
 /**
  * Read directory contents
  * 
  * @param path Directory path to read
  * @param buf Buffer to fill with directory entries
  * @param filler FUSE function to add entries to buffer
  * @param offset Current read offset (unused)
  * @param fi File information (unused)
  * @return 0 on success, -errno on error
  * 
  * Lists all entries in the directory using the FUSE filler function
  */
 int nufs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                  off_t offset, struct fuse_file_info *fi) {
     printf("readdir(%s)\n", path);
 
     // Lookup directory inode
     int inum = storage_lookup_path(path);
     if (inum < 0) return -ENOENT;
 
     inode_t *dir = get_inode(inum);
     if (!dir || !S_ISDIR(dir->mode)) return -ENOTDIR;
 
     // Add standard directory entries
     filler(buf, ".", NULL, 0);
     filler(buf, "..", NULL, 0);
 
     // Get directory contents
     dir_entry_t *entries = blocks_get_block(dir->block);
     int count = dir->size / sizeof(dir_entry_t);
 
     // Add valid entries to FUSE listing
     for (int i = 0; i < count; i++) {
         if (entries[i].name[0] != '\0' &&
             strcmp(entries[i].name, ".") != 0 &&
             strcmp(entries[i].name, "..") != 0) {
             struct stat st;
             char full_path[PATH_MAX];
             snprintf(full_path, sizeof(full_path), "%s/%s", path, entries[i].name);
             
             if (nufs_getattr(full_path, &st) == 0) {
                 filler(buf, entries[i].name, &st, 0);
             }
         }
     }
 
     return 0;
 }
 
 /**
  * Create a filesystem node (file, device, etc.)
  * 
  * @param path Path for new node
  * @param mode File type and permissions
  * @param rdev Device number (unused)
  * @return 0 on success, -errno on error
  */
 int nufs_mknod(const char *path, mode_t mode, dev_t rdev) {
     printf("mknod(%s, %04o)\n", path, mode);
     return storage_mknod(path, mode);
 }
 
 /**
  * Create a new directory
  * 
  * @param path Path for new directory
  * @param mode Directory permissions
  * @return 0 on success, -errno on error
  * 
  * Handles path splitting and parent directory lookup
  */
 int nufs_mkdir(const char *path, mode_t mode) {
     printf("mkdir(%s)\n", path);
     
     // Handle root directory specially
     if (strcmp(path, "/") == 0) return -EEXIST;
     
     // Split path into parent and new directory name
     char *path_copy = strdup(path);
     char *dirname = strrchr(path_copy, '/');
     if (!dirname || strlen(dirname) <= 1) {
         free(path_copy);
         return -EINVAL;
     }
     
     *dirname = '\0';
     dirname++;  // Now points to the new directory name
     
     const char *parent_path = (path_copy[0] == '\0') ? "/" : path_copy;
     int parent_inum = storage_lookup_path(parent_path);
     if (parent_inum < 0) {
         free(path_copy);
         return parent_inum;
     }
     
     // Create the directory
     int rv = storage_mkdir_at(parent_inum, dirname, mode);
     free(path_copy);
     return rv;
 }
 
 /**
  * Write data to a file
  * 
  * @param path File path to write to
  * @param buf Data buffer to write
  * @param size Number of bytes to write
  * @param offset File offset to write at
  * @param fi File information (unused)
  * @return Number of bytes written, or -errno on error
  */
 int nufs_write(const char *path, const char *buf, size_t size, off_t offset,
                struct fuse_file_info *fi) {
     printf("write(%s, %ld bytes @%ld)\n", path, size, offset);
     return storage_write(path, buf, size, offset);
 }
 
 /**
  * Remove a file (unlink)
  * 
  * @param path File path to remove
  * @return 0 on success, -errno on error
  * 
  * Handles directory entry removal and inode cleanup
  */
 int nufs_unlink(const char *path) {
     printf("unlink(%s)\n", path);
     
     // Split path into parent and filename
     char *path_copy = strdup(path);
     if (!path_copy) return -ENOMEM;
     
     char *filename = strrchr(path_copy, '/');
     if (!filename) {
         free(path_copy);
         return -EINVAL;
     }
     *filename++ = '\0';
     
     // Lookup parent directory
     const char *parent_path = path_copy[0] ? path_copy : "/";
     int parent_inum = storage_lookup_path(parent_path);
     if (parent_inum < 0) {
         free(path_copy);
         return parent_inum;
     }
     
     inode_t *parent = get_inode(parent_inum);
     if (!parent || !S_ISDIR(parent->mode)) {
         free(path_copy);
         return -ENOTDIR;
     }
     
     // Find the file in the directory
     int file_inum = directory_lookup(parent, filename);
     if (file_inum < 0) {
         free(path_copy);
         return file_inum;
     }
     
     inode_t *node = get_inode(file_inum);
     if (!node) {
         free(path_copy);
         return -ENOENT;
     }
     
     if (S_ISDIR(node->mode)) {
         free(path_copy);
         return -EISDIR;  // Can't unlink directories with this function
     }
 
     // Remove directory entry
     dir_entry_t *entries = blocks_get_block(parent->block);
     int count = parent->size / sizeof(dir_entry_t);
     int found = 0;
     
     for (int i = 0; i < count; i++) {
         if (strcmp(entries[i].name, filename) == 0) {
             // Clear and shift entries
             memset(entries[i].name, 0, sizeof(entries[i].name));
             
             if (i < count - 1) {
                 memmove(&entries[i], &entries[i+1], 
                         (count - i - 1) * sizeof(dir_entry_t));
             }
             
             memset(&entries[count - 1], 0, sizeof(dir_entry_t));
             parent->size -= sizeof(dir_entry_t);
             found = 1;
             break;
         }
     }
     
     if (!found) {
         free(path_copy);
         return -ENOENT;
     }
 
     // Free file resources
     if (node->block >= 0) {
         free_block(node->block);
     }
     free_inode(file_inum);
     
     // Update parent directory timestamps
     parent->mtime = parent->ctime = time(NULL);
     
     free(path_copy);
     return 0;
 }
 
 /**
  * Create a hard link (not implemented)
  * 
  * @param from Source path
  * @param to Destination path
  * @return -1 (not implemented)
  */
 int nufs_link(const char *from, const char *to) {
     int rv = -1;
     printf("link(%s => %s) -> %d\n", from, to, rv);
     return rv;
 }
 
 /**
  * Remove a directory
  * 
  * @param path Directory path to remove
  * @return 0 on success, -errno on error
  * 
  * Verifies directory is empty before removal
  */
 int nufs_rmdir(const char *path) {
     printf("\nRMDIR START: %s\n", path);
 
     // 1. Lookup the target directory
     int dir_inum = storage_lookup_path(path);
     if (dir_inum < 0) return dir_inum;
 
     inode_t *dir = get_inode(dir_inum);
     if (!dir || !S_ISDIR(dir->mode)) return -ENOTDIR;
 
     // 2. Ensure the directory is empty
     dir_entry_t *entries = blocks_get_block(dir->block);
     int count = dir->size / sizeof(dir_entry_t);
     for (int i = 0; i < count; i++) {
         if (entries[i].name[0] != '\0' &&
             strcmp(entries[i].name, ".") != 0 &&
             strcmp(entries[i].name, "..") != 0) {
             return -ENOTEMPTY;
         }
     }
 
     // 3. Get parent path and directory name
     char *path_copy = strdup(path);
     char *dirname = strrchr(path_copy, '/');
     if (!dirname) {
         free(path_copy);
         return -EINVAL;
     }
     *dirname = '\0';
     dirname++;
 
     char name_buf[DIR_NAME_LENGTH];
     strncpy(name_buf, dirname, DIR_NAME_LENGTH);
     name_buf[DIR_NAME_LENGTH - 1] = '\0';
 
     const char *parent_path = (path_copy[0] == '\0') ? "/" : path_copy;
     int parent_inum = storage_lookup_path(parent_path);
     free(path_copy);
     if (parent_inum < 0) return parent_inum;
 
     inode_t *parent = get_inode(parent_inum);
     if (!parent || !S_ISDIR(parent->mode)) return -ENOTDIR;
 
     // 4. Remove the entry from parent directory
     dir_entry_t *parent_entries = blocks_get_block(parent->block);
     int parent_count = parent->size / sizeof(dir_entry_t);
     int found = 0;
     
     for (int i = 0; i < parent_count; i++) {
         printf("Checking parent entry: '%s'\n", parent_entries[i].name);
         if (strcmp(parent_entries[i].name, name_buf) == 0) {
             memmove(&parent_entries[i], &parent_entries[i + 1],
                     (parent_count - i - 1) * sizeof(dir_entry_t));
             memset(&parent_entries[parent_count - 1], 0, sizeof(dir_entry_t));
             parent->size -= sizeof(dir_entry_t);
             parent->mtime = time(NULL);
             found = 1;
             break;
         }
     }
 
     if (!found) {
         printf("ERROR: Entry '%s' not found in parent\n", name_buf);
         return -ENOENT;
     }
 
     // 5. Free directory resources
     free_block(dir->block);
     free_inode(dir_inum);
 
     printf("RMDIR SUCCESS: %s\n", path);
     blocks_flush();
     return 0;
 }
 
 /**
  * Rename/move a file or directory
  * 
  * @param from Source path
  * @param to Destination path
  * @return 0 on success, -errno on error
  * 
  * Implements atomic rename by first adding new entry then removing old
  */
 int nufs_rename(const char *from, const char *to) {
     // Verify both paths are absolute
     if (from[0] != '/' || to[0] != '/') return -EINVAL;
 
     // Lookup source
     int from_inum = storage_lookup_path(from);
     if (from_inum < 0) return from_inum;
 
     // Split paths into parent and name components
     char *from_copy = strdup(from);
     char *from_name = strrchr(from_copy, '/');
     if (!from_name) {
         free(from_copy);
         return -EINVAL;
     }
     *from_name++ = '\0';
 
     char *to_copy = strdup(to);
     char *to_name = strrchr(to_copy, '/');
     if (!to_name) {
         free(from_copy);
         free(to_copy);
         return -EINVAL;
     }
     *to_name++ = '\0';
 
     // Get parent directories
     const char *from_parent_path = (from_copy[0] == '\0') ? "/" : from_copy;
     const char *to_parent_path = (to_copy[0] == '\0') ? "/" : to_copy;
 
     int from_parent = storage_lookup_path(from_parent_path);
     int to_parent = storage_lookup_path(to_parent_path);
     
     if (from_parent < 0 || to_parent < 0) {
         free(from_copy);
         free(to_copy);
         return -ENOENT;
     }
 
     // Atomic rename: first add to new location
     int rv = directory_put(get_inode(to_parent), to_name, from_inum);
     if (rv < 0) {
         free(from_copy);
         free(to_copy);
         return rv;
     }
 
     // Then remove from old location
     rv = directory_delete(get_inode(from_parent), from_name);
     if (rv < 0) {
         // Rollback if delete fails
         directory_delete(get_inode(to_parent), to_name);
         free(from_copy);
         free(to_copy);
         return rv;
     }
 
     free(from_copy);
     free(to_copy);
     return 0;
 }
 
 /**
  * Change file permissions (not implemented)
  * 
  * @param path File path
  * @param mode New permissions
  * @return -1 (not implemented)
  */
 int nufs_chmod(const char *path, mode_t mode) {
     int rv = -1;
     printf("chmod(%s, %04o) -> %d\n", path, mode, rv);
     return rv;
 }
 
 /**
  * Truncate or extend a file
  * 
  * @param path File path
  * @param size New size
  * @return 0 on success, -errno on error
  */
 int nufs_truncate(const char *path, off_t size) {
     int inum = storage_lookup_path(path);
     if (inum < 0) return -ENOENT;
     
     inode_t *node = get_inode(inum);
     if (!node) return -ENOENT;
     
     // For now, just update the size
     node->size = size;
     return 0;
 }
 
 /**
  * Open a file
  * 
  * @param path File path
  * @param fi File information
  * @return 0 on success, -errno on error
  * 
  * Checks file exists and permissions are valid
  */
 int nufs_open(const char *path, struct fuse_file_info *fi) {
     int inum = storage_lookup_path(path);
     if (inum < 0) return -ENOENT;
     
     inode_t *node = get_inode(inum);
     if (!node) return -ENOENT;
     
     // Check access modes
     if ((fi->flags & O_ACCMODE) != O_RDONLY) {
         if ((node->mode & 0222) == 0) return -EACCES;
     }
     
     return 0;
 }
 
 /**
  * Read data from a file
  * 
  * @param path File path
  * @param buf Buffer to read into
  * @param size Number of bytes to read
  * @param offset Offset to read from
  * @param fi File information (unused)
  * @return Number of bytes read, or -errno on error
  */
 int nufs_read(const char *path, char *buf, size_t size, off_t offset,
               struct fuse_file_info *fi) {
     int inum = storage_lookup_path(path);
     if (inum < 0) return -ENOENT;
     
     inode_t *node = get_inode(inum);
     if (!node || !S_ISREG(node->mode)) return -EISDIR;
     
     // Check bounds
     if (offset >= node->size) return 0;
     if (offset + size > node->size) size = node->size - offset;
     
     // Read data
     void *block = blocks_get_block(node->block);
     memcpy(buf, block + offset, size);
     
     // Update access time
     node->atime = time(NULL);
     
     return size;
 }
 
 /**
  * Update file timestamps
  * 
  * @param path File path
  * @param ts Timespec array (atime, mtime)
  * @return 0 on success, -errno on error
  * 
  * Currently sets both times to current time
  */
 int nufs_utimens(const char *path, const struct timespec ts[2]) {
     int inum = storage_lookup_path(path);
     if (inum < 0) return -ENOENT;
     
     inode_t *node = get_inode(inum);
     if (!node) return -ENOENT;
     
     // Update timestamps (simplified to current time)
     time_t now = time(NULL);
     node->atime = now;
     node->mtime = now;
     
     return 0;
 }
 
 /**
  * IOCTL operation (not implemented)
  * 
  * @param path File path
  * @param cmd IOCTL command
  * @param arg Command argument
  * @param fi File information
  * @param flags Additional flags
  * @param data Additional data
  * @return -1 (not implemented)
  */
 int nufs_ioctl(const char *path, int cmd, void *arg, struct fuse_file_info *fi,
                unsigned int flags, void *data) {
     int rv = -1;
     printf("ioctl(%s, %d, ...) -> %d\n", path, cmd, rv);
     return rv;
 }
 
 /* ====================== FUSE INITIALIZATION ===================== */
 
 /**
  * Initialize FUSE operations structure
  * 
  * @param ops Pointer to fuse_operations struct to populate
  */
 void nufs_init_ops(struct fuse_operations *ops) {
     memset(ops, 0, sizeof(struct fuse_operations));
     ops->access = nufs_access;
     ops->getattr = nufs_getattr;
     ops->readdir = nufs_readdir;
     ops->mknod = nufs_mknod;
     ops->mkdir = nufs_mkdir;
     ops->link = nufs_link;
     ops->unlink = nufs_unlink;
     ops->rmdir = nufs_rmdir;
     ops->rename = nufs_rename;
     ops->chmod = nufs_chmod;
     ops->truncate = nufs_truncate;
     ops->open = nufs_open;
     ops->read = nufs_read;
     ops->write = nufs_write;
     ops->utimens = nufs_utimens;
     ops->ioctl = nufs_ioctl;
 }
 
 struct fuse_operations nufs_ops;
 
 /**
  * Main entry point for NUFS
  * 
  * @param argc Argument count
  * @param argv Argument vector
  * @return FUSE operation result
  * 
  * Initializes storage and starts FUSE main loop
  */
 int main(int argc, char *argv[]) {
     assert(argc > 2 && argc < 6);
     storage_init(argv[--argc]);  // Initialize with disk image path
     nufs_init_ops(&nufs_ops);
     return fuse_main(argc, argv, &nufs_ops, NULL);
 }