/**
 * Inode management module for file system implementation.
 * 
 * This module provides functionality for managing inodes, which are data structures
 * that store metadata about files and directories in the file system.
 * It handles inode allocation, deallocation, retrieval, and printing.
 */

 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <time.h>
 #include "inode.h"
 #include "helpers/bitmap.h"
 
 // Inode table starts at block 1 (after block 0 which has bitmaps)
 #define INODE_TABLE_START 1
 #define INODES_PER_BLOCK (BLOCK_SIZE / sizeof(inode_t))
 
 /**
  * Retrieves an inode by its inode number.
  * 
  * @param inum The inode number to retrieve
  * @return Pointer to the inode structure, or NULL if the inode number is invalid
  */
 inode_t *get_inode(int inum) {
   if (inum < 0 || inum >= INODE_COUNT) {
     return NULL;
   }
   int block_num = INODE_TABLE_START + (inum / INODES_PER_BLOCK);
   int offset = (inum % INODES_PER_BLOCK) * sizeof(inode_t);
   return (inode_t *)((char *)blocks_get_block(block_num) + offset);
 }
 
 /**
  * Allocates a new inode from the inode bitmap.
  * 
  * Searches the inode bitmap for a free inode, marks it as allocated,
  * and initializes its basic fields (references, timestamps).
  * 
  * @return The inode number of the newly allocated inode, or -1 if no free inodes are available
  */
 int alloc_inode() {
   void *ibm = get_inode_bitmap();
   for (int i = 0; i < INODE_COUNT; ++i) {
     if (!bitmap_get(ibm, i)) {
       bitmap_put(ibm, i, 1);
       
       // Initialize the inode
       inode_t *node = get_inode(i);
       memset(node, 0, sizeof(inode_t));
       node->inum = i; // Store the inode number in the inode
       node->refs = 1;
       node->atime = node->mtime = node->ctime = time(NULL);
       
       printf("+ alloc_inode() -> %d\n", i);
       return i;
     }
   }
   return -1; // No free inodes
 }
 
 /**
  * Frees an inode and any associated blocks.
  * 
  * Marks the inode as free in the inode bitmap and releases any blocks
  * that were allocated to this inode.
  * 
  * @param inum The inode number to free
  */
 void free_inode(int inum) {
   printf("+ free_inode(%d)\n", inum);
   void *ibm = get_inode_bitmap();
   bitmap_put(ibm, inum, 0);
   
   // Free any blocks associated with this inode
   inode_t *node = get_inode(inum);
   if (node->block != 0) {
     free_block(node->block);
   }
 }
 
 /**
  * Prints the contents of an inode for debugging.
  * 
  * Displays the inode's reference count, mode, size, and associated block.
  *
  * @param node Pointer to the inode to print
  */
 void print_inode(inode_t *node) {
   if (node == NULL) {
     printf("NULL inode\n");
     return;
   }
   printf("inode{refs: %d, mode: %04o, size: %d, block: %d}\n",
          node->refs, node->mode, node->size, node->block);
 }