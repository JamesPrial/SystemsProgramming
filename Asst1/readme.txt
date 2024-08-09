Mymalloc.c includes the custom implementation for malloc() and free(). The metadata structure used for blocks of manually allocated memory starts with a
size_t number representing the size of the block, followed by a char, being either '1', for malloc'd memory in use, or '2', for a free block. Metadata is
traversed to the next block by adding the size of the block and the size of metadata to the pointer to the current block. Created by James Prial
