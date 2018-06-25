#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include "ext2.h"

#define BYTE_TO_BINARY_PATTERN " %c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)  \
  (byte & 0x01 ? '1' : '0'), \
  (byte & 0x02 ? '1' : '0'), \
  (byte & 0x04 ? '1' : '0'), \
  (byte & 0x08 ? '1' : '0'), \
  (byte & 0x10 ? '1' : '0'), \
  (byte & 0x20 ? '1' : '0'), \
  (byte & 0x40 ? '1' : '0'), \
  (byte & 0x80 ? '1' : '0')

unsigned char *disk;


int main(int argc, char **argv) {

    if(argc != 2) {
        fprintf(stderr, "Usage: %s <image file name>\n", argv[0]);
        exit(1);
    }
    int fd = open(argv[1], O_RDWR);

    disk = mmap(NULL, 128 * 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(disk == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }

    struct ext2_super_block *sb = (struct ext2_super_block *)(disk + 1024);
    printf("Inodes: %d\n", sb->s_inodes_count);
    printf("Blocks: %d\n", sb->s_blocks_count);
    // TE7
    struct ext2_group_desc *db = (struct ext2_group_desc *)(disk + 2048);
    printf("Block group:\n");
    printf("    block bitmap: %d\n", db->bg_block_bitmap);
    printf("    inode bitmap: %d\n", db->bg_inode_bitmap);
    printf("    inode table: %d\n", db->bg_inode_table);
    printf("    free blocks: %d\n", db->bg_free_blocks_count);
    printf("    free inodes: %d\n", db->bg_free_inodes_count);
    printf("    used_dirs: %d\n", db->bg_used_dirs_count);
    // TE8
    char *block_bitmap = (char *)(disk + (db->bg_block_bitmap)*EXT2_BLOCK_SIZE);
    int i;
    printf("Block bitmap:");
    for(i=0; i<sb->s_blocks_count/8; i++){
        printf(BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(*(block_bitmap+i)));
    }
    printf("\n");
    char *inode_bitmap = (char *)(disk + (db->bg_inode_bitmap)*EXT2_BLOCK_SIZE);
    printf("Inode bitmap:");
    for(i=0; i<sb->s_inodes_count/8; i++){
        printf(BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(*(inode_bitmap+i)));
    }
    printf("\n\n");
    char *inode_table = (char *)(disk + (db->bg_inode_table) * EXT2_BLOCK_SIZE);
    struct ext2_inode *inode;
    // Initialize type to be unknown marked as 'X'
    char type = 'X';
    int max_block_count = 0;
    int j;
    int *directory_bytemap=malloc(sizeof(int)  * sb->s_inodes_count);
    memset(directory_bytemap, 0, sb->s_inodes_count);
    printf("Inodes:\n");
    for(i=EXT2_ROOT_INO-1;i<sb->s_inodes_count;i++){
        if(i < EXT2_GOOD_OLD_FIRST_INO && i != EXT2_ROOT_INO - 1){
            continue;
        }
        inode = (struct ext2_inode *) (inode_table + sizeof(struct ext2_inode) * i);
        if(inode->i_size == 0) {
            continue;
        }
        if(inode->i_mode & EXT2_S_IFREG){
            type = 'f';
        }else if (inode->i_mode & EXT2_S_IFDIR){
            type = 'd';
            directory_bytemap[i] = 1;
        }
        printf("[%d] type: %c size: %d links: %d blocks: %d\n", i + 1, type, inode->i_size, inode->i_links_count, inode->i_blocks);
        printf("[%d] Blocks: ", i + 1);
        max_block_count = inode->i_blocks/(2<<sb->s_log_block_size);
        for (j = 0; j < max_block_count; j++) {
            if (j > 11) {
                break;
            }
            printf(" %d", inode->i_block[j]);
        }
        printf("\n");
    }
    // TE9
    printf("\nDirectory Blocks:\n");
    for (i=EXT2_ROOT_INO-1; i<sb->s_inodes_count; i++){
        if(!directory_bytemap[i]){
            continue;
        }
        inode = (struct ext2_inode *)(inode_table + sizeof(struct ext2_inode) * i);

        max_block_count = inode->i_blocks/(2<<sb->s_log_block_size);
        unsigned int cur_block;
        for (j=0; j<max_block_count; j++) {
            cur_block = inode->i_block[j];
            printf("   DIR BLOCK NUM: %d (for inode %d)\n", cur_block, i + 1);

             // Extract the ext2_dir_entry structure out of the block
            struct ext2_dir_entry *dir_entry = (struct ext2_dir_entry *)(disk + EXT2_BLOCK_SIZE * cur_block);

            int len=0;
            while(len<EXT2_BLOCK_SIZE){
                len += dir_entry->rec_len;
                if ((unsigned short) dir_entry->file_type == EXT2_FT_REG_FILE) {
                    type = 'f';
                }else if((unsigned short) dir_entry->file_type == EXT2_FT_DIR) {
                    type = 'd';
                }
                char *entity_name=malloc(sizeof(char)  * dir_entry->name_len);
                strncpy(entity_name, dir_entry->name, dir_entry->name_len);
                printf("Inode: %d rec_len: %d name_len: %d type= %c name=%s\n", dir_entry->inode, dir_entry->rec_len\
                 , dir_entry-> name_len, type, entity_name);
                free(entity_name);
                dir_entry = (void *)dir_entry + dir_entry->rec_len;
            }
        }
    }
    free(directory_bytemap);
    return 0;
}
