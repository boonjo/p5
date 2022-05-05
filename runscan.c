#include <stdio.h>
#include <string.h>
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

	unsigned int total_groups = super.s_blocks_count / super.s_blocks_per_group; //total number of groups 
	printf("debug: total_groups: %u\n\n\n\n\n", total_groups);
	printf("There are %u inodes in an inode table block and %u blocks in the idnode table\n", inodes_per_block, itable_blocks);

	int found_inode_numbers[20]; //detected inodes for jpg files
	int freePosition = 0;

	//iterate the first inode block
	for (unsigned int j = 0; j < total_groups; j++) {

		off_t start_inode_table = locate_inode_table(j, &group);
		//off_t start_inode_table = locate_inode_table(0, &group);

		//inodes_per_block = block_size / sizeof(struct ext2_inode);		/* number of inodes per data block */
		//printf("sizeof(struct ext2_inode): %lu\n\n\n", sizeof(struct ext2_inode)); //128. debug
		//printf("s_inodes_per_group: %u\n\n\n\n\n", super.s_inodes_per_group); //128. debug
		

		//loop through all inodes in the inode table
		for (unsigned int i = 0; i < super.s_inodes_per_group; i++) { //is this condition supposed to be s_inodes_per_group? originally it's  i < inodes_per_block
				printf("inode %u: \n", i);
				int current_inode = i;
				struct ext2_inode *inode = malloc(sizeof(struct ext2_inode));

			
				read_inode(fd, j, start_inode_table, i, inode); //read all the group, instead of just group 0
				//read_inode(fd, 0, start_inode_table, i, inode);
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
									if(is_jpg == 1){
										printf("found: inode: %u\n\n\n\n\n", i);
										found_inode_numbers[freePosition++] = current_inode;
									}
								}	
							}
							else if (i == EXT2_IND_BLOCK){                             /* single indirect block */
									printf("Single   : %u\n", inode->i_block[i]);
									lseek(fd, BLOCK_OFFSET(inode->i_block[i]), SEEK_SET);        
									read(fd, &buffer, SIZE_OF_BLOCK);  

									block_num = *((int*)(buffer)+0); //dereferenced the first 4 bytes pointer
									lseek(fd, BLOCK_OFFSET(block_num), SEEK_SET);        
									read(fd, &buffer, SIZE_OF_BLOCK);  
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

									
							}

					}
				}
				
					// this inode represents a regular file
				
				
				free(inode);

			}
	}

	
	//loop over again. traverse inside directory to recover the filename through inode-filename mapping
	int EXPECTED = 0; //TODO: CHANGE 0 TO THE INODE NUMBER OF THE JPG FILE
	for (unsigned int j = 0; j < total_groups; j++) {
		off_t start_inode_table = locate_inode_table(j, &group);
		for (unsigned int i = 0; i < inodes_per_block; i++) {
			printf("inode %u: \n", i);
			struct ext2_inode *inode = malloc(sizeof(struct ext2_inode));
			read_inode(fd, j, start_inode_table, i, inode); //read all the group, instead of just group 0	
			unsigned int i_blocks = inode->i_blocks/(2<<super.s_log_block_size); // = maximum index of the i_block array+1
			printf("number of blocks %u\n", i_blocks); 
			if(S_ISDIR(inode->i_mode)){ //dir use only the first data block for this project
				lseek(fd, BLOCK_OFFSET(inode->i_block[0]), SEEK_SET);        
				char buffer[SIZE_OF_BLOCK];
				read(fd, &buffer, SIZE_OF_BLOCK); //buffer copy the data(mappings) stored in the directory

				struct ext2_dir_entry_2* dir_entry = (struct ext2_dir_entry_2*)buffer; //nice, core to parse the entry.
				int inode_num; //inode_num in the current entry
				int offset;    //offset to the next entry
				//According to the structure of ext2_dir_entry_2 found in ext2_fs.h,
				int INODE_NUM_BYTES = 4; //use 2 bytes to store the inode number (because __u32. see struct)
				int ENTRY_LENGTH_BYTES = 2; //use 2 bytes to store the entry length (because __u16)
				int NAME_LENGTH_BYTES = 1; //use 1 byte to store the name length (because __u8)
				int FILE_TYPE_BYTES = 1; //use 1 byte to store the name length (because __u8)
				int TOTAL_FIXED_BYTES = INODE_NUM_BYTES+ENTRY_LENGTH_BYTES+NAME_LENGTH_BYTES+FILE_TYPE_BYTES; //8
				int name_length; //length of the file name in bytes. divisible by 4! e.x. 5->8, 1->4, 4->4
				inode_num = dir_entry->inode;
				while (inode_num != EXPECTED){ //is this condition correct? (overflow?)
					if (dir_entry->name_len % 4 != 0)	
						name_length = dir_entry->name_len + 4 - dir_entry->name_len % 4; //make it 4bytes aligned. any better algorithm?
					offset = TOTAL_FIXED_BYTES + name_length; //the offset to next entry
					dir_entry = (struct ext2_dir_entry_2*)((char*)dir_entry + offset); //point to the next entry
					inode_num = dir_entry->inode; //get next inode_num
				}
				//inode_num == EXPECTED; found the entry corresponding to the deleted jpg file, which contains the inode num-filename mapping
				char name [EXT2_NAME_LEN];
				strncpy(name, dir_entry->name, dir_entry->name_len);
				name[dir_entry->name_len] = '\0';
				printf("Entry name is --%s--", name);


				//is the below two lines(provided code)useful?
				//dentry = (struct ext2_dir_entry*) & ( buffer[68] );
				//int name_len = dentry->name_len & 0xFF; // convert 2 bytes to 4 bytes properly
				

				/*TODO: 
				* traverse inside mappings (recover deleted files)
				* provided function?
				* find the filename corresponding to the inode number of jpg file found in part1
				* canvas hints
				*/

				


				

				
			}
			free(inode);
		}
	}
				
	//print all the detected inode numbers that corresponds to the jpg file
	for (int i = 0; i < freePosition; i++)
		printf("found_inode_numbers: %i", found_inode_numbers[i]);
	
	
	
	close(fd);
}
