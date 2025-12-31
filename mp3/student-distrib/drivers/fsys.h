/* fsys.h - header for file system
 * vim:ts=4 noexpandtab
 */

#ifndef _FSYS_H
#define _FSYS_H

#include "../lib.h"

#define DENTRY_SIZE             64
#define DISK_BLOCK_SIZE         4096

#define RTC_FILE_TYPE           0
#define DIRECTORY_FILE_TYPE     1
#define REGULAR_FILE_TYPE       2

/* 64 byte boot dentry */
typedef struct boot_dentry_t {
    uint32_t num_dir_entries;
    uint32_t num_inodes;
    uint32_t num_dblocks;
    uint8_t reserved_52[52];
} boot_dentry_t;

/* single dentry struct */
typedef struct dentry_t {
    char file_name[32];
    uint32_t file_type;
    uint32_t inode_idx;
    uint8_t reserved_24[24];
} dentry_t;

/* inode struct containing file size and block idxs */
typedef struct inode_t {
    uint32_t file_size;
    uint32_t block_idx_arr[1023];
} inode_t;

/*initializes file system with memory address to disk*/
void init_fsys(uint32_t starting_addr);

/*reads file dentry by user readable name*/
int32_t read_dentry_by_name(uint8_t* fname, dentry_t* dentry);

/*reads file dentry by index number*/
int32_t read_dentry_by_index(uint32_t index, dentry_t* dentry);

/*reads data segment indicated by offset and lengthinto parameterized buffer from a specified inode*/
int32_t read_data(uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t length);

/* get file length */
int32_t get_file_length(char* fname);

/* file operations jumptable struct */
typedef struct fops_jumptable_t {
    int32_t (*read)(int32_t fd, void* buf, int32_t nbytes);
    int32_t (*write)(int32_t fd, const void* buf, int32_t nbytes);
    int32_t (*open)(const uint8_t* filename);
    int32_t (*close)(int32_t fd);
} fops_jumptable_t;

/* file descriptor struct */
typedef struct file_desc_t {
    fops_jumptable_t* fops_table_ptr;
    uint32_t inode_num;
    uint32_t file_pos;
    uint32_t flags;
} file_desc_t;

/* file read syscall */
int32_t file_read(int32_t fd, void* buf, int32_t nbytes);

/* file write syscall */
int32_t file_write(int32_t fd, const void* buf, int32_t nbytes);

/* file open syscall */
int32_t file_open(const uint8_t* filename);

/* file close syscall */
int32_t file_close(int32_t fd);

/* directory read syscall */
int32_t directory_read(int32_t fd, void* buf, int32_t nbytes);

/* directory write syscall */
int32_t directory_write(int32_t fd, const void* buf, int32_t nbytes);

/* directory open syscall */
int32_t directory_open(const uint8_t* filename);

/* directory close syscall */
int32_t directory_close(int32_t fd);

#endif /* _FSYS_H */
