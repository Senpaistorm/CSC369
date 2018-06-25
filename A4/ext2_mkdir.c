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
		fprintf(stderr, "ext2_mkdir: cannot create directory ‘/’: File exists\n");
		exit(EISDIR);
	}

	int abs_path_depth=0;
	char **abs_path_tokens = parse_absolute_path(argv[2], &abs_path_depth, "ext2_mkdir");

	int parent_inode_no = EXT2_ROOT_INO;
	inode *cur_dir_inode  = (inode *) get_inode(parent_inode_no);
	
	// get parent directory of the target directory where it would be placed in
	char *dir_name;
	int i = 0;
	while(i < abs_path_depth - 1){
		dir_name = abs_path_tokens[i];
		parent_inode_no = get_child_dir_inode_no(dir_name, cur_dir_inode, "ext2_mkdir");
		cur_dir_inode = (inode *) get_inode(parent_inode_no);
		free(dir_name);
		i++;
	}

	char *target_dir_name = abs_path_tokens[i];

	int  child_inode_no = get_child_inode_no(target_dir_name, cur_dir_inode, EXT2_FT_DIR);

	if(child_inode_no != CHILD_NOT_FOUND){
		fprintf(stderr, "ext2_mkdir: %s: the file or directory already exists\n", target_dir_name);
		exit(EEXIST);
	}

	// get last block of current directory
	char *last_block = get_last_block(cur_dir_inode);

	// initialize all fileds of this directory entry and its correspoding inode, then
	// update block and inode bitmap
	add_dir_entry_4dir(parent_inode_no, last_block, target_dir_name);

	free(target_dir_name);

	return 0;
}