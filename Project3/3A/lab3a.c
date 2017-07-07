/*
NAME: Jaekwan Ahn, Aaron Van Doren
EMAIL: ahnjk0513@gmail.com, vandoren96@gmail.com
ID: 604057669, 504635123
*/

#include <linux/types.h>
#include <stdint.h>
#include <sys/types.h>
#include "ext2_fs.h" //header file

#include <stdio.h> //printf
#include <stdlib.h> //malloc
#include <unistd.h> //pread
#include <sys/stat.h> //open
#include <fcntl.h> //open
#include <string.h> //strlen
#include <math.h> //ceil
#include <time.h> //creation, modification, last access time

int image_fd;
struct ext2_super_block *super;
struct ext2_group_desc **group;
struct ext2_inode *inode;
struct ext2_dir_entry *directory;
int block_size;
int group_num;
int **dir_entries;
int entries_num;
struct ext2_inode* curr_inode;
int *s_indirect_block;
int *d_indirect_block;
int *t_indirect_block;

//declare here for use in get_inode
void get_indirect_blocks(int inodeNum);

void get_super_block(){
	//ext2_super_block is declared in header file "ext2_fs.h"
	super = malloc(sizeof(struct ext2_super_block));
	if(super == NULL){
		fprintf(stderr, "ERROR in malloc ext2_super_block\n");
		exit(2);
	}

	//offsets are given in the spec
	int super_offset = 1024;
	int super_size = 1024;

	//scan superblock in the file system. 
	if(pread(image_fd, super, super_size, super_offset) == -1){
		fprintf(stderr, "ERROR in pread() of super\n");
		exit(2);
	}
	block_size = 1024 << super->s_log_block_size;

	//A single new-line terminated line, comprised of eight comma-separated fields (with no white-space), summarizing the key file system parameters:
	printf("SUPERBLOCK,%d,%d,%d,%d,%d,%d,%d\n", super->s_blocks_count,
		super->s_inodes_count, block_size, super->s_inode_size, 
		super->s_blocks_per_group, super->s_inodes_per_group, 
		super->s_first_ino);
	return;
}

void get_group(){
	//find the number of groups
	double calc = ((double)super->s_blocks_count)/ super->s_blocks_per_group;
	group_num = (int) ceil(calc);
	//find the remaining blocks and inodes for the last group
	int blocks_last_group = super->s_blocks_count - (group_num - 1) * super->s_blocks_per_group;
	int inodes_last_group = super->s_inodes_count - (group_num - 1) * super->s_inodes_per_group;
	
	//ext2_group_desc_block is declared in header file "ext2_fs.h"
	group = malloc(sizeof(struct ext2_group_desc*) * group_num);
	if(group == NULL){
		fprintf(stderr, "ERROR in malloc ext2_group_desc\n");
		exit(2);
	}

	//offsets are given in the spec
	int group_offset = 2048;
	int group_size = 32;

	//execute for each group
	int i, num_blocks, num_inodes;
	for(i = 0; i < group_num; i++){
		if(i == (group_num - 1)){ //if it's the last group
			num_blocks = blocks_last_group;
			num_inodes = inodes_last_group;
		} else{
			num_blocks = super->s_blocks_per_group;
			num_inodes = super->s_inodes_per_group;
		}

		//scan each of the groups in the file system
		group[i] = malloc(sizeof(struct ext2_group_desc));
		if(pread(image_fd, group[i], group_size, group_offset + i * group_size) == -1){
			fprintf(stderr, "ERROR in pread() of group %d\n", i);
			exit(2);
		}

		//for each group, produce a new-line terminated line for each group, each comprised of nine comma-separated fields (with no white space), summarizing its contents
		printf("GROUP,%d,%d,%d,%d,%d,%d,%d,%d\n", i, num_blocks, num_inodes,
			group[i]->bg_free_blocks_count, group[i]->bg_free_inodes_count, 
			group[i]->bg_block_bitmap, group[i]->bg_inode_bitmap, group[i]->bg_inode_table);
	}
	return;
}

void get_free_block(){
	int i, j, k;
	for(i = 0; i < group_num; i++){
		int offset = group[i]->bg_block_bitmap * block_size;
		for(j = 0; j < block_size; j++){
			//scan the free block bitmap for each group
			int buffer;
			if(pread(image_fd, &buffer, 1, offset + j) == -1){
				fprintf(stderr, "ERROR in pread() of free block byte\n");
				exit(2);
			} 
			int byte_mask = 1;
			for(k = 0; k < 8; k++){
				//do the bit mask
				int byte = buffer & byte_mask;
				if(byte == 0){ //0 means free 1 means allocated
					//for each free block, produce a new-line terminated line, with two comma-separated fields (with no white space)
					int num_free_block = i * super->s_blocks_per_group + j * 8 + k + 1;
					printf("BFREE,%d\n", num_free_block);			
				}
				//update
				byte_mask = byte_mask << 1;
			}
		}
	}
	return;
}

void get_free_inodes(){
	int i, j, k;
	for(i = 0; i < group_num; i++){
		int offset = group[i]->bg_inode_bitmap * block_size;
		for(j = 0; j < block_size; j++){
			//scan the free I-node bitmap for each group
			int buffer;
			if(pread(image_fd, &buffer, 1, offset + j) == -1){
				fprintf(stderr, "ERROR in pread() of free inode byte\n");
				exit(2);
			} 
			int byte_mask = 1;
			for(k = 0; k < 8; k++){
				//do the bit mask
				int byte = buffer & byte_mask;
				if(byte == 0){ //0 means free 1 means allocated
					//for each free I-node, produce a new-line terminated line, with two comma-separated fields (with no white space)
					int num_free_inode = i * super->s_inodes_per_group + j * 8 + k + 1;
					printf("IFREE,%d\n", num_free_inode);			
				}
				//update
				byte_mask = byte_mask << 1;
			}
		}
	}
	return;
}

void get_inode(){
	int i, j, k;
	//ext2_inode is declared in header file "ext2_fs.h"
	inode = malloc(sizeof(struct ext2_inode));
	if(inode == NULL){
		fprintf(stderr, "ERROR in malloc ext2_inode\n");
		exit(2);
	}
	int inode_size = 128;

	//do this for the future use of directory entries
	dir_entries = malloc(sizeof(int*) * super->s_inodes_count);
	for(i = 0; i < super->s_inodes_count; i++){
		dir_entries[i] = malloc(sizeof(int) * 2);
	}
	entries_num = 0;

	for(i = 0; i < group_num; i++){
		int offset = group[i]->bg_inode_bitmap * block_size;
		for(j = 0; j < block_size; j++){
			int buffer;
			if(pread(image_fd, &buffer, 1, offset + j) == -1){
				fprintf(stderr, "ERROR in pread() of free block byte\n");
				exit(2);
			}
			int byte_mask = 1;
			for(k = 0; k < 8; k++){
				int byte = buffer & byte_mask;
				if(byte != 0 && (j * 8 + k) < super->s_inodes_per_group){ //1 means allocated and also check if it's in bound
					int inode_offset = group[i]->bg_inode_table * block_size + inode_size * (j * 8 + k);
					//scan the I-nodes for each group
					if(pread(image_fd, inode, inode_size, inode_offset) == -1){
						fprintf(stderr, "ERROR in pread() of inode\n");
						exit(2);
					}

					//check if mode is zero. if so, skip
					__u16 mode;
					if(inode->i_mode == 0){
						continue;
					} else{
						//throw away except the last 3 digits by using bit mask
						mode = inode->i_mode & 0777;
					}

					//check if link count is zero. if so, skip
					if(inode->i_links_count == 0){
						continue;
					}

					//inode number
					int num_inode = i * super->s_inodes_per_group + j * 8 + k + 1;

					//file type
					char file_type;
					if(inode->i_mode & 0x8000){ //file
						file_type = 'f';
					} else if(inode->i_mode & 0x4000){ //directory
						file_type = 'd';
						dir_entries[entries_num][0] = inode_offset;
						dir_entries[entries_num][1] = num_inode;
						entries_num++;
					} else if(inode->i_mode & 0xA000){ //symbolic link
						file_type = 's';
					} else{ //anything else
						file_type = '?';
					}

					//creation time
					char create_time[18];
					time_t t = inode->i_ctime;
					struct tm *lt = gmtime(&t); //gmtime instead of localtime
					if(lt == NULL){
						fprintf(stderr, "ERROR in creation gmtime\n");
						exit(2);
					}
					if(strftime(create_time, sizeof(create_time), "%m/%d/%g %H:%M:%S", lt) == 0) {
						fprintf(stderr, "ERROR in creation strftime\n");
						exit(2);
					}

					//modification time
					char modify_time[18];
					t = inode->i_mtime;
					lt = gmtime(&t);
					if(lt == NULL){
						fprintf(stderr, "ERROR in modification gmtime\n");
						exit(2);
					}
					if(strftime(modify_time, sizeof(modify_time), "%m/%d/%g %H:%M:%S", lt) == 0) {
						fprintf(stderr, "ERROR in modification strftime\n");
						exit(2);
					}

					//time of last access
					char last_access_time[18];
					t = inode->i_atime;
					lt = gmtime(&t);
					if(lt == NULL){
						fprintf(stderr, "ERROR in last access gmtime\n");
						exit(2);
					}
					if(strftime(last_access_time, sizeof(last_access_time), "%m/%d/%g %H:%M:%S", lt) == 0) {
						fprintf(stderr, "ERROR in last access strftime\n");
						exit(2);
					}
					
					//for each valid (non-zero mode and non-zero link count) I-node, produce a new-line terminated line, with 27 comma-separated fields (with no white space)
					printf("INODE,%d,%c,%o,%d,%d,%d,%s,%s,%s,%d,%d", num_inode, file_type, 
						mode, inode->i_uid, inode->i_gid, inode->i_links_count, 
						create_time, modify_time, last_access_time, 
						inode->i_size, inode->i_blocks);				     
					
					int l;
					//do the next fifteen fields are block addresses 
					//(decimal, 12 direct, one indirect, one double indirect, one tripple indirect)
					for(l = 0; l < 15; l++){
						printf(",%d", inode->i_block[l]);
					}
					printf("\n");

					//get indirect blocks for each inode****
					get_indirect_blocks(num_inode);
				}
				//update
				byte_mask = byte_mask << 1;
			}
		}
	}
	return;
}

void get_directory(){
	//ext2_dir_entry is declared in header file "ext2_fs.h"
	directory = malloc(sizeof(struct ext2_dir_entry));
	if(directory == NULL){
		fprintf(stderr, "ERROR in malloc ext2_dir_entry\n");
		exit(2);
	}
	int directory_size = sizeof(struct ext2_dir_entry);

	int i;
	for(i = 0; i < entries_num; i++){
		//find each directory I-node by using saved offset
		if(pread(image_fd, inode, 128, dir_entries[i][0]) == -1){
			fprintf(stderr, "ERROR in pread() of inode in director entries\n");
			exit(2);
		}
		int l;
		for(l = 0; l < 12; l++){
			if(inode->i_block[l] != 0){
				int offset = inode->i_block[l] * block_size;
				int byte_offset = 0;
				while(byte_offset < block_size){ //check the bound
					//scan every data block 
					if(pread(image_fd, directory, directory_size, offset + byte_offset) == -1){
						fprintf(stderr, "ERROR in pread() of directory\n");
						exit(2);
					}

					//check if I-node number is non-zero. if so, skip
					if(directory->inode == 0){
						byte_offset += directory->rec_len;
						continue;
					}
					
					//for each valid (non-zero I-node number) directory entry, produce a new-line terminated line, with seven comma-separated fields (no white space)
					printf("DIRENT,%d,%d,%d,%d,%d,'%s'\n", dir_entries[i][1], byte_offset, 
						directory->inode, directory->rec_len, directory->name_len, 
						directory->name);
					
					//update for each loop
					byte_offset += directory->rec_len;
				}
			}
		}
	}
	return;
}


void get_indirect_blocks(int inodeNum){
	//basic inode info
	int inode_size = 128;
	int tableBlockNum = group[0]->bg_inode_table;  //block ID of first block in inode table
	int inodeTableOffset = tableBlockNum * block_size; //byte offset of inode 1
	int inodeOffset = inodeTableOffset + ((inodeNum - 1) * inode_size); //byte offset of inode we're looking at

	//obtain inode structure
	curr_inode = malloc(sizeof(struct ext2_inode));
	if(curr_inode == NULL){
	    fprintf(stderr, "ERROR in malloc ext2_inode\n");
	    exit(2);
	}
	if (pread(image_fd, curr_inode, inode_size, inodeOffset) == -1){
	    fprintf(stderr, "Error in pread() of inode %d\n", inodeNum);
	    exit(2);
	}

	//make sure all direct pointers are not zero
	int p;
	for (p = 0; p < 12; p++){
	    if (curr_inode->i_block[p] == 0){
			return;
	    }
	}

  	/*--------------------------*/
  	/* SINGLE-INDIRECT BLOCK */
  	/*--------------------------*/

  	int s_blockNum = curr_inode->i_block[12]; //get block number for singly-indirect block
  	if (s_blockNum == 0){
    	return;
  	}
  	int s_byte_offset = s_blockNum * block_size; //get byte offset of current block
  	s_indirect_block = malloc(block_size); //pointer to first block ID in indirect array
  	if (pread(image_fd, s_indirect_block, block_size, s_byte_offset) == -1){
    	fprintf(stderr, "Error in pread() of an indirect block");
    	exit(2);
  	}
  	int levelOfIndirection = 1;
  	int blockOffset = 12;
  
  	int nEntries;
  	nEntries = block_size / sizeof(int);
  	int i;
  	for (i = 0; i < nEntries; i++){
    	if (s_indirect_block[i] == 0){
      		continue;
    	}
    	//print info
    	printf("INDIRECT,%d,%d,%d,%d,%d\n", inodeNum, levelOfIndirection, blockOffset, s_blockNum, s_indirect_block[i]);
    	blockOffset++; //increment block count for logical offset
  	}
  
  	/*------------------------*/
  	/* DOUBLE-INDIRECT BLOCK */
  	/*------------------------*/

  	int d_blockNum = curr_inode->i_block[13]; //get block number for doubly-indirect block
  	if (d_blockNum == 0){ 
    	return;
    }
  	int d_byte_offset = d_blockNum * block_size; //get byte offset of current block
  	d_indirect_block = malloc(block_size); //pointer to first block ID in indirect array
  	if (pread(image_fd, d_indirect_block, block_size, d_byte_offset) == -1){
    	fprintf(stderr, "Error in pread() of an indirect block");
    	exit(2);
  	}

  	for (i = 0; i < nEntries; i++){
    	if (d_indirect_block[i] == 0){
      		continue;
    	}
    
    	//recursively get singles
    	levelOfIndirection = 1;
    	s_byte_offset = d_indirect_block[i] * block_size;
    	if (pread(image_fd, s_indirect_block, block_size, s_byte_offset) == -1){
      		fprintf(stderr, "Error in pread() of an indirect block");
      		exit(2);
    	}
    	int j;
    	for (j = 0; j < nEntries; j++){
			if (s_indirect_block[j] == 0){
	  			continue;
			}
			//print info
			printf("INDIRECT,%d,%d,%d,%d,%d\n", inodeNum, levelOfIndirection, blockOffset, d_indirect_block[i], s_indirect_block[j]);
			blockOffset++; //increment block count for logical offset
    	}

    	levelOfIndirection = 2;
    	//print info
    	printf("INDIRECT,%d,%d,%d,%d,%d\n", inodeNum, levelOfIndirection, blockOffset, d_blockNum, d_indirect_block[i]);
  	}

  	/*-----------------------*/
  	/* TRIPLE-INDIRECT BLOCK */
  	/*-----------------------*/

  	int t_blockNum = curr_inode->i_block[14]; //get block number for triply-indirect block
  	if (t_blockNum == 0){
    	return;
  	}
  	int t_byte_offset = t_blockNum * block_size; //get byte offset of current block
  	t_indirect_block = malloc(block_size); //pointer to first block ID in indirect array
  	if (pread(image_fd, t_indirect_block, block_size, t_byte_offset) == -1){
    	fprintf(stderr, "Error in pread() of an indirect block");
    	exit(2);
  	}	

  	for (i = 0; i < nEntries; i++){
    	if (t_indirect_block[i] == 0){
      		continue;
    	}
    
    	//recursively get doubles
    	d_byte_offset = t_indirect_block[i] * block_size;
    	if (pread(image_fd, d_indirect_block, block_size, d_byte_offset) == -1){
      		fprintf(stderr, "Error in pread() of an indirect block");
      		exit(2);
    	}
    	int j;
    	for (j = 0; j < nEntries; j++){
			if (d_indirect_block[j] == 0){
	  			continue;
			}
	
			//recursively get singles
			levelOfIndirection = 1;
			s_byte_offset = d_indirect_block[i] * block_size;
			if (pread(image_fd, s_indirect_block, block_size, s_byte_offset) == -1){
			  fprintf(stderr, "Error in pread() of an indirect block");
			  exit(2);
			}
			int k;
			for (k = 0; k < nEntries; k++){
			    if (s_indirect_block[k] == 0){
			    	continue;
			    }
			    
			    //print info for level 1
			    printf("INDIRECT,%d,%d,%d,%d,%d\n", inodeNum, levelOfIndirection, blockOffset, d_indirect_block[j], s_indirect_block[k]);
			    blockOffset++; //increment block count for logical offset
			}
	
			levelOfIndirection = 2;
			//print info for level 2
			printf("INDIRECT,%d,%d,%d,%d,%d\n", inodeNum, levelOfIndirection, blockOffset, t_indirect_block[i], d_indirect_block[j]);
			blockOffset++; //increment block count for logical offset
		}
    
	    levelOfIndirection = 3;
	    //print info for level 3
	    printf("INDIRECT,%d,%d,%d,%d,%d\n", inodeNum, levelOfIndirection, blockOffset, t_blockNum, t_indirect_block[i]);
	}
	return;
}

void free_all(){
	int i;
	int inodes_num = super->s_inodes_count;
	for(i = 0; i < inodes_num; i++){
		free(dir_entries[i]);
	}
	free(dir_entries);
	
	free(super);
	free(inode);
	free(directory);

  	for(i = 0; i < group_num; i++){
  		free(group[i]);
  	}
 	free(group);

	free(curr_inode);
 	free(s_indirect_block);
 	free(d_indirect_block);
 	free(t_indirect_block);
	
 	return;
}

int main(int argc, char **argv){
	//check if argument is correct and save it as disk image name
	char *disk_image_name;
	if(argc != 2){
		fprintf(stderr, "ERROR: Only one argument of a disk image name is acceptable\n");
		exit(1);
	} else{
		disk_image_name = malloc(sizeof(char) * (strlen(argv[1]) + 1));
		disk_image_name = argv[1];
	}

	//open image fd
	image_fd = open(disk_image_name, O_RDONLY);
	if(image_fd == -1){
		fprintf(stderr, "ERROR in open() image file\n");
		exit(2);
	}

	//superblock summary
	get_super_block();

	//group summary
	get_group();

	//free block entries
	get_free_block();

	//free I-node entries
	get_free_inodes();

	//I-node summary
	get_inode();

	//directory entries
	get_directory();

	//indirect block references
	//function is called within get_inode()

	//free all allocated memories
	free_all();
	return 0;
}
