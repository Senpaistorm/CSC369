#include "utils.h"

unsigned char *disk;
char *symlink_path;

int main(int argc, char **argv) {

	if(argc != 2) {
		fprintf(stderr, "Usage: %s <image file name>\n", argv[0]);
		exit(1);
	}

	init_FS(argv[1]);

	int counter = 0;
	inode *cur_dir_inode  = (inode *) get_inode(EXT2_ROOT_INO);

	// check a for super block and block group descriptor
	check_a(&counter);
	
	// check e fro root directory
	check_e(cur_dir_inode, &counter, EXT2_ROOT_INO);

	// check bcde for all the rest
	check_bcde(cur_dir_inode, &counter);

	if(counter) printf("%d file system inconsistencies repaired!\n", counter);
	else printf("No file system inconsistencies detected!\n");

	return 0;
}