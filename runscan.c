#include <stdio.h>
#include<string.h>
#include "ext2_fs.h"
#include "read_ext2.h"

#define SIZE_OF_BLOCK 1024

int main(int argc, char **argv) {
	if (argc != 3) {
		printf("expected usage: ./runscan inputfile outputfile\n");
		exit(0);
	}
	
	int fd;

	fd = open(argv[1], O_RDONLY);    /* open disk image */

	ext2_read_init(fd);

	struct ext2_super_block super;
	struct ext2_group_desc group;
	
	// example read first the super-block and group-descriptor
	read_super_block(fd, 0, &super);
	read_group_desc(fd, 0, &group);
	
	printf("There are %u inodes in an inode table block and %u blocks in the idnode table\n", inodes_per_block, itable_blocks);
	//iterate the first inode block
	off_t start_inode_table = locate_inode_table(0, &group);
    for (unsigned int i = 0; i < inodes_per_block; i++) {
            printf("inode %u: \n", i);
            struct ext2_inode *inode = malloc(sizeof(struct ext2_inode));
            read_inode(fd, 0, start_inode_table, i, inode);
	    /* the maximum index of the i_block array should be computed from i_blocks / ((1024<<s_log_block_size)/512)
			 * or once simplified, i_blocks/(2<<s_log_block_size)
			 * https://www.nongnu.org/ext2-doc/ext2.html#i-blocks
			 */
			unsigned int i_blocks = inode->i_blocks/(2<<super.s_log_block_size);
            printf("number of blocks %u\n", i_blocks);
             printf("Is directory? %s \n Is Regular file? %s\n",
                S_ISDIR(inode->i_mode) ? "true" : "false",
                S_ISREG(inode->i_mode) ? "true" : "false");


			//is a regular file
			if(S_ISREG(inode->i_mode)){
				char buffer[SIZE_OF_BLOCK];	
				int block_num;

			// print i_block numberss
			// delve into the specific file
				for(unsigned int i=0; i<EXT2_N_BLOCKS; i++)
				{       if (i < EXT2_NDIR_BLOCKS) {                                /* direct blocks */
							printf("Block %2u : %u\n", i, inode->i_block[i]);
							//read the content of the block (block[inode->i_block[i]])
							if(i == 0){ //only check the first data block
								lseek(fd, BLOCK_OFFSET(inode->i_block[i]), SEEK_SET);        
								read(fd, &buffer, SIZE_OF_BLOCK);   

								int is_jpg = 0;
								// check if the first data block of the inode contains the jpg magic numbers
								if (buffer[0] == (char)0xff &&
									buffer[1] == (char)0xd8 &&
									buffer[2] == (char)0xff &&
									(buffer[3] == (char)0xe0 ||
									buffer[3] == (char)0xe1 ||
									buffer[3] == (char)0xe8)) {
									is_jpg = 1;
								}
								if(is_jpg == 1)
									printf("found: inode: %u\n\n\n\n\n", i);
							}	
						}
						else if (i == EXT2_IND_BLOCK){                             /* single indirect block */
								printf("Single   : %u\n", inode->i_block[i]);
								lseek(fd, BLOCK_OFFSET(inode->i_block[i]), SEEK_SET);        
								read(fd, &buffer, SIZE_OF_BLOCK);  

								block_num = *((int*)(buffer)+0); //dereferenced the first 4 bytes pointer
								lseek(fd, BLOCK_OFFSET(block_num), SEEK_SET);        
								read(fd, &buffer, SIZE_OF_BLOCK);  

								int is_jpg = 0;
								if (buffer[0] == (char)0xff &&
									buffer[1] == (char)0xd8 &&
									buffer[2] == (char)0xff &&
									(buffer[3] == (char)0xe0 ||
									buffer[3] == (char)0xe1 ||
									buffer[3] == (char)0xe8)) {
									is_jpg = 1;
								}
								if(is_jpg == 1)
									printf("found: inode: %u\n\n\n\n\n", i);
						}
						else if (i == EXT2_DIND_BLOCK) {                           /* double indirect block */
								
								printf("Double   : %u\n", inode->i_block[i]);
								lseek(fd, BLOCK_OFFSET(inode->i_block[i]), SEEK_SET);        
								read(fd, &buffer, SIZE_OF_BLOCK);  

								block_num = *((int*)(buffer)+0); //dereferenced the 4 bytes pointer
								lseek(fd, BLOCK_OFFSET(block_num), SEEK_SET);        
								read(fd, &buffer, SIZE_OF_BLOCK);   
								block_num = *((int*)(buffer)+0); //double indirect, direferenced again
								lseek(fd, BLOCK_OFFSET(block_num), SEEK_SET);        
								read(fd, &buffer, SIZE_OF_BLOCK);        

								//debug
								printf("buffer[0]: %u", buffer[0]);	
								printf("buffer[1]: %u", buffer[1]);
								printf("buffer[2]: %u", buffer[2]);    
								printf("buffer[3]: %u", buffer[3]); 
								printf("buffer[4]: %u", buffer[4]);
								printf("buffer[7]: %u", buffer[7]); 

								int is_jpg = 0;
								if (buffer[0] == (char)0xff &&
									buffer[1] == (char)0xd8 &&
									buffer[2] == (char)0xff &&
									(buffer[3] == (char)0xe0 ||
									buffer[3] == (char)0xe1 ||
									buffer[3] == (char)0xe8)) {
									is_jpg = 1;
								}
								if(is_jpg == 1)
									printf("found: inode: %u\n\n\n\n\n", i);
						}
						else if (i == EXT2_TIND_BLOCK) {                           /* triple indirect block */

								printf("Triple   : %u\n", inode->i_block[i]);
								lseek(fd, BLOCK_OFFSET(inode->i_block[i]), SEEK_SET);        
								read(fd, &buffer, SIZE_OF_BLOCK);   

								//triple indirect, read 3 more times!
								block_num = *((int*)(buffer)+0); //dereferenced the 4 bytes pointer
								lseek(fd, BLOCK_OFFSET(block_num), SEEK_SET);        
								read(fd, &buffer, SIZE_OF_BLOCK);   
								block_num = *((int*)(buffer)+0); 
								lseek(fd, BLOCK_OFFSET(block_num), SEEK_SET);        
								read(fd, &buffer, SIZE_OF_BLOCK);     
								block_num = *((int*)(buffer)+0); 
								lseek(fd, BLOCK_OFFSET(block_num), SEEK_SET);        
								read(fd, &buffer, SIZE_OF_BLOCK);   //now the buffer contains the content of the first data block    

								int is_jpg = 0;
								if (buffer[0] == (char)0xff &&
									buffer[1] == (char)0xd8 &&
									buffer[2] == (char)0xff &&
									(buffer[3] == (char)0xe0 ||
									buffer[3] == (char)0xe1 ||
									buffer[3] == (char)0xe8)) {
									is_jpg = 1;
								}
								if(is_jpg == 1)
									printf("found: inode: %u\n\n\n\n\n", i);
						}

				}
			}
			else if(S_ISDIR(inode->i_mode)){
				for(unsigned int i=0; i<EXT2_N_BLOCKS; i++)
				{       if (i < EXT2_NDIR_BLOCKS) {                                /* direct blocks */
							printf("Block %2u : %u\n", i, inode->i_block[i]);
							if (S_ISREG(inode->i_mode)) {
								char buffer[SIZE_OF_BLOCK];
								//read the content of the block (block[inode->i_block[i]])
								lseek(fd, BLOCK_OFFSET(inode->i_block[i]), SEEK_SET);        
								read(fd, &buffer, SIZE_OF_BLOCK);              						
							}
						}
						else if (i == EXT2_IND_BLOCK)                             /* single indirect block */
								printf("Single   : %u\n", inode->i_block[i]);
						else if (i == EXT2_DIND_BLOCK)                            /* double indirect block */
								printf("Double   : %u\n", inode->i_block[i]);
						else if (i == EXT2_TIND_BLOCK)                            /* triple indirect block */
								printf("Triple   : %u\n", inode->i_block[i]);

				}
			}

				// this inode represents a regular file
			
			
            free(inode);

        }

	
	close(fd);
}
