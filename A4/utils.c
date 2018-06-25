#include "utils.h"

/******************************************************************************
	Map disk image to memory space
******************************************************************************/
void init_FS(char *image_name){
	int fd = open(image_name, O_RDWR);

	disk = mmap(NULL, 128 * 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if(disk == MAP_FAILED) {
	    perror("mmap");
	    exit(1);
	}
}

/******************************************************************************
	Get the address of super block
******************************************************************************/
super_block *get_super_block(){
	return (super_block *) get_block(1);
}

/******************************************************************************
	Get the address of block where block group descriptor is stored
******************************************************************************/
group_desc *get_group_desc(){
	return (group_desc *) get_block(2);
}

/******************************************************************************
	Get the address of block where block bitmap is stored
******************************************************************************/
char *get_block_bitmap(){
	return get_block(3);
}

/******************************************************************************
	Get the address of block where inode bitmap is stored
******************************************************************************/
char *get_inode_bitmap(){
	return get_block(4);
}

/******************************************************************************
	Get the address of the block where inode table is stored
******************************************************************************/
char *get_inode_table(){
	return get_block(5);
}

/******************************************************************************
	Get the address of the block by the block number
	Note: block_no is the index starting from 0
******************************************************************************/
char *get_block(int block_no){
	char *block = (char *)(disk + block_no * EXT2_BLOCK_SIZE);
	return block;
}

/******************************************************************************
	Get the address of the inode by the inode number
	Note: inode_index is the index starting from 0 (it is equal to inode_no -1)
******************************************************************************/
char *get_inode(int inode_no){
	char *inode_table = get_inode_table();
	char *ret_inode = inode_table + (sizeof(inode) * (inode_no - 1));
	return ret_inode;
}

/******************************************************************************
	Check if the absolute path start with root (/) or if it is just root (/)
******************************************************************************/
void check_root_dir(char *absolute_path, char *mode){
	char*  valid_root_dir = "/";
	if(strncmp(absolute_path, valid_root_dir, 1)){
		fprintf(stderr, "%s: invalid absolute path\n", mode);
		exit(ENOENT);
	}
}

/******************************************************************************
	Given string of absolute path, parse them into seperate path tokens, then
		return array of all the parsed path tokens (Remember to free the token
		to avoid memory leak)
******************************************************************************/
char **parse_absolute_path(char *absolute_path, int *depth_counter, char *mode){

	check_root_dir(absolute_path, mode);

	if(strlen(absolute_path)==1){
		return NULL;
	}

	char *search = "/";
	char *temp_tokens[MAX_DIR_DEPTH];

	char *token = malloc(sizeof(char) * EXT2_NAME_LEN);
	if(token == NULL){
		perror("malloc");
		exit(1);
	}
	char *temp_token = strtok(absolute_path, search);
	strncpy(token, temp_token, EXT2_NAME_LEN);
	
	while(1){
		temp_tokens[*depth_counter] = token;
		temp_token = strtok(NULL, search);
		(*depth_counter)++;
		if(temp_token == NULL) break;
		token = malloc(sizeof(char) * EXT2_NAME_LEN);
		if(token == NULL){
			perror("malloc");
			exit(1);
		}
		strncpy(token, temp_token, EXT2_NAME_LEN);
	}
	
	char **tokens = malloc(sizeof(char *) * (*depth_counter));
	if(tokens == NULL){
			perror("malloc");
			exit(1);
	}
	memcpy(tokens, temp_tokens, (*depth_counter) * sizeof(*tokens));

	return tokens; 
}

/******************************************************************************
	Set cur_dir_entry to be the one before the dir_entry matching entity name 
		and file_type. Return
	FIRST_ENTRY (1) 			if it is the first dir_entry in that block
	NOT_FIRST_ENTRY (0) 		if it is not the first_dir_entry in that block
	CHILD_NOT_FOUND (-1) 	if entry is not found
	CHILD_NOT_MATCH (-2) 	if entry is found but file_type does not match
******************************************************************************/
int get_dir_entry_prev(dir_entry **cur_dir_entry, char *entity_name, \
	inode *parent_dir_inode, unsigned char file_type, int *block_idx){
	int rec_len_sum, prev_rec_len;
	int max_block_index = parent_dir_inode->i_blocks/2;
	for(*block_idx=0;*block_idx<max_block_index;(*block_idx)++){
		*cur_dir_entry = (dir_entry *) get_block(parent_dir_inode->i_block[*block_idx]);
		rec_len_sum = 0;
		while(rec_len_sum < EXT2_BLOCK_SIZE){
			if(!strncmp(entity_name, (*cur_dir_entry)->name, (*cur_dir_entry)->\
				name_len) && strlen(entity_name) == (*cur_dir_entry)->name_len){
				if((*cur_dir_entry)->file_type == file_type){
					if(rec_len_sum){
						*cur_dir_entry = (void *) (*cur_dir_entry) - prev_rec_len;
						return NOT_FIRST_ENTRY;
					}else{
						return FIRST_ENTRY;
					}
				}else{
					return CHILD_NOT_MATCH;
				}
			}
			prev_rec_len = (*cur_dir_entry)->rec_len;
			rec_len_sum += prev_rec_len;
			*cur_dir_entry = (void *) (*cur_dir_entry) + prev_rec_len;
		}
	}
	return CHILD_NOT_FOUND;
}

/******************************************************************************
	Set cur_dir_entry_4gap to be one which has been removed matching entity
	 	name and file_type. Set cur_dir_entry to be the valid directory entry one
	 	before the cur_dir_entry_4gap. Return
	FIRST_ENTRY (1) 			if it is the first dir_entry in that block
	NOT_FIRST_ENTRY (0) 		if it is not the first_dir_entry in that block
	CHILD_NOT_FOUND (-1) 	if entry is not found
	CHILD_NOT_MATCH (-2) 	if entry is found but file_type does not match
******************************************************************************/
int get_dir_entry_prev_4restore(dir_entry **cur_dir_entry, \
	dir_entry **cur_dir_entry_4gap, char *entity_name, \
	inode *parent_dir_inode, unsigned char file_type){
	int rec_len_sum, prev_rec_len, rec_len_sum_4gap, prev_rec_len_4gap, i;
	int max_block_index = parent_dir_inode->i_blocks/2;
	for(i=0;i<max_block_index;i++){
		*cur_dir_entry = (dir_entry *) get_block(parent_dir_inode->i_block[i]);
		rec_len_sum = 0;
		while(rec_len_sum < EXT2_BLOCK_SIZE){
			rec_len_sum_4gap = 0;
			*cur_dir_entry_4gap = (void *) (*cur_dir_entry) + rec_len_sum_4gap;
			while(rec_len_sum_4gap < (*cur_dir_entry)->rec_len){
				if(!strncmp(entity_name, (*cur_dir_entry_4gap)->name, (*cur_dir_entry_4gap)->\
					name_len) && strlen(entity_name) == (*cur_dir_entry_4gap)->name_len){
					if((*cur_dir_entry_4gap)->file_type == file_type){
						if(rec_len_sum_4gap){
							return RESTORABLE;
						}else{
							return NOT_RESTORABLE;
						}
					}else{
						return CHILD_NOT_MATCH;
					}
				}
				prev_rec_len_4gap = (*cur_dir_entry_4gap)->name_len + DIR_ENTRY_LEN_EX_NAME;
				if(prev_rec_len_4gap%4) prev_rec_len_4gap += (4-prev_rec_len_4gap%4);
				rec_len_sum_4gap += prev_rec_len_4gap;
				*cur_dir_entry_4gap = (void *) (*cur_dir_entry) + rec_len_sum_4gap;
			}
			prev_rec_len = (*cur_dir_entry)->rec_len;
			rec_len_sum += prev_rec_len;
			*cur_dir_entry = (void *) (*cur_dir_entry) + prev_rec_len;
		}
	}
	return CHILD_NOT_FOUND;
}

/******************************************************************************
	Get child inode number if name and file type are matched given parent directory,
		otherwise return NULL
******************************************************************************/
int get_child_inode_no(char *entity_name, inode *parent_dir_inode, \
	unsigned char file_type){
	int block_idx;
	dir_entry *cur_dir_entry;
	int result = get_dir_entry_prev(&cur_dir_entry, entity_name, parent_dir_inode, file_type, &block_idx);
	if(result == FIRST_ENTRY || result ==  NOT_FIRST_ENTRY){
		cur_dir_entry = (void *) cur_dir_entry + cur_dir_entry->rec_len;
		return cur_dir_entry->inode;
	}
	return result;
}

/******************************************************************************
	Get child directory inode number  if name and file type are matched given 
		parent directory, otherwise return NULL
******************************************************************************/
int get_child_dir_inode_no(char *dir_name, inode *cur_inode, char *mode){
	int child_dir_inode_no = get_child_inode_no(dir_name, cur_inode, EXT2_FT_DIR);
	if(child_dir_inode_no == CHILD_NOT_FOUND){
		fprintf(stderr, "%s: %s: no such directory\n", mode, dir_name);
		exit(ENOENT);
	}else if(child_dir_inode_no == CHILD_NOT_MATCH){
		fprintf(stderr, "%s: %s: the path does not refer to a directory\n", \
			mode, dir_name);
		exit(ENOENT);
	}
	return child_dir_inode_no;
}

/******************************************************************************
	Get child file inode number if name and file type are matched given parent 
		directory, otherwise return NULL
******************************************************************************/
int get_child_file_inode_no(char *dir_name, inode *cur_inode, char *mode){
	int child_file_inode_no = get_child_inode_no(dir_name, cur_inode, EXT2_FT_REG_FILE);
	if(child_file_inode_no == CHILD_NOT_FOUND){
		fprintf(stderr, "%s: %s: no such file\n", mode, \
			dir_name);
		exit(ENOENT);
	}else if(child_file_inode_no == CHILD_NOT_MATCH){
		fprintf(stderr, "%s: %s: the path does not refer to a file\n", mode, dir_name);
		exit(ENOENT);
	}
	return child_file_inode_no;
}

/******************************************************************************
	Get child link inode number if name and file type are matched given parent 
		directory, otherwise return NULL
******************************************************************************/
int get_child_link_inode_no(char *dir_name, inode *cur_inode, char *mode){
	int child_link_inode_no = get_child_inode_no(dir_name, cur_inode, EXT2_FT_SYMLINK);
	if(child_link_inode_no == CHILD_NOT_FOUND){
		fprintf(stderr, "%s: %s: the path does not refer to a link\n", mode, \
			dir_name);
		exit(EISDIR);
	}else if(child_link_inode_no == CHILD_NOT_MATCH){
		fprintf(stderr, "%s: %s: no such link\n", mode, dir_name);
		exit(ENOENT);
	}
	return child_link_inode_no;
}

/******************************************************************************
	Get last block of the specified directory inode
******************************************************************************/
char *get_last_block(inode *dir_inode){
	int max_block_index = dir_inode->i_blocks/2;
	int last_block_no = dir_inode->i_block[max_block_index-1];
	char *last_block = get_block(last_block_no);
	return last_block;
}

/******************************************************************************
	Add directory entry to the dir_entry right after cur_dir_entry if space is 
		available
******************************************************************************/
void add_dir_entry(dir_entry *cur_dir_entry, int inode_no, int space_left, \
	char *target_dir_name, unsigned char file_type){
	if(cur_dir_entry){
		init_dir_entry(cur_dir_entry, inode_no, space_left, target_dir_name, \
			file_type);
	}else{
		int avail_block_no;
		cur_dir_entry = (dir_entry *) allocate_new_block(&avail_block_no);
		init_dir_entry(cur_dir_entry, inode_no, EXT2_BLOCK_SIZE,\
			target_dir_name, file_type);
		update_inode(inode_no, avail_block_no);
		update_block_bitmap(avail_block_no, ADD_MODE);
	}
}

/******************************************************************************
	Add directory entry to the dir_entry in the last block (file_type:directory)
******************************************************************************/
void add_dir_entry_4dir(int parent_inode_no, char *last_block, \
	char *target_dir_name){
	int avail_inode_no;
	inode *cur_inode = (inode *) allocate_new_inode(&avail_inode_no);
	init_inode(cur_inode, EXT2_S_IFDIR, 1);

	int avail_block_no;
	char *new_block = allocate_new_block(&avail_block_no);
	init_dir_entry((dir_entry *) new_block, avail_inode_no, 12, ".", EXT2_FT_DIR);
	new_block = new_block + 12;
	init_dir_entry((dir_entry *) new_block, parent_inode_no, 1012, "..", EXT2_FT_DIR);
	cur_inode->i_block[0] = avail_block_no;
	update_block_bitmap(avail_block_no, ADD_MODE);

	int space_left;
	dir_entry *cur_dir_entry =  check_if_no_space(last_block, target_dir_name, \
		&space_left);

	add_dir_entry(cur_dir_entry, avail_inode_no, space_left, \
		target_dir_name, EXT2_FT_DIR);

	update_inode_bitmap(avail_inode_no, ADD_MODE, DIR);
}

/******************************************************************************
	Add directory entry to the dir_entry in the last block (file_type:file)
******************************************************************************/
void add_dir_entry_4file(char *last_block, char *target_dir_name, int fd){
	struct stat buf;
	fstat(fd, &buf);
	int file_size = buf.st_size;
	int block_amount = file_size/EXT2_BLOCK_SIZE;
	if(file_size%EXT2_BLOCK_SIZE) block_amount += 1;
	if(block_amount>12) block_amount++;

	int avail_inode_no;
	inode *cur_inode = (inode *) allocate_new_inode(&avail_inode_no);
	init_inode(cur_inode, EXT2_S_IFREG, block_amount);

	int free_blocks[block_amount];
	int if_avail = get_avail_blocks(block_amount, free_blocks);
	if(if_avail == NOT_AVAILABLE){
		fprintf(stderr, "Block full\n");
		exit(ENOSPC);
	}

	int i;
	unsigned char *source_file = mmap(NULL, file_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
	if(source_file == MAP_FAILED) {
	    perror("mmap");
	    exit(1);
	}

	int direct_blocks = min(block_amount,12);
	int indirect_blocks = max(block_amount - direct_blocks - 1, 0);

	int len_left = file_size;
	for(i=0;i<direct_blocks;i++, source_file+=EXT2_BLOCK_SIZE, \
		len_left-=EXT2_BLOCK_SIZE){
		if(len_left <= EXT2_BLOCK_SIZE)
			memcpy(get_block(free_blocks[i]), source_file, len_left);
		else
			memcpy(get_block(free_blocks[i]), source_file, EXT2_BLOCK_SIZE);
		update_block_bitmap(free_blocks[i], ADD_MODE);
		cur_inode->i_block[i] = free_blocks[i];
	}
	if(indirect_blocks){
		unsigned int *indirect_block = (unsigned int *) get_block(free_blocks[12]);
		for(i = 0; i < indirect_blocks; i++, source_file+=EXT2_BLOCK_SIZE, \
			len_left-=EXT2_BLOCK_SIZE){
			if(len_left <= EXT2_BLOCK_SIZE)
				memcpy(get_block(free_blocks[i+13]), source_file, len_left);
			else
				memcpy(get_block(free_blocks[i+13]), source_file, EXT2_BLOCK_SIZE);
			update_block_bitmap(free_blocks[i+13], ADD_MODE);
			indirect_block[i] = free_blocks[i+13];
		}
		cur_inode->i_block[12] = free_blocks[12];
		update_block_bitmap(free_blocks[12], ADD_MODE);
	}
	int space_left ;
	dir_entry *cur_dir_entry =  check_if_no_space(last_block, target_dir_name, \
		&space_left);

	add_dir_entry(cur_dir_entry, avail_inode_no, space_left, \
		target_dir_name, EXT2_FT_REG_FILE);

	update_inode_bitmap(avail_inode_no, ADD_MODE, NOT_DIR);
}

/******************************************************************************
	Add directory entry to the dir_entry in the last block (file_type:link)
******************************************************************************/
void add_dir_entry_4link(int inode_no, char *last_block, \
	char *target_dir_name, int symlink_true){
	if(symlink_true){
		inode *cur_inode = (inode *) allocate_new_inode(&inode_no);
		init_inode(cur_inode, EXT2_S_IFLNK, 1);
		int avail_block_no;
		char *new_block = allocate_new_block(&avail_block_no);
		strcpy(new_block, symlink_path);
		cur_inode->i_block[0] = avail_block_no;
		update_block_bitmap(avail_block_no, ADD_MODE);
	}
	int space_left;
	dir_entry *cur_dir_entry =  check_if_no_space(last_block, target_dir_name, \
		&space_left);
	if(symlink_true){
		add_dir_entry(cur_dir_entry, inode_no, space_left, \
			target_dir_name, EXT2_FT_SYMLINK);
		update_inode_bitmap(inode_no, ADD_MODE, NOT_DIR);
	}else{
		add_dir_entry(cur_dir_entry, inode_no, space_left, \
			target_dir_name, EXT2_FT_REG_FILE);
	}
}

/******************************************************************************
	Initialize inode to set the corresponding properties
******************************************************************************/
void init_inode(inode *cur_inode, unsigned short file_mode, int block_amount){
	memset(cur_inode, 0, sizeof(inode));
	cur_inode->i_mode = file_mode;
	cur_inode->i_size = EXT2_BLOCK_SIZE * block_amount;
	cur_inode->i_ctime = time(NULL);
	cur_inode->i_blocks = 2 * block_amount;
}

/******************************************************************************
	Check if the last block has run out of space, if yes, NULL, else, return the 
		last entry in that block.
******************************************************************************/
dir_entry *check_if_no_space(char *last_block, char *target_dir_name, \
	int *space_left){
	dir_entry *cur_dir_entry = (dir_entry *) last_block;
	int rec_len_sum = 0;
	while(1){
		rec_len_sum += cur_dir_entry->rec_len;
		if(rec_len_sum == EXT2_BLOCK_SIZE) break;
		cur_dir_entry = (void *) cur_dir_entry + cur_dir_entry->rec_len;
	}
	int cur_dir_entry_len = DIR_ENTRY_LEN_EX_NAME + cur_dir_entry->name_len;
	// align to 4B
	if((cur_dir_entry_len%4)) cur_dir_entry_len+=(4-cur_dir_entry_len%4);
	*space_left = cur_dir_entry->rec_len - cur_dir_entry_len;
	if((*space_left) < DIR_ENTRY_LEN_EX_NAME + strlen(target_dir_name))
		return NULL;
	else{
		cur_dir_entry->rec_len = cur_dir_entry_len;
		cur_dir_entry = (void *) cur_dir_entry + cur_dir_entry_len;
		return cur_dir_entry;
	}
}

/******************************************************************************
	Allocate block space and store available block number in avail_block_no,
		then return the address of the block
******************************************************************************/
char *allocate_new_block(int *avail_block_no){
	*avail_block_no = get_avail_block_no();
	char *avail_block = get_block(*avail_block_no);
	return avail_block;
}

/******************************************************************************
	Allocate inode space and store available inode number in avail_inode_no,
		then return the address of the inode
******************************************************************************/
char *allocate_new_inode(int *avail_inode_no){
	*avail_inode_no = get_avail_inode_no();
	char *avail_inode = get_inode(*avail_inode_no);
	return avail_inode;
}

/******************************************************************************
	Initialize directory entry given rec_len and dir_name
******************************************************************************/
void init_dir_entry(dir_entry *cur_dir_entry, int inode_no, int rec_len, \
	char *dir_name, unsigned char file_type){
	cur_dir_entry->inode = inode_no;
	if(file_type == EXT2_FT_DIR){
		update_links_count(inode_no, ADD_MODE, DIR);
	}else{
		update_links_count(inode_no, ADD_MODE, NOT_DIR);
	}
	cur_dir_entry->rec_len = rec_len;
	cur_dir_entry->name_len = strlen(dir_name);
	cur_dir_entry->file_type = file_type;
	// In order to make the string consitant to the name it is being assigned, 
	// cur_dir_entry->namecan be null terminated or not null terminated depend
	// on the strlen(dir_name)
	strncpy(cur_dir_entry->name, dir_name, strlen(dir_name));
}


/******************************************************************************
	Update Super Block
******************************************************************************/
void update_super_block(int inodes_usage, int blocks_usage){
	super_block *sb = get_super_block();
	sb->s_free_blocks_count -= blocks_usage;
	sb->s_free_inodes_count -= inodes_usage;
}

/******************************************************************************
	Update Group Descriptor
******************************************************************************/
void update_group_desc(int inodes_usage, int blocks_usage, int dirs_usage){
	group_desc *gd = get_group_desc();
	gd->bg_free_blocks_count -= blocks_usage;
	gd->bg_free_inodes_count -=inodes_usage;
	gd->bg_used_dirs_count += dirs_usage;
}


/******************************************************************************
	Update INODE bitmap if INODE has been allocated
******************************************************************************/
void update_inode_bitmap(int inode_no, int mode, int if_dir){
	char *bitmap = get_inode_bitmap();
	int idx = (inode_no-1)/8;
	int offset = (inode_no-1)%8;
	char mask;
	if(mode == ADD_MODE){
		mask = 0x01 << offset;
		bitmap[idx] |= mask;
		if(if_dir == DIR){
			update_group_desc(1, 0, 1);
			update_super_block(1, 0);
		}else{
			update_group_desc(1, 0, 0);
			update_super_block(1, 0);
		}
	}else{
		mask = ~(0x80 >> (7 - offset));
		bitmap[idx] &= mask;
		if(if_dir == DIR){
			update_group_desc(-1, 0, -1);
			update_super_block(-1, 0);
		}else{
			update_group_desc(-1, 0, 0);
			update_super_block(-1, 0);
		}
	}
}

/******************************************************************************
	Update BLOCK bitmap BLOCK has been allocated
******************************************************************************/
void update_block_bitmap(int block_no, int mode){
	char *bitmap = get_block_bitmap();
	int idx = (block_no-1)/8;
	int offset = (block_no-1)%8;
	char mask;
	if(mode == ADD_MODE){
		mask = 0x01 << offset;
		bitmap[idx] |= mask;
		update_group_desc(0, 1, 0);
		update_super_block(0, 1);
	}else{
		mask = ~(0x80 >> (7 - offset));
		bitmap[idx] &= mask;
		update_group_desc(0, -1, 0);
		update_super_block(0, -1);
	}
}

/******************************************************************************
	Update links count for a inode specified by inode_no
******************************************************************************/
void update_links_count(int inode_no, int mode, int if_dir){
	inode *cur_inode = (inode *) get_inode(inode_no);
	if(mode == ADD_MODE){
		cur_inode->i_links_count++;
	}else{
		cur_inode->i_links_count--;
		if(cur_inode->i_links_count == 0){
			update_inode_bitmap(inode_no, SUB_MODE, if_dir);
			int total_blocks = cur_inode->i_blocks/2;
			int direct_blocks = min(total_blocks, 12);
			int indirect_blocks = max(total_blocks - direct_blocks - 1, 0);
			int i;
			for(i = 0; i < direct_blocks; i++){ 
				update_block_bitmap(cur_inode->i_block[i], SUB_MODE);
			}
			if(indirect_blocks){
				unsigned int *indirect_block = (unsigned int *) get_block(cur_inode->i_block[12]);
				for(i = 0; i < indirect_blocks; i++){
					update_block_bitmap(indirect_block[i], SUB_MODE);
				}
				update_block_bitmap(cur_inode->i_block[12], SUB_MODE);
			}
		}
	}
}

/******************************************************************************
	Update inode filds given parent_inode_no
******************************************************************************/
void update_inode(int inode_no, int block_no){
	inode *cur_inode = (inode *) get_inode(inode_no);
	cur_inode->i_size += EXT2_BLOCK_SIZE;
	cur_inode->i_blocks += EXT2_BLOCK_SIZE/DISK_SECTOR_SIZE;
	int last_block_index = \
		cur_inode->i_blocks/(EXT2_BLOCK_SIZE/DISK_SECTOR_SIZE) - 1;
	cur_inode->i_block[last_block_index] = block_no;
}

/******************************************************************************
	Shift the block index in the inode to the left by 1 
******************************************************************************/
void inode_blocks_shift(inode *cur_inode, int block_idx){
	int max_block_index = cur_inode->i_blocks/2 - 1;
	for(;block_idx<max_block_index;block_idx++){
		cur_inode->i_block[block_idx] = cur_inode->i_block[block_idx+1];
	}
	cur_inode->i_blocks = max_block_index;
}

/******************************************************************************
	Find available block by traversing through block bitmap and return block 
		number which is found
******************************************************************************/
int get_avail_block_no(){
	char *block_bitmap = get_block_bitmap();
	int start_idx = PROTECTED_BLOCK_COUNT/8;
	int start_offset = PROTECTED_BLOCK_COUNT%8;
	int block_no = get_avail_no(start_idx, start_offset, block_bitmap, \
		BLOCK_BITMAP_LEN);
	if(block_no == NOT_AVAILABLE){
		fprintf(stderr, "Disk image full\n");
		exit(ENOSPC);
	}
	return block_no;
}

/******************************************************************************
	Find available inode by traversing through inode bitmap and return 
		inode number
******************************************************************************/
int get_avail_inode_no(){
	char *inode_bitmap = get_inode_bitmap();
	int start_idx = PROTECTED_INODE_COUNT/8;
	int start_offset = PROTECTED_INODE_COUNT%8;
	int inode_no =  get_avail_no(start_idx, start_offset, inode_bitmap, \
		INODE_BITMAP_LEN);
	if(inode_no == NOT_AVAILABLE){
		fprintf(stderr, "Inode table full\n");
		exit(ENOSPC);
	}
	return inode_no;
}

/******************************************************************************
	Helper function to get index of bit 0 in bitmap (both BLOCK and INODE)
******************************************************************************/
int get_avail_no(int start_idx, int start_offset, char *bitmap, int bitmap_len){
	int i, bit_index;
	for(i=start_idx;i<bitmap_len;i++){
		bit_index = bit_checker(bitmap[i]);
		if(i == start_idx){
			if(bit_index <= start_offset || bit_index == BIT_NOT_FOUND)
				continue;
			return i * 8 + bit_index;
		}
		if(bit_index == BIT_NOT_FOUND) continue;
		else return i * 8 + bit_index;
	}
	return NOT_AVAILABLE;
}

/******************************************************************************
	Get block numbers (stored in free_block_nos) that are available to be used
	Note: memory size of free_block_nos should match amount 
******************************************************************************/
int get_avail_blocks(int amount, int *free_block_nos){
	char *bitmap = get_block_bitmap();
	int i, j, counter = 0;
	int start_idx = 1;
	for(i=start_idx;i<BLOCK_BITMAP_LEN;i++){
		if(i == start_idx){
			for(j=1;j<8;j++){
				if(!((bitmap[i]>>j) & 0x01)) free_block_nos[counter++] = 8+j+1;
				if(counter == amount) return AVAILABLE;
			}
		}else{
			for(j=0;j<8;j++){
				if(!((bitmap[i]>>j) & 0x01)) free_block_nos[counter++] = i*8+j+1;
				if(counter == amount) return AVAILABLE;
			}	
		}
	}
	return NOT_AVAILABLE;
}

/******************************************************************************
	Check if a bit in a byte is 0, then return the index of this bit
	Note: scan order is from left to right
******************************************************************************/
int bit_checker(char byte){
	int i;
	for(i=0;i<8;i++){
		if(!((byte>>i) & 0x01)) return i+1;
	}
	return BIT_NOT_FOUND;
}

/******************************************************************************
	Given specified number, check if it is taken by checking it on the bitmap
	Return 1 if being taken, else 0
******************************************************************************/
int check_against_inode_bitmap(int inode_no){
	char *inode_bitmap = get_inode_bitmap();
	int start_idx = (inode_no-1)/8;
	int start_offset = (inode_no-1)%8;
	return inode_bitmap[start_idx]>>start_offset & 0x01;
}

/******************************************************************************
	Given specified number, check if it is taken by checking it on the bitmap
	Return 1 if being taken, else 0
******************************************************************************/
int check_against_block_bitmap(int block_no){
	char *block_bitmap = get_block_bitmap();
	int start_idx = (block_no-1)/8;
	int start_offset = (block_no-1)%8;
	return block_bitmap[start_idx]>>start_offset & 0x01;
}

/******************************************************************************
	Restore the directory entry given entry to be restored and valid entry
		right before it
******************************************************************************/
void restore(dir_entry *cur_dir_entry, dir_entry *entry_2b_restored){
	if(check_against_inode_bitmap(entry_2b_restored->inode)){
		fprintf(stderr, "Inode has been taken, unable to restore\n");
		exit(EEXIST);
	}
	int i;
	inode *cur_inode = (inode *) get_inode(entry_2b_restored->inode);
	int total_blocks = cur_inode->i_blocks/2;
	int direct_blocks = min(total_blocks,12);
	int indirect_blocks = max(total_blocks-direct_blocks-1, 0);

	for(i=0;i<direct_blocks;i++){
		if(check_against_block_bitmap(cur_inode->i_block[i])){
			fprintf(stderr, "Block has been taken, unable to restore\n");
			exit(EEXIST);
		}
	}
	if(indirect_blocks){
		unsigned int *indirect_block = (unsigned int *) get_block(cur_inode->i_block[12]);
		if(check_against_block_bitmap(cur_inode->i_block[12])){
				fprintf(stderr, "Block has been taken, unable to restore\n");
				exit(EEXIST);
		}
		for(i = 0; i < indirect_blocks; i++){
			if(check_against_block_bitmap(indirect_block[i])){
				fprintf(stderr, "Block has been taken, unable to restore\n");
				exit(EEXIST);
			}
		}
	}
	update_inode_bitmap(entry_2b_restored->inode, ADD_MODE, NOT_DIR);

	for(i=0;i<direct_blocks;i++){
		update_block_bitmap(cur_inode->i_block[i], ADD_MODE);
	}
	if(indirect_blocks){
		unsigned int *indirect_block = (unsigned int *) get_block(cur_inode->i_block[12]);
		update_block_bitmap(cur_inode->i_block[12], ADD_MODE);
		for(i = 0; i < indirect_blocks; i++){
			update_block_bitmap(indirect_block[i], ADD_MODE);
		}
	}

	cur_inode->i_links_count++;
	int full_rec_len = cur_dir_entry->rec_len;
	cur_dir_entry->rec_len = (void *) entry_2b_restored - (void *) cur_dir_entry;
	entry_2b_restored->rec_len = full_rec_len - cur_dir_entry->rec_len;
}

/******************************************************************************
	Check super block and block group desciptor fields against bitmap
******************************************************************************/
void check_a(int *counter){
	int i, free_inodes = 0, free_blocks = 0;
	for(i=1;i<=MAX_INODE;i++){
		if(!check_against_inode_bitmap(i)){
			free_inodes++;
		}
	}
	for(i=1;i<=MAX_BLOCK;i++){
		if(!check_against_block_bitmap(i)){
			free_blocks++;
		}
	}
	super_block *sb = get_super_block();
	group_desc *gd = get_group_desc();
	int inodes_difference_4sb = abs(free_inodes - sb->s_free_inodes_count);
	int blocks_difference_4sb = abs(free_blocks - sb->s_free_blocks_count);
	int inodes_difference_4gd = abs(free_inodes - gd->bg_free_inodes_count);
	int blocks_difference_4gd = abs(free_blocks - gd->bg_free_blocks_count);
	if(inodes_difference_4sb){
		sb->s_free_inodes_count = free_inodes;
		(*counter) += inodes_difference_4sb;
		printf("Fixed: superblock's free inodes counter was off by %d compared to the bitmap\n",\
			inodes_difference_4sb);
	}
	if(blocks_difference_4sb){
		sb->s_free_blocks_count = free_blocks;
		(*counter) += blocks_difference_4sb;
		printf("Fixed: superblock's free blocks counter was off by %d compared to the bitmap\n",\
			blocks_difference_4sb);
	}
	if(inodes_difference_4gd){
		gd->bg_free_inodes_count = free_inodes;
		(*counter) += inodes_difference_4gd;
		printf("Fixed: block group's free inodes counter was off by %d compared to the bitmap\n",\
			inodes_difference_4gd);
	}
	if(blocks_difference_4gd){
		gd->bg_free_blocks_count = free_blocks;
		(*counter) += blocks_difference_4gd;
		printf("Fixed: block group's free blocks counter was off by %d compared to the bitmap\n",\
			blocks_difference_4gd);
	}
}

/******************************************************************************
	Check file_type, inode, dtime and block
******************************************************************************/
void check_bcde(inode *cur_dir_inode, int *counter){
	int max_block_index = cur_dir_inode->i_blocks/2;
	dir_entry *child_dir_entry;
	inode *child_inode;
	int i, rec_len_sum;
	for(i=0;i<max_block_index;i++){
		child_dir_entry = (dir_entry *) get_block(cur_dir_inode->i_block[i]);
		rec_len_sum = 0;
		while(rec_len_sum < EXT2_BLOCK_SIZE){
			child_inode = (inode *) get_inode(child_dir_entry->inode);
			// check b
			if(child_inode->i_mode & EXT2_S_IFLNK && child_dir_entry->file_type != EXT2_FT_SYMLINK){
				child_dir_entry->file_type = EXT2_FT_SYMLINK;
				(*counter)++;
				printf("Fixed: Entry type vs inode mismatch: inode [%d]\n", child_dir_entry->inode);
			}
			if(child_inode->i_mode & EXT2_S_IFREG && child_dir_entry->file_type != EXT2_FT_REG_FILE){
				child_dir_entry->file_type = EXT2_FT_REG_FILE;
				(*counter)++;
				printf("Fixed: Entry type vs inode mismatch: inode [%d]\n", child_dir_entry->inode);
			}
			if(strncmp(child_dir_entry->name,".",1) && strncmp(child_dir_entry->name,"..",2)\
				&& strncmp(child_dir_entry->name,"lost+found",10)){
				if(child_inode->i_mode & EXT2_S_IFDIR && child_dir_entry->file_type == EXT2_FT_DIR){
					check_bcde(child_inode, counter);
				}else if(child_inode->i_mode & EXT2_S_IFDIR && child_dir_entry->file_type != EXT2_FT_DIR){
					child_dir_entry->file_type = EXT2_FT_DIR;
					(*counter)++;
					printf("Fixed: Entry type vs inode mismatch: inode [%d]\n", child_dir_entry->inode);
					check_bcde(child_inode, counter);
				}
			}
			// check c
			if(!check_against_inode_bitmap(child_dir_entry->inode)){
				update_inode_bitmap(child_dir_entry->inode, ADD_MODE, NOT_DIR);
				(*counter)++;
				printf("Fixed: inode [%d] not marked as in-use\n", child_dir_entry->inode);
			}
			// check d
			if(child_inode->i_dtime){
				child_inode->i_dtime = 0;
				(*counter)++;
				printf("Fixed: valid inode marked for deletion: [%d]\n", child_dir_entry->inode);
			}
			// check e
			check_e(child_inode, counter, child_dir_entry->inode);
			rec_len_sum += child_dir_entry->rec_len;
			child_dir_entry = (void *) child_dir_entry + child_dir_entry->rec_len;
		}
	}
}

/******************************************************************************
	Check all blocks in an inode against block bitmap
******************************************************************************/
void check_e(inode *cur_inode, int *counter, int inode_no){
	int max_block_index = cur_inode->i_blocks/2;
	int i, block_counter = 0;
	for(i=0;i<max_block_index;i++){
		if(!check_against_block_bitmap(cur_inode->i_block[i])){
			update_block_bitmap(cur_inode->i_block[i], ADD_MODE);
			block_counter++;
		}
	}
	if(block_counter){
		printf("Fixed: %d in-use data blocks not marked in data bitmap for inode: [%d]\n", block_counter, inode_no);
		(*counter)+=block_counter;
	}
}