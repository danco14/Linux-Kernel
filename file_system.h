#ifndef _FILE_SYSTEM_H
#define _FILE_SYSTEM_H

#include "types.h"

#define NAME_LENGTH 32

#ifndef ASM
/* Directory entry struct */
struct dentry_struct{
  uint8_t file_name[NAME_LENGTH];
  uint32_t file_type;
  uint32_t inode_num;
  uint32_t reserved24[6];
};

// directory entry struct
typedef struct dentry_struct dentry_t;

/* Boot block struct */
struct boot_struct{
  uint32_t num_dentries;
  uint32_t num_inodes;
  uint32_t num_dblocks;
  uint32_t reserved52[13];
  dentry_t dentries[63];
};

// boot block
typedef struct boot_struct boot_block_t;


void file_system_init(uint32_t* file_sys_start);

dentry_t* find_dentry(const uint8_t* filename);

int32_t read_dentry_by_name(const uint8_t* filename, dentry_t* dentry);

int32_t read_dentry_by_index(uint32_t index, dentry_t* dentry);

int32_t read_data(uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t length);

int32_t file_open(const uint8_t* filename);

int32_t file_close(int32_t fd);

int32_t file_read(int32_t fd, void* buf, int32_t num_bytes);

int32_t file_write(int32_t fd, const void* buf, int32_t num_bytes);

int32_t dir_open(const uint8_t* filename);

int32_t dir_close(int32_t fd);

int32_t dir_read(int32_t fd, void* buf, int32_t nbytes);

int32_t dir_write(int32_t fd, const void* buf, int32_t num_bytes);

#endif /* ASM */

#endif /* _FILE_SYSTEM_H */
