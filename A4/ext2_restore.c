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
		fprintf(stderr, "ext2_restore: cannot restore directory ‘/’\n");
		exit(EISDIR);
	}

	int abs_path_depth=0;
	char **abs_path_tokens = parse_absolute_path(argv[2], &abs_path_depth, "ext2_restore");

	int parent_inode_no = EXT2_ROOT_INO;
	inode *cur_dir_inode  = (inode *) get_inode(parent_inode_no);
	
	// get parent directory of the target file where it would be restored
	char *dir_name;
	int i = 0;
	while(i < abs_path_depth - 1){
		dir_name = abs_path_tokens[i];
		parent_inode_no = get_child_dir_inode_no(dir_name, cur_dir_inode, "ext2_restore");
		cur_dir_inode = (inode *) get_inode(parent_inode_no);
		free(dir_name);
		i++;
	}

	char *target_dir_name = abs_path_tokens[i];

	// skip along gap and get the valid directory entry before the one we want to restore
	dir_entry *cur_dir_entry_4file;
	dir_entry *cur_dir_entry_4gap_4file;
	dir_entry *cur_dir_entry_4symlink;
	dir_entry *cur_dir_entry_4gap_4symlink;
	int result_4file = get_dir_entry_prev_4restore(&cur_dir_entry_4file, &cur_dir_entry_4gap_4file,\
	 target_dir_name, cur_dir_inode, EXT2_FT_REG_FILE);
	int result_4symlink = get_dir_entry_prev_4restore(&cur_dir_entry_4symlink, &cur_dir_entry_4gap_4symlink,\
	 target_dir_name, cur_dir_inode, EXT2_FT_SYMLINK);

	if(result_4file == CHILD_NOT_MATCH && result_4symlink == CHILD_NOT_MATCH){
		fprintf(stderr, "ext2_restore: cannot restore directory: %s\n", target_dir_name);
		exit(EISDIR);
	}

	if(result_4file == NOT_RESTORABLE || result_4symlink == NOT_RESTORABLE){
		fprintf(stderr, "ext2_restore: %s: file or link already exists\n", target_dir_name);
		exit(EEXIST);
	}

	if(result_4file == CHILD_NOT_FOUND){
		fprintf(stderr, "ext2_restore: %s: no such file or link\n", target_dir_name);
		exit(ENOENT);
	}

	// update all fileds of this directory entry and its correspoding inode, then
	// update block and inode bitmap based on symlink or file
	if(result_4file == CHILD_NOT_MATCH){
		restore(cur_dir_entry_4symlink, cur_dir_entry_4gap_4symlink);
	}else{
		restore(cur_dir_entry_4file, cur_dir_entry_4gap_4file);
	}

	return 0;
}