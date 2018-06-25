#include "utils.h"

unsigned char *disk;
char *symlink_path;

int main(int argc, char **argv) {

	if(argc != 4) {
		fprintf(stderr, "Usage: %s <image file name> <native absolute path> <absolute path>\n", argv[0]);
		exit(1);
	}

	init_FS(argv[1]);
	
	int source_file;
	if ((source_file = open(argv[2], O_RDONLY)) == -1){
		fprintf(stderr, "%s: %s: no such file\n", argv[0], argv[2]);
    		exit(ENOENT);
    	}

	int abs_path_depth=0;
	char **abs_path_tokens = parse_absolute_path(argv[3], &abs_path_depth, "ext2_cp");

	int parent_inode_no = EXT2_ROOT_INO;
	inode *cur_dir_inode  = (inode *) get_inode(parent_inode_no);
	
	// get parent directory of the target file where it would be placed in
	char *dir_name;
	int i = 0;
	while(i < abs_path_depth - 1){
		dir_name = abs_path_tokens[i];
		parent_inode_no = get_child_dir_inode_no(dir_name, cur_dir_inode, "ext2_cp");
		cur_dir_inode = (inode *) get_inode(parent_inode_no);
		free(dir_name);
		i++;
	}

	char *target_dir_name = argv[2];

	int  child_inode_no = get_child_inode_no(target_dir_name, cur_dir_inode, EXT2_FT_DIR);

	if(child_inode_no != CHILD_NOT_FOUND){
		fprintf(stderr, "ext2_cp: %s: the file or directory already exists\n", target_dir_name);
		exit(EEXIST);
	}

	// get last block of current directory
	char *last_block = get_last_block(cur_dir_inode);

	// initialize all fileds of this directory entry and its correspoding inode, then
	// update block and inode bitmap
	add_dir_entry_4file(last_block, target_dir_name, source_file);

	if (close(source_file) == -1){
		perror("close");
    		exit(1);
    	}

	return 0;
}