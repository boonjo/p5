#include <stdio.h>
#include "ext2_fs.h"
#include "read_ext2.h"

// Takes two arguments, an input file that contains the disk image and an output directory where your output files will be stored.
int main(int argc, char **argv) {
    if (argc != 2) {
        printf("expected usage: ./runscan inputfile outputdir\n");
        exit(0);
    }

    int fd;
    fd = open(argv[1], O_RDONLY);    /* open disk image */
    
    ext2_read_init(fd);

    struct ext2_super_block super;
    struct ext2_group_desc group;
    

    // Reconstruct all jpg files from a given disk image.    
    // Scan all inodes that represent regular files and check if the first data block of the inode contains the jpg magic numbers: 
    // FF D8 FF E0 or FF D8 FF E1 or FF D8 FF E8
    for (unsigned int i = 0; i < inodes_per_block; i++) {
        struct ext2_inode *inode = malloc(sizeof(struct ext2_inode));
        read_inode(fd, start_inode_table, i, inode);
        
        // Check if the inode is a regular file
        if (S_ISREG(inode->i_mode)) {
            // Check if the first data block of the inode contains the jpg magic numbers
            unsigned int first_block = inode->i_block[0];
            unsigned char *block = malloc(block_size);
            read_block(fd, first_block, block);
            int is_jpg = 0;
            if (block[0] == 0xFF && block[1] == 0xD8 && block[2] == 0xFF && (block[3] == 0xE0 || block[3] == 0xE1 || block[3] == 0xE8)) {
                is_jpg = 1;
                // copy the content of that file to an output file (stored in your 'output/' directory), using the inode number as the file name
                // THIS STILL NEEDS TO BE IMPLEMENTED
                char *filename = malloc(sizeof(char) * 20);
                sprintf(filename, "%u", i);
                FILE *f = fopen(filename, "w");
                fwrite(block, block_size, 1, f);
                fclose(f);
            }
        }
    }

    // Find out the filenames of those inodes that represent the jpg files.
    // Scan all directory data blocks to find the corresponding filenames.
    // After you get the filename of a jpg file, you should again copy the content of that file to an output file, 
    // but this time, using the actual filename.
    for (unsigned int i = 0; i < inodes_per_block; i++) {
        struct ext2_inode *inode = malloc(sizeof(struct ext2_inode));
        read_inode(fd, start_inode_table, i, inode);

        // Check if the inode is a directory
        if (S_ISDIR(inode->i_mode)) {
            // THIS STILL NEEDS TO BE IMPLEMENTED
        }
}