#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <dirent.h>
#include "ext2_fs.h"
#include "read_ext2.h"

#define SIZE_OF_BLOCK 1024
#define SIZE_OF_POINTER 4





 




int isJPG(char* data);
void write_block_data(FILE* fp, int i_blocks, struct ext2_inode *inode, char* buffer);

int main(int argc, char **argv) {
	if (argc != 3) {
		printf("expected usage: ./runscan inputfile outputfile\n");
		exit(0);
	}

	if (opendir(argv[2]) != NULL){ //already exists
		printf("error: dir already existed\n");
		exit(1);
	}
	
	mkdir(argv[2], 0777);

	
	// char outputpath[50] ;
	// strcpy(outputpath, argv[2]);
	//printf("outputpath: %s", outputpath);
	//exit(0);
	
	int fd;

	fd = open(argv[1], O_RDONLY);    /* open disk image */

	ext2_read_init(fd);

	struct ext2_super_block super;
	struct ext2_group_desc group;
	
	// example read first the super-block and group-descriptor
	read_super_block(fd, 0, &super);
	read_group_desc(fd, 0, &group);

	unsigned int total_groups = super.s_blocks_count / super.s_blocks_per_group; //total number of groups 
	printf("debug: total_groups: %u\n\n", total_groups);
	printf("There are %u inodes in an inode table block and %u blocks in the idnode table\n", inodes_per_block, itable_blocks);

	int found_inode_numbers[512]; //detected inodes for jpg files. 之前写的20就错了！地没有留充分！最后一个test 100个jpgs！
	int freePosition = 0;
	int is_jpg;
	int size_to_write; //how many bytes of the file left to copy
	int opened;
	int out_fd;
	//FILE *fp = NULL;
	

	//iterate the first inode block
	for (unsigned int j = 0; j < total_groups; j++) {

		off_t start_inode_table = locate_inode_table(j, &group);
		//off_t start_inode_table = locate_inode_table(0, &group);

		//inodes_per_block = block_size / sizeof(struct ext2_inode);		/* number of inodes per data block */
		//printf("sizeof(struct ext2_inode): %lu\n\n\n", sizeof(struct ext2_inode)); //128. debug
		//printf("s_inodes_per_group: %u\n\n\n\n\n", super.s_inodes_per_group); //128. debug
		

		//loop through all inodes in the inode table
		for (unsigned int i = 0; i < super.s_inodes_per_group; i++) { //is this condition supposed to be s_inodes_per_group? originally it's  i < inodes_per_block
				//printf("inode %u: \n", i);
				int current_inode_number = i;
				struct ext2_inode *inode = malloc(sizeof(struct ext2_inode));

			
				read_inode(fd, j, start_inode_table, i, inode); //read all the group, instead of just group 0
				//read_inode(fd, 0, start_inode_table, i, inode);
			/* the maximum index of the i_block array should be computed from i_blocks / ((1024<<s_log_block_size)/512)
				* or once simplified, i_blocks/(2<<s_log_block_size)
				* https://www.nongnu.org/ext2-doc/ext2.html#i-blocks
				*/
				//unsigned int i_blocks = inode->i_blocks/(2<<super.s_log_block_size);
				// printf("number of blocks %u\n", i_blocks);
				// printf("Is directory? %s \n Is Regular file? %s\n",
				// 	S_ISDIR(inode->i_mode) ? "true" : "false",
				// 	S_ISREG(inode->i_mode) ? "true" : "false");





				





				//is a regular file
				if(S_ISREG(inode->i_mode)){
					char buffer[SIZE_OF_BLOCK];	
					int block_num;
					is_jpg = 0; //initialize to 0 for every regular file
					opened = 0; //whether the fopen is already opened. initialize to 0 for every regular file
					

					// print i_block numberss
					// delve into the specific file
					for(unsigned int i=0; i<EXT2_N_BLOCKS; i++){       
						if (i < EXT2_NDIR_BLOCKS) {                                /* direct blocks */

							//printf("current_inode_number: %i\n", current_inode_number);
							//printf("index of i_block %i\n", i);
							if(inode->i_block[i] == 0) //unused
								break;	

							//printf("Block %2u : %u\n", i, inode->i_block[i]);
							lseek(fd, BLOCK_OFFSET(inode->i_block[i]), SEEK_SET);     
							read(fd, buffer, SIZE_OF_BLOCK);  //buffer or &buffer doesn't make diffeence?

							//printf("buffer for block %i: %s\n", inode->i_block[i], buffer); //debug


							if(i == 0){ //only check the first data block for detecting jpg files
								if(isJPG(buffer)) //check if the first data block of the inode contains the jpg magic numbers
									is_jpg = 1;
								else
									break; //this file is not jpg. useless in this project
							}	
							//printf("debug: line97\n\n");
							//printf("debug: isjpg: %i\n", is_jpg);
							if(is_jpg == 1){ //or this condition can be write as  i==0
								if(!opened){ //make sure only open the file once
									//printf("debug: opened: %i\n", opened);

									//compose the output filename. TODO: put this into a function
									char filename[50];
									strcpy(filename,  argv[2]); //argv[2] is the output dir
									strcat(filename, "/");
									strcat(filename,  "file-");
									char number_str[10]; //make sure enough size
									sprintf(number_str, "%d", current_inode_number); //convert number to string. (e.x. 10 to "10")							
									strcat(filename, number_str); 	
									strcat(filename, ".jpg");
									//printf("filename: %s\n", filename);	

									out_fd = open(filename, O_CREAT | O_WRONLY, 0666); //TEMP CHANGE

									//fp = fopen(filename, "a"); 					//open in append mode to ease write
									//open(filename, O_CREAT, 0666);
									opened = 1;
									size_to_write = inode->i_size;				//initialize to the size of the file
									//printf("found: inode: %u\n\n\n\n\n", i);
									found_inode_numbers[freePosition++] = current_inode_number;
									//printf("debug: line119\n\n");
								}

								if(size_to_write >= SIZE_OF_BLOCK){ //copy full block
									//https://www.tutorialspoint.com/c_standard_library/c_function_fwrite.html
									//printf("buffer to write %s\n", buffer); //debug
									write(out_fd, buffer, SIZE_OF_BLOCK);//mistake here. previosly write size_to-write. shit mistake
									//fwrite(buffer, 1, size_to_write, fp); 
									//printf("current_inode_number: %i\n", current_inode_number);
									//write_block_data(fp, size_to_write, inode, buffer);//current inode represents the jpg file. 
									size_to_write -= SIZE_OF_BLOCK; 
									//printf("sizetowrite: %i\n", size_to_write);
								}
								else if (size_to_write == 0){
									//fclose(fp);
									close(out_fd);
									break;
								}
								else{ // 0 < size_to_write < full block size
									//printf("buffer to write %s\n", buffer); //debug
									write(out_fd, buffer, size_to_write);
									//fwrite(buffer, 1, size_to_write, fp); 
									size_to_write = 0;
									//printf("sizetowrite: %i\n", size_to_write); //VERY USEFUL DEBUG
									//fclose(fp); //I MUST CLOSE THE FILE!!! OTHERWISE ITS EMPTY
									close(out_fd);
									break; //done with that file
								}
							}
							//exit(0); //debug
						}
						else if (i == EXT2_IND_BLOCK){                             /* single indirect block */

							if(inode->i_block[i] == 0) //unused
								continue;	

							if(is_jpg == 1){ //otherwise no need to read the block (not needed in this project)
								//printf("Single   : %u\n", inode->i_block[i]);
								lseek(fd, BLOCK_OFFSET(inode->i_block[i]), SEEK_SET);        
								read(fd, &buffer, SIZE_OF_BLOCK);  //this buffer stores the pointers

								for(int k = 0; k < SIZE_OF_BLOCK / SIZE_OF_POINTER; k++){
									block_num = *((__u32*)(buffer)+k); //dereferenced the first 4 bytes pointer
									char data[SIZE_OF_BLOCK];
									lseek(fd, BLOCK_OFFSET(block_num), SEEK_SET);        
									read(fd, &data, SIZE_OF_BLOCK);  //this buffer then store the real data

									if(size_to_write >= SIZE_OF_BLOCK){ //copy full block
										write(out_fd, data, SIZE_OF_BLOCK);
										//fwrite(buffer, 1, size_to_write, fp);  
										size_to_write -= SIZE_OF_BLOCK; 
									}
									else if (size_to_write == 0){
										close(out_fd);
										//fclose(fp);
										break;
									}
									else{ // 0 < size_to_write < full block size
										write(out_fd, data, size_to_write);
										//fwrite(buffer, 1, size_to_write, fp); 
										size_to_write = 0;
										close(out_fd);
										//fclose(fp);
										break; 
									}
								}
								if (size_to_write == 0)
									break; //done with that file	
							} 
						}
						else if (i == EXT2_DIND_BLOCK) {                           /* double indirect block */

							if(inode->i_block[i] == 0 || is_jpg == 0 ) //unused or not jpg
								continue;	

							//printf("Double   : %u\n", inode->i_block[i]);
							lseek(fd, BLOCK_OFFSET(inode->i_block[i]), SEEK_SET);        
							read(fd, &buffer, SIZE_OF_BLOCK);  

							for(int j = 0; j < SIZE_OF_BLOCK / SIZE_OF_POINTER; j++){
								block_num = *((__u32*)(buffer)+j); //dereferenced the 4 bytes pointer
								char first_content[SIZE_OF_BLOCK];
								lseek(fd, BLOCK_OFFSET(block_num), SEEK_SET);        
								read(fd, &first_content, SIZE_OF_BLOCK);   
								
								for(int k = 0; k < SIZE_OF_BLOCK / SIZE_OF_POINTER; k++){
									//double indirect, direferenced again. this time buffer stores the real data
									block_num = *((__u32*)(first_content)+k); 
									lseek(fd, BLOCK_OFFSET(block_num), SEEK_SET);     
									char data[SIZE_OF_BLOCK];   
									read(fd, &data, SIZE_OF_BLOCK);   

									if(size_to_write >= SIZE_OF_BLOCK){ //copy full block
										write(out_fd, data, SIZE_OF_BLOCK);
										//fwrite(buffer, 1, size_to_write, fp);  
										size_to_write -= SIZE_OF_BLOCK; 
									}
									else if (size_to_write == 0){
										close(out_fd);
										//fclose(fp);
										break;
									}
									else{ // 0 < size_to_write < full block size
										write(out_fd, data, size_to_write);
										//fwrite(buffer, 1, size_to_write, fp); 
										size_to_write = 0;
										close(out_fd);
										//fclose(fp);
										break; 
									}	
								}   
								if (size_to_write == 0)
									break; //done with that file   
							}
							if (size_to_write == 0)
								break; //done with that file   
							
							printf("buffer[0]: %u", buffer[0]);	 //debug					
						}
						else if (i == EXT2_TIND_BLOCK) {                           /* triple indirect block */

							if(inode->i_block[i] == 0 || is_jpg == 0 ) //unused
								continue;	

							//printf("Triple   : %u\n", inode->i_block[i]);
							lseek(fd, BLOCK_OFFSET(inode->i_block[i]), SEEK_SET);        
							read(fd, &buffer, SIZE_OF_BLOCK);   

							//triple indirect, read 3 more times!
							for(int i = 0; i < SIZE_OF_BLOCK / SIZE_OF_POINTER; i++){
								block_num = *((int*)(buffer)+i); //dereferenced the 4 bytes pointer
								char first_content[SIZE_OF_BLOCK];
								lseek(fd, BLOCK_OFFSET(block_num), SEEK_SET);        
								read(fd, &first_content, SIZE_OF_BLOCK);   
								for(int j = 0; j < SIZE_OF_BLOCK / SIZE_OF_POINTER; j++){
									block_num = *((int*)(first_content)+j); 
									char second_content[SIZE_OF_BLOCK];
									lseek(fd, BLOCK_OFFSET(block_num), SEEK_SET);        
									read(fd, &second_content, SIZE_OF_BLOCK);   
									for(int k = 0; k < SIZE_OF_BLOCK / SIZE_OF_POINTER; k++){
										block_num = *((int*)(second_content)+k); 
										char data[SIZE_OF_BLOCK];
										lseek(fd, BLOCK_OFFSET(block_num), SEEK_SET);        
										read(fd, &data, SIZE_OF_BLOCK);   //this time buffer finally stores the real data 
										if(size_to_write >= SIZE_OF_BLOCK){ //copy full block
											write(out_fd, data, SIZE_OF_BLOCK);
											//fwrite(buffer, 1, size_to_write, fp);  
											size_to_write -= SIZE_OF_BLOCK; 
										}
										else if (size_to_write == 0){
											close(out_fd);
											//fclose(fp);
											break;
										}
										else{ // 0 < size_to_write < full block size
											write(out_fd, data,size_to_write);
											//fwrite(buffer, 1, size_to_write, fp); 
											size_to_write = 0;
											close(out_fd);
											//fclose(fp);
											break; 
										}
										
									}
									if (size_to_write == 0)
										break; //done with that file   
								}   
								if (size_to_write == 0)
									break; //done with that file   
							}
							if (size_to_write == 0)
								break; //done with that file   	
						}
						//fclose(fp); //SHIT. THIS IS THE PROBLEM!! CLOSE BEFORE WRITE FULL. BE CLEAR ABOUT THE EXECUTION FLOW! 
					}
				}
				free(inode);
			}
	} //DONE WITH PART1

//print all the detected inode numbers that corresponds to the jpg file
	for (int i = 0; i < freePosition; i++)
		printf("found_inode_numbers: %i\n", found_inode_numbers[i]);





/********************************************* PART 2 *********************************************/
__u32 INODE_NUM_BYTES = 4; //use 2 bytes to store the inode number (because __u32. see struct)
__u32 ENTRY_LENGTH_BYTES = 2; //use 2 bytes to store the entry length (because __u16)
__u32 NAME_LENGTH_BYTES = 1; //use 1 byte to store the name length (because __u8)
__u32 FILE_TYPE_BYTES = 1; //use 1 byte to store the name length (because __u8)
__u32 TOTAL_FIXED_BYTES = INODE_NUM_BYTES+ENTRY_LENGTH_BYTES+NAME_LENGTH_BYTES+FILE_TYPE_BYTES; //8

	//loop over again. traverse inside directory to recover the filename through inode-filename mapping
	for(int k = 0; k < freePosition; k++){ //loop through each inode number found in part1(inefficient though)
		int EXPECTED = found_inode_numbers[k]; //TODO: this implementation is inefficient. should find all mappings in one loop, not loop k times, so much more unecessary computation
		for (unsigned int j = 0; j < total_groups; j++) {
			off_t start_inode_table = locate_inode_table(j, &group);
			for (unsigned int i = 0; i < super.s_inodes_per_group; i++) { //一个地方改了以后另一个地方别忘了也改！之前我这里写的是初始的inodes_per_block
				//printf("inode %u: \n", i);
				struct ext2_inode *inode = malloc(sizeof(struct ext2_inode));
				read_inode(fd, j, start_inode_table, i, inode); //read all the group, instead of just group 0	
				//unsigned int i_blocks = inode->i_blocks/(2<<super.s_log_block_size); // = maximum index of the i_block array+1
				//printf("number of blocks %u\n", i_blocks); 
				if(S_ISDIR(inode->i_mode)){ 
					__u32 accumulated_offset = 0; // make sure not overflow
					int found_mapping = 1; //assume first that we found the mapping
					lseek(fd, BLOCK_OFFSET(inode->i_block[0]), SEEK_SET); //dir use only the first data block for this project        
					char buffer[SIZE_OF_BLOCK];
					read(fd, &buffer, SIZE_OF_BLOCK); //buffer copy the data(mappings) stored in the directory

					struct ext2_dir_entry_2* dir_entry = (struct ext2_dir_entry_2*)buffer; //nice, core to parse the entry.
					int inode_num; //inode_num in the current entry
					int offset;    //offset to the next entry
					//According to the structure of ext2_dir_entry_2 found in ext2_fs.h,
					
					int name_length; //length of the file name in bytes. divisible by 4! e.x. 5->8, 1->4, 4->4
					inode_num = dir_entry->inode;
					while (inode_num != EXPECTED){ //is this condition correct? (overflow?)
						if (dir_entry->name_len % 4 != 0)	
							name_length = dir_entry->name_len + 4 - dir_entry->name_len % 4; //make it 4bytes aligned. any better algorithm?
						offset = TOTAL_FIXED_BYTES + name_length; //the offset to next entry

						accumulated_offset += offset;
						if (accumulated_offset + TOTAL_FIXED_BYTES >= inode->i_size){ //reach the end of the dir data. not sure if this is correct
							found_mapping = 0;
							break;
						}
						
						dir_entry = (struct ext2_dir_entry_2*)((char*)dir_entry + offset); //point to the next entry
						inode_num = dir_entry->inode; //get next inode_num
						if(inode_num == 0){ //reach the end of the dir data. Avoid infinite looping
							found_mapping = 0;
							break;
						}
					}
					if(found_mapping){ //exit the previous while loop not because it reaches the end of data
						//inode_num == EXPECTED; found the entry corresponding to the deleted jpg file, which contains the inode num-filename mapping
						char name [EXT2_NAME_LEN];
						strncpy(name, dir_entry->name, dir_entry->name_len);
						name[dir_entry->name_len] = '\0';
						//copy data from the file in part1 to here. Otherwise need to do the same thing in part1 again, which makes no sense.
						//https://www.tutorialspoint.com/c-program-to-copy-the-contents-of-one-file-to-another-file
						char outputname[512]; //shit！之前这里是50，然后有一个filename名字特别长地就不够了！最好别hardcoding！另外留够地
						strcpy(outputname,  argv[2]); //argv[2] is the output dir
						strcat(outputname, "/");
						strcat(outputname,  name);
						
						
						//char ch;// source_file[20], target_file[20];
						//FILE *source, *target;
						char source_file[512];
						strcpy(source_file,  argv[2]); //argv[2] is the output dir
						strcat(source_file,  "/file-"); //argv[2] is the output dir
						char num[10]; 
						sprintf(num, "%d", inode_num);
						strcat(source_file, num); 						
						strcat(source_file, ".jpg");
						//strcat(name, ".jpg");
						//char target_file[] = name; //name or name.jpg? do we need to append .jpg?

						//printf("\ninode number is %i\n", inode_num);
						//printf("Entry name: %s\n", name);
						//printf("source_file: %s\n", source_file);
						//printf("outputname: %s\n", outputname);

						// source = fopen(source_file, "r");
						// if (source == NULL) {
						// 	exit(1);
						// }
						// target = fopen(outputname, "w");
						// if (target == NULL) {
						// 	fclose(source);
						// 	exit(1);
						// }

						// //debug: get file's size
						// int sz;
						// fseek(source, 0L, SEEK_END);
						// sz = ftell(source);
						// fseek(source, 0L, SEEK_SET);
						// printf("size: %i", sz);

						//debug: calculate bytes to copy:
						struct ext2_inode *jpg_inode = malloc(sizeof(struct ext2_inode));
						read_inode(fd, j, start_inode_table, inode_num, jpg_inode);
						int bytes_to_copy = jpg_inode->i_size;
						//printf("bytes_to_copy: %i\n", bytes_to_copy);
						free(jpg_inode);


						 
						int int_fd = open(source_file, O_RDONLY, 0666); //这个如果写O_WRONLY都不行！很奇怪！
						int out_fd = open(outputname, O_CREAT | O_WRONLY, 0666); 
						char data[SIZE_OF_BLOCK];

						if(bytes_to_copy > 20*1024*1024){ //debug specifically for test9
							printf("dir inode number : %i\n", i);
							printf("file inode number: %i\n", found_inode_numbers[k]);
							printf("file size: %i\n", bytes_to_copy);
							//printf("need to loop %i time\n", bytes_to_copy / SIZE_OF_BLOCK);
						}

						
						for(int i = 0; i < bytes_to_copy / SIZE_OF_BLOCK; i++)
						{
							read(int_fd,  data, SIZE_OF_BLOCK); 
							write(out_fd, data, SIZE_OF_BLOCK);					 	
						}
						read(int_fd,  data, bytes_to_copy % SIZE_OF_BLOCK); 
						write(out_fd, data, bytes_to_copy % SIZE_OF_BLOCK);

						// int temp_index;
						// for(temp_index = 0; temp_index < bytes_to_copy / SIZE_OF_BLOCK; temp_index++)
						// {
						// 	read(int_fd,  data, SIZE_OF_BLOCK); 
						// 	write(out_fd, data, SIZE_OF_BLOCK);					 	
						// }
						// printf("temp_index: %i\n", temp_index);
						// read(int_fd,  data, bytes_to_copy % SIZE_OF_BLOCK); 
						// write(out_fd, data, bytes_to_copy % SIZE_OF_BLOCK);
						//loop几百万次的话程序好像可能会卡崩

						// if(bytes_to_copy < 1024*1024){
						// 	char data[1024*1024];
						// 	read(int_fd,  data, bytes_to_copy); 
						// 	write(out_fd, data, bytes_to_copy);
						// }
						// else{ //large file
						// 	char data[1024*1024*10];
						// 	read(int_fd,  data, bytes_to_copy); 
						// 	write(out_fd, data, bytes_to_copy);
						// }
						
						close(int_fd);	
						close(out_fd);		

						if(k == freePosition-1){ //done with processing all files. 
							free(inode);
							close(fd);
							return 0;
						}



						//用这种方式copy会出现多一个EOF的issue
						// //ch = fgetc(source);
						// // while (ch != EOF) //if this doesn't work I can use i_size
						// for(int i = 0; i < bytes_to_copy * 8; i++)
						// {
						// 	ch = fgetc(source);
						// 	fputc(ch, target);	
						// }


						//这种方式压根copy不了！空的！
						// // while ((ch = fgetc(source)) != EOF)
						// // 	fputc(ch, target);


						// printf("File copied successfully.\n");
						// fclose(source);
						// fclose(target);
					}			
				}
				free(inode);
			}
		}
	}
				
	

	close(fd);
}



int isJPG(char* data){
	if (data[0] == (char)0xff &&
		data[1] == (char)0xd8 &&
		data[2] == (char)0xff &&
		(data[3] == (char)0xe0 ||
		data[3] == (char)0xe1 ||
		data[3] == (char)0xe8)) {
		return 1;
	}
	return 0;
}




/******************************************  TEMP USELESS CODE ******************************************/

//struct ext2_inode *inode_jpg = malloc(sizeof(struct ext2_inode));
//read_inode(fd, j, start_inode_table, current_inode_number, inode_jpg); 

//is the below two lines(provided code)useful?
//dentry = (struct ext2_dir_entry*) & ( buffer[68] );
//int name_len = dentry->name_len & 0xFF; // convert 2 bytes to 4 bytes properly

/*
 * write the data in the data block to the file pointed to by fp 
 * 
 * fp: where to write to. 
 * size_to_write: how many bytes to write
 * inode: 
 * buffer: where to write from 
 * 
*/
// void write_block_data(FILE* fp, int size_to_write, struct ext2_inode *inode, char* buffer){
// 	for (int i = 0; i < i_blocks; i++){
// 		char buffer[SIZE_OF_BLOCK];	
// 		lseek(fd, BLOCK_OFFSET(inode->i_block[i]), SEEK_SET);        
// 		read(fd, &buffer, SIZE_OF_BLOCK);   
// 		fwrite(buffer, 1, size_to_write, fp) //https://www.tutorialspoint.com/c_standard_library/c_function_fwrite.html
// 	}
// }
//我现在突然感觉不用这个函数了啊。直接写fwrite不就行了嘛



//doesn't work, no idea. works in brand new compiler
//just change to char filename[50]; strcpy(filename,  "file-"); works
//maybe because of the size? tricky				
// char filename[] = "file-";
// char number_str[10]; //make sure enough size
// sprintf(number_str, "%d", current_inode_number); //convert number to string. (e.x. 10 to "10")	
// printf("number_str: %s\n\n", number_str);						
// strcat(filename, number_str); 					 //append inode number to filename
// printf("debug: line109\n\n");	
// strcat(filename, ".jpg"); 					
// printf("debug: line111\n\n");	