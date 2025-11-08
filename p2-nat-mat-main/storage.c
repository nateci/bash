/**
 * Storage module for a file system implementation.
 * 
 * This module provides core functionality for a simple file system,
 * including file and directory operations, inode management, and
 * block allocation.
 */

 #define _GNU_SOURCE
 #include <string.h>
 #include <stdlib.h>
 #include <errno.h>
 #include "inode.h"
 #include "helpers/blocks.h"
 #include "storage.h"
 #include "helpers/bitmap.h"
 #include <unistd.h>
 #include "directory.h"
 #include <sys/stat.h>
 
 /**
  * Initializes the storage system.
  * 
  * Sets up the block storage at the specified path and initializes
  * the root directory if it doesn't exist or is invalid.
  * 
  * @param path The path to the storage file/device
  */
 void storage_init(const char *path) {
     blocks_init(path);
 
     int root_inum = blocks_get_root_block();

     inode_t *root = get_inode(root_inum);
 
     if (root_inum <= 0 || !root || !S_ISDIR(root->mode)) {
 
         root_inum = alloc_inode();

         inode_t *new_root = get_inode(root_inum);
         new_root->mode = S_IFDIR | 0755;
         print_inode(new_root); 
         new_root->block = alloc_block();
         new_root->size = 5 * sizeof(dir_entry_t);
 
         dir_entry_t *entries = blocks_get_block(new_root->block);
         memset(entries, 0, BLOCK_SIZE);
         strcpy(entries[0].name, ".");
         entries[0].inum = root_inum;
         strcpy(entries[1].name, "..");
         entries[1].inum = root_inum;
 
         blocks_set_root_block(root_inum);
         blocks_flush();
 
         root = new_root;  // Update root pointer
         print_inode(new_root); 
     }
 
     if (!S_ISDIR(root->mode)) {
         fprintf(stderr, "Error: Invalid root inode even after init\n");
         exit(1);
     }
 }
 
 /**
  * Gets metadata about a file or directory.
  * 
  * @param path The path to the file or directory
  * @param st Pointer to a stat structure to fill with metadata
  * @return 0 on success, negative error code on failure
  */
 int storage_stat(const char *path, struct stat *st) {
     // TODO: Implement proper path lookup
     if (strcmp(path, "/") == 0) {
         st->st_mode = 040755;
         st->st_size = 0;
         st->st_uid = getuid();
         return 0;
     }
     return -ENOENT;
 }
 
 /**
  * Reads data from a file.
  * 
  * @param path The path to the file
  * @param buf Buffer to store the read data
  * @param size Number of bytes to read
  * @param offset Starting position for reading
  * @return Number of bytes read on success, negative error code on failure
  */
 int storage_read(const char *path, char *buf, size_t size, off_t offset) {
     // TODO: Implement proper file reading
     if (strcmp(path, "/hello.txt") == 0) {
         strcpy(buf, "hello\n");
         return 6;
     }
     return -ENOENT;
 }
 
 /**
  * Prints the current state of the file system.
  * 
  * Displays the root inode information and lists all entries in the root directory.
  */
 void print_storage_status() {
     printf("Storage status:\n");
     printf("  Root inode: %d\n", storage_lookup_path("/"));
     printf("  Files in root:\n");
     
     inode_t *root = get_inode(0);
     dir_entry_t *entries = blocks_get_block(root->block);
     int count = root->size / sizeof(dir_entry_t);
     
     for (int i = 0; i < count; i++) {
         printf("    %s -> inode %d\n", entries[i].name, entries[i].inum);
     }
 }
 
 /**
  * Writes data to a file.
  * 
  * @param path The path to the file
  * @param buf The data to write
  * @param size Number of bytes to write
  * @param offset Starting position for writing
  * @return Number of bytes written on success, negative error code on failure
  */
 int storage_write(const char *path, const char *buf, size_t size, off_t offset) {
     int inum = storage_lookup_path(path);
     if (inum < 0) return -ENOENT;
     
     inode_t *node = get_inode(inum);
     if (!node || !S_ISREG(node->mode)) return -EISDIR;
     
     // Calculate required blocks
     size_t required_blocks = (offset + size + BLOCK_SIZE - 1) / BLOCK_SIZE;
     size_t current_blocks = (node->size + BLOCK_SIZE - 1) / BLOCK_SIZE;
     
     // Allocate additional blocks if needed
     if (required_blocks > current_blocks) {
         for (size_t i = current_blocks; i < required_blocks; i++) {
             int new_block = alloc_block();
             if (new_block < 0) return -ENOSPC;
             // For multi-block support, you'll need to implement block pointers
             // This is a simplified version assuming single block
             if (i == 0) node->block = new_block;
         }
     }
     
     // Write data (simplified single-block version)
     void *block = blocks_get_block(node->block);
     memcpy(block + offset, buf, size);
     
     // Update size
     if (offset + size > node->size) {
         node->size = offset + size;
     }
     
     return size;
 }
 
 /**
  * Creates a new file.
  * 
  * @param path The path to the new file
  * @param mode File permissions and type
  * @return 0 on success, negative error code on failure
  */
 int storage_mknod(const char *path, mode_t mode) {
     // Split into directory and filename
     char *path_copy = strdup(path);
     char *filename = strrchr(path_copy, '/');
     if (!filename) {
         free(path_copy);
         return -EINVAL;
     }
     *filename++ = '\0';
 
     // Get parent directory
     const char *parent_path = (path_copy[0] == '\0') ? "/" : path_copy;
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
 
     // Check if file exists
     if (directory_lookup(parent, filename) >= 0) {
         free(path_copy);
         return -EEXIST;
     }
 
     // Create new file
     int inum = alloc_inode();
     if (inum < 0) {
         free(path_copy);
         return -ENOSPC;
     }
 
     inode_t *node = get_inode(inum);
     node->mode = mode;
     node->size = 0;
     node->block = alloc_block();
     if (node->block < 0) {
         free_inode(inum);
         free(path_copy);
         return -ENOSPC;
     }
 
     // Add to directory
     int rv = directory_put(parent, filename, inum);
     if (rv < 0) {
         free_block(node->block);
         free_inode(inum);
         free(path_copy);
         return rv;
     }
 
     free(path_copy);
     return 0;
 }
 
 /**
  * Creates a new file in a specified directory.
  * 
  * @param parent_inum The inode number of the parent directory
  * @param name The name of the new file
  * @param mode File permissions and type
  * @return 0 on success, negative error code on failure
  */
 int storage_mknod_at(int parent_inum, const char *name, mode_t mode) {
     inode_t *parent = get_inode(parent_inum);
     if (!parent || !S_ISDIR(parent->mode)) return -ENOTDIR;
 
     // Check if file exists
     if (directory_lookup(parent, name) >= 0) return -EEXIST;
 
     // Allocate new file
     int inum = alloc_inode();
     if (inum < 0) return -ENOSPC;
 
     inode_t *node = get_inode(inum);
     node->mode = mode;
     node->size = 0;
     node->block = alloc_block();
     if (node->block < 0) {
         free_inode(inum);
         return -ENOSPC;
     }
 
     // Add to directory
     return directory_put(parent, name, inum);
 }
 
 /**
  * Removes a file.
  * 
  * @param path The path to the file to remove
  * @return 0 on success, negative error code on failure
  */
 int storage_unlink(const char *path) {
     printf("unlink(%s)\n", path);
     
     // Split path into parent and filename
     char *path_copy = strdup(path);
     if (!path_copy) return -ENOMEM;
     
     char *filename = strrchr(path_copy, '/');
     if (!filename) {
         free(path_copy);
         return -EINVAL;
     }
     *filename++ = '\0'; // Split into dirpath and filename
     
     // Get parent directory
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
     
     // Find the file
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
     
     // Can't unlink directories (use rmdir instead)
     if (S_ISDIR(node->mode)) {
         free(path_copy);
         return -EISDIR;
     }
     
     // Remove directory entry FIRST
     int rv = directory_delete(parent, filename);
     if (rv != 0) {
         free(path_copy);
         return rv;
     }
     
     // THEN free resources
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
  * Creates a new directory.
  * 
  * @param path The path to the new directory
  * @param mode Directory permissions
  * @return 0 on success, negative error code on failure
  */
 int storage_mkdir(const char *path, mode_t mode) {
     // Verify path starts with /
     if (path[0] != '/') return -EINVAL;
     
     // Split into parent path and new dir name
     char *path_copy = strdup(path);
     char *dirname = strrchr(path_copy, '/');
     if (!dirname) {
         free(path_copy);
         return -EINVAL;
     }
     *dirname = '\0';
     dirname++;
     
     // Get parent directory
     const char *parent_path = path_copy[0] ? path_copy : "/";
     int parent_inum = storage_lookup_path(parent_path);
     free(path_copy);
     
     if (parent_inum < 0) return parent_inum;
     
     inode_t *parent = get_inode(parent_inum);
     if (!parent || !S_ISDIR(parent->mode)) {
         return -ENOTDIR;
     }
 
     // Check if directory already exists
     if (directory_lookup(parent, dirname) >= 0) {
         return -EEXIST;
     }
 
     // Create new directory
     int inum = alloc_inode();
     if (inum < 0) return -ENOSPC;
     
     inode_t *dir = get_inode(inum);
     dir->mode = S_IFDIR | (mode & 0777);
     dir->block = alloc_block();
     if (dir->block < 0) {
         free_inode(inum);
         return -ENOSPC;
     }
 
     // Initialize directory contents
     dir_entry_t *entries = blocks_get_block(dir->block);
     memset(entries, 0, BLOCK_SIZE);
     
     // Create . and .. entries
     strncpy(entries[0].name, ".", DIR_NAME_LENGTH);
     entries[0].inum = inum;
     strncpy(entries[1].name, "..", DIR_NAME_LENGTH);
     entries[1].inum = parent_inum;
     dir->size = 2 * sizeof(dir_entry_t);
 
     // Add to parent directory
     dir_entry_t *parent_entries = blocks_get_block(parent->block);
     int parent_count = parent->size / sizeof(dir_entry_t);
     
     // Find empty slot or extend directory
     int slot = -1;
     for (int i = 0; i < parent_count; i++) {
         if (parent_entries[i].name[0] == '\0') {
             slot = i;
             break;
         }
     }
     
     if (slot == -1) { // Need to extend directory
         if (parent->size + sizeof(dir_entry_t) > BLOCK_SIZE) {
             free_block(dir->block);
             free_inode(inum);
             return -ENOSPC;
         }
         slot = parent_count;
         parent->size += sizeof(dir_entry_t);
     }
     
     // Add the new entry
     strncpy(parent_entries[slot].name, dirname, DIR_NAME_LENGTH);
     parent_entries[slot].inum = inum;
     parent->mtime = time(NULL);
     
     return 0;
 }
 
 /**
  * Looks up an inode number by path.
  * 
  * @param path The path to look up
  * @return Inode number on success, negative error code on failure
  */
 int storage_lookup_path(const char *path) {
     if (strcmp(path, "/") == 0) return 0;
 
     char *path_copy = strdup(path);
     char *saveptr;
     char *component = strtok_r(path_copy, "/", &saveptr);
     int current_inum = 0; // Start at root
 
     while (component) {
         inode_t *current = get_inode(current_inum);
         if (!current || !S_ISDIR(current->mode)) {
             free(path_copy);
             return -ENOTDIR;
         }
 
         int next_inum = directory_lookup(current, component);
         if (next_inum < 0) {
             free(path_copy);
             return -ENOENT;
         }
 
         current_inum = next_inum;
         component = strtok_r(NULL, "/", &saveptr);
     }
 
     free(path_copy);
     return current_inum;
 }
 
 /**
  * Creates a new directory in a specified parent directory.
  * 
  * @param parent_inum The inode number of the parent directory
  * @param name The name of the new directory
  * @param mode Directory permissions
  * @return 0 on success, negative error code on failure
  */
 int storage_mkdir_at(int parent_inum, const char *name, mode_t mode) {
     printf("[mkdir_at] name='%s'\n", name);
     printf("Adding directory entry: '%s' (length %ld)\n", name, strlen(name));
     inode_t *parent = get_inode(parent_inum);
     if (!parent || !S_ISDIR(parent->mode)) return -ENOTDIR;
     
     // Check if directory already exists
     if (directory_lookup(parent, name) >= 0) return -EEXIST;
     
     // Allocate new directory
     int inum = alloc_inode();
     if (inum < 0) return -ENOSPC;
     
     inode_t *dir = get_inode(inum);
     dir->mode = S_IFDIR | (mode & 0777);
     dir->size = 0;
     dir->block = alloc_block();
     if (dir->block < 0) {
         free_inode(inum);
         return -ENOSPC;
     }
     
     // Initialize directory contents
     dir_entry_t *entries = blocks_get_block(dir->block);
     memset(entries, 0, BLOCK_SIZE);
     
     // Create . and .. entries
     strncpy(entries[0].name, ".", DIR_NAME_LENGTH);
     entries[0].inum = inum;
     strncpy(entries[1].name, "..", DIR_NAME_LENGTH);
     entries[1].inum = parent_inum;
     dir->size = 2 * sizeof(dir_entry_t);
     
     // Add to parent directory
     return directory_put(parent, name, inum);
 }
 
 /**
  * Creates a directory path, creating parent directories as needed.
  * Similar to the "mkdir -p" command.
  * 
  * @param path The directory path to create
  * @param mode Directory permissions
  * @return 0 on success, negative error code on failure
  */
 int storage_mkdir_p(const char *path, mode_t mode) {
     if (strcmp(path, "/") == 0) return 0; // Root exists
 
     char *path_copy = strdup(path);
     char *saveptr;
     char *component = strtok_r(path_copy, "/", &saveptr);
     int current_inum = 0; // Start at root
     inode_t *current = get_inode(current_inum);
 
     while (component) {
         int next_inum = directory_lookup(current, component);
         if (next_inum < 0) {
             // Create new directory
             next_inum = alloc_inode();
             if (next_inum < 0) {
                 free(path_copy);
                 return -ENOSPC;
             }
 
             inode_t *new_dir = get_inode(next_inum);
             new_dir->mode = S_IFDIR | (mode & 0777);
             new_dir->block = alloc_block();
             if (new_dir->block < 0) {
                 free_inode(next_inum);
                 free(path_copy);
                 return -ENOSPC;
             }
 
             // Initialize directory
             dir_entry_t *entries = blocks_get_block(new_dir->block);
             strcpy(entries[0].name, ".");
             entries[0].inum = next_inum;
             strcpy(entries[1].name, "..");
             entries[1].inum = current_inum;
             new_dir->size = 2 * sizeof(dir_entry_t);
 
             // Add to parent
             if (directory_put(current, component, next_inum) < 0) {
                 free_block(new_dir->block);
                 free_inode(next_inum);
                 free(path_copy);
                 return -EIO;
             }
         }
 
         current_inum = next_inum;
         current = get_inode(current_inum);
         component = strtok_r(NULL, "/", &saveptr);
     }
 
     free(path_copy);
     return 0;
 }
 
 /**
  * Initializes a directory with "." and ".." entries.
  * 
  * @param dir Pointer to the directory inode
  * @param parent_inum Inode number of the parent directory
  */
 void init_directory(inode_t *dir, int parent_inum) {
     // Clear the entire block
     void *block = blocks_get_block(dir->block);
     memset(block, 0, BLOCK_SIZE);
     
     dir_entry_t *entries = block;
     
     // Add . entry
     strncpy(entries[0].name, ".", sizeof(entries[0].name));
     entries[0].inum = dir->inum;
     
     // Add .. entry
     strncpy(entries[1].name, "..", sizeof(entries[1].name));
     entries[1].inum = parent_inum;
     
     dir->size = 2 * sizeof(dir_entry_t);
 }
 
 /**
  * Prints debug information about a directory.
  * 
  * @param inum The inode number of the directory to print
  */
 void debug_print_directory(int inum) {
     inode_t *dir = get_inode(inum);
     if (!dir || !S_ISDIR(dir->mode)) {
         printf("Invalid directory inode %d\n", inum);
         return;
     }
     
     printf("\nDirectory inode %d (mode %o, size %d):\n", 
            inum, dir->mode, dir->size);
     
     dir_entry_t *entries = blocks_get_block(dir->block);
     int count = dir->size / sizeof(dir_entry_t);
     
     for (int i = 0; i < count; i++) {
         printf("  [%d] '%.*s' -> inode %d\n", 
               i, DIR_NAME_LENGTH, entries[i].name, entries[i].inum);
         
         // Print raw bytes for debugging
         printf("    Raw bytes: ");
         for (int j = 0; j < sizeof(dir_entry_t); j++) {
             printf("%02x ", ((unsigned char*)&entries[i])[j]);
             if (j % 16 == 15) printf("\n             ");
         }
         printf("\n");
     }
 }