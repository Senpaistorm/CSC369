#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include "ext2.h"

#define MAX_BLOCK 128
#define BLOCK_BITMAP_LEN MAX_BLOCK/8
#define MAX_INODE 32
#define INODE_BITMAP_LEN MAX_INODE/8
#define MAX_DIR_DEPTH 255 
#define DIR_ENTRY_LEN_EX_NAME 8
#define BIT_NOT_FOUND 0
#define PROTECTED_BLOCK_COUNT 9
#define PROTECTED_INODE_COUNT 11
#define AVAILABLE 1
#define NOT_AVAILABLE 0
#define DISK_SECTOR_SIZE 512
#define FIRST_ENTRY 1
#define NOT_FIRST_ENTRY 0
#define CHILD_NOT_FOUND -1
#define CHILD_NOT_MATCH -2
#define ADD_MODE 1
#define SUB_MODE -1
#define DIR 1
#define NOT_DIR 0
#define RESTORABLE 1
#define NOT_RESTORABLE 0

#define max(a, b) a>b?a:b
#define min(a,b) a<b?a:b

extern unsigned char *disk;
extern char *symlink_path;

void init_FS(char *);
super_block *get_super_block();
group_desc *get_group_desc();
char *get_block_bitmap();
char *get_inode_bitmap();
char *get_inode_table();
char *get_block(int);
char *get_inode(int);
void check_root_dir(char *, char *);
char **parse_absolute_path(char *, int *, char *);
int get_dir_entry_prev(dir_entry **, char *, inode *, unsigned char, int *);
int get_dir_entry_prev_4restore(dir_entry **, dir_entry **, char *, inode *, unsigned char);
int get_child_inode_no(char *, inode *, unsigned char);
int get_child_dir_inode_no(char *, inode *, char *);
int get_child_file_inode_no(char *, inode *, char *);
int get_child_link_inode_no(char *, inode *, char *);
char *get_last_block(inode *);
void add_dir_entry(dir_entry *, int, int, char *, unsigned char);
void add_dir_entry_4dir(int, char *, char *);
void add_dir_entry_4file(char *, char *, int);
void add_dir_entry_4link(int, char *, char *, int);
dir_entry *check_if_no_space(char *, char *, int *);
char *allocate_new_block(int *);
char *allocate_new_inode(int *);
void init_dir_entry(dir_entry *, int, int, char *, unsigned char);
void init_inode(inode *, unsigned short, int);
void update_super_block(int, int);
void update_group_desc(int, int, int);
void update_inode_bitmap(int, int, int);
void update_block_bitmap(int, int);
void update_links_count(int, int, int);
void update_inode(int, int);
void inode_blocks_shift(inode *, int);
int get_avail_block_no();
int get_avail_inode_no();
int get_avail_no(int, int, char *, int);
int get_avail_blocks(int, int *);
int bit_checker(char);
int check_against_inode_bitmap(int);
int check_against_block_bitmap(int);
void restore(dir_entry *, dir_entry *);
void check_a(int *);
void check_bcde(inode *, int *);
void check_e(inode *, int *, int);