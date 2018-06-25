#include "utils.h"

unsigned char *disk;
char *symlink_path;

int main(int argc, char **argv) {

	// check command line arguments and tokenize the string that we need
	int symlink_true = 0, target_path_depth=0, link_path_depth = 0;
	char **target_path_tokens;
	char **link_path_tokens;
	if(argc == 5){
		if(!strncmp(argv[2], "-s", 2) && strlen(argv[2]) == 2){
			if(strlen(argv[3]) == 1 && !strncmp(argv[3], "/", 1)){
				fprintf(stderr, "ext2_ln: cannot link ‘/’\n");
				exit(EISDIR);
			}
			symlink_true = 1;
			target_path_tokens= parse_absolute_path(argv[3], &target_path_depth, "ext2_ln");
			link_path_tokens = parse_absolute_path(argv[4], &link_path_depth, "ext2_ln");
			symlink_path = malloc((strlen(argv[3])+1) * sizeof(char));
			strcpy(symlink_path, argv[3]);
		}else{
			fprintf(stderr, "Usage: %s <-s> <image file name> <target name> <link name>\n", argv[0]);
			exit(1);
		}
	}else if(argc == 4) {
		if(strlen(argv[2]) == 1 && !strncmp(argv[2], "/", 1)){
				fprintf(stderr, "ext2_ln: cannot link ‘/’\n");
				exit(EISDIR);
		}
		target_path_tokens= parse_absolute_path(argv[2], &target_path_depth, "ext2_ln");
		link_path_tokens = parse_absolute_path(argv[3], &link_path_depth, "ext2_ln");
	}else{
		fprintf(stderr, "Usage: %s <image file name> <target name> <link name>\n", argv[0]);
		exit(1);
	}

	init_FS(argv[1]);

	// get parent directory of the target file where it is coming from
	int cur_inode_no = EXT2_ROOT_INO;
	inode *cur_inode  = (inode *) get_inode(cur_inode_no);
	char *dir_name;
	int i = 0;
	while(i < target_path_depth - 1){
		dir_name = target_path_tokens[i];
		cur_inode_no = get_child_dir_inode_no(dir_name, cur_inode, "ln");
		cur_inode = (inode *) get_inode(cur_inode_no);
		free(dir_name);
		i++;
	}
	char *target_file_name = target_path_tokens[i];
	int target_inode_no = get_child_file_inode_no(target_file_name, cur_inode, "ln");

	// get parent directory of the link where it would be placed in
	cur_inode_no = EXT2_ROOT_INO;
	cur_inode  = (inode *) get_inode(cur_inode_no);
	i = 0;
	while(i < link_path_depth){
		dir_name = link_path_tokens[i];
		cur_inode_no = get_child_dir_inode_no(dir_name, cur_inode, "ext2_ln");
		cur_inode = (inode *) get_inode(cur_inode_no);
		free(dir_name);
		i++;
	}

	int  child_inode_no = get_child_inode_no(target_file_name, cur_inode, EXT2_FT_REG_FILE);

	if(child_inode_no != CHILD_NOT_FOUND){
		fprintf(stderr, "ext2_ln: %s: the file or directory already exists\n", target_file_name);
		exit(EEXIST);
	}

	// get last block of current directory
	char *last_block = get_last_block(cur_inode);

	// initialize all fileds of this directory entry and its correspoding inode, then
	// update block and inode bitmap
	add_dir_entry_4link(target_inode_no, last_block, \
		target_file_name, symlink_true);

	free(target_file_name);
	free(symlink_path);

	return 0;
}