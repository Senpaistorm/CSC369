#include "utils.h"

unsigned char *disk;
char *symlink_path;

int main(int argc, char **argv) {

	if(argc != 3) {
		fprintf(stderr, "Usage: %s <image file name> <absolute path>\n", argv[0]);
		exit(1);
	}

	init_FS(argv[1]);

	if(strlen(argv[2]) == 1 && !strncmp(argv[2], "/", 1)){
		fprintf(stderr, "ext2_rm: cannot remove directory ‘/’\n");
		exit(EISDIR);
	}

	int abs_path_depth=0;
	char **abs_path_tokens = parse_absolute_path(argv[2], &abs_path_depth, "ext2_rm");

	int parent_inode_no = EXT2_ROOT_INO;
	inode *cur_dir_inode  = (inode *) get_inode(parent_inode_no);

	// get parent directory of the target file where it would be removed
	char *dir_name;
	int i = 0;
	while(i < abs_path_depth - 1){
		dir_name = abs_path_tokens[i];
		parent_inode_no = get_child_dir_inode_no(dir_name, cur_dir_inode, "ext2_rm");
		cur_dir_inode = (inode *) get_inode(parent_inode_no);
		free(dir_name);
		i++;
	}

	char *target_dir_name = abs_path_tokens[i];

	int block_idx;
	dir_entry *prev_dir_entry_4file;
	dir_entry *prev_dir_entry_4symlink;
	int result_4file = get_dir_entry_prev(&prev_dir_entry_4file, target_dir_name, cur_dir_inode, EXT2_FT_REG_FILE, &block_idx);
	int result_4symlink = get_dir_entry_prev(&prev_dir_entry_4symlink, target_dir_name, cur_dir_inode, EXT2_FT_SYMLINK, &block_idx);

	if(result_4file == CHILD_NOT_MATCH && result_4symlink == CHILD_NOT_MATCH){
		fprintf(stderr, "ext2_rm: cannot remove directory: %s\n", target_dir_name);
		exit(EISDIR);
	}

	if(result_4file == CHILD_NOT_FOUND){
		fprintf(stderr, "ext2_rm: %s: no such file or link\n", target_dir_name);
		exit(ENOENT);
	}

	// update all fileds of this directory entry and its correspoding inode, then
	// update block and inode bitmap based on symlink or file
	int block_no = cur_dir_inode->i_block[block_idx];
	int deleted_inode_no;
	dir_entry *prev_dir_entry;
	dir_entry *cur_dir_entry;
	if(result_4file == CHILD_NOT_MATCH){
		prev_dir_entry = prev_dir_entry_4symlink;
		if(result_4symlink == FIRST_ENTRY){
			deleted_inode_no = prev_dir_entry->inode;
			prev_dir_entry->inode = 0;
			cur_dir_entry = prev_dir_entry;
			if(cur_dir_entry->rec_len == EXT2_BLOCK_SIZE){
				update_block_bitmap(block_no, SUB_MODE);
				inode_blocks_shift(cur_dir_inode, block_idx);
			}
		}
		if(result_4symlink == NOT_FIRST_ENTRY){
			cur_dir_entry = (void *) prev_dir_entry + prev_dir_entry->rec_len;
			deleted_inode_no = cur_dir_entry->inode;
			prev_dir_entry->rec_len += cur_dir_entry->rec_len;
			if(prev_dir_entry->inode == 0 && prev_dir_entry->rec_len== EXT2_BLOCK_SIZE){
				update_block_bitmap(block_no, SUB_MODE);
				inode_blocks_shift(cur_dir_inode, block_idx);
			}
		}
	}else{
		prev_dir_entry = prev_dir_entry_4file;
		if(result_4file == FIRST_ENTRY){
			deleted_inode_no = prev_dir_entry->inode;
			prev_dir_entry->inode = 0;
			cur_dir_entry = prev_dir_entry;
			if(cur_dir_entry->rec_len == EXT2_BLOCK_SIZE){
				update_block_bitmap(block_no, SUB_MODE);
				inode_blocks_shift(cur_dir_inode, block_idx);
			}
		}
		if(result_4file == NOT_FIRST_ENTRY){
			cur_dir_entry = (void *) prev_dir_entry + prev_dir_entry->rec_len;
			deleted_inode_no = cur_dir_entry->inode;
			prev_dir_entry->rec_len += cur_dir_entry->rec_len;
			if(prev_dir_entry->inode == 0 && prev_dir_entry->rec_len== EXT2_BLOCK_SIZE){
				update_block_bitmap(block_no, SUB_MODE);
				inode_blocks_shift(cur_dir_inode, block_idx);
			}
		}
	}

	inode *deleted_inode = (inode *) get_inode(deleted_inode_no); 
	deleted_inode->i_dtime = time(NULL);

	update_links_count(cur_dir_entry->inode, SUB_MODE, NOT_DIR);

	return 0;
}