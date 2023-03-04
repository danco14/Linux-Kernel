#include "file_system.h"
#include "lib.h"
#include "syscalls.h"

#define FOUR_KB 4096

/* Address of the file system */
static boot_block_t* boot_block;
static uint32_t* fs_start;

/*
 * file_system_init
 *    DESCRIPTION: Initializes the file system
 *    INPUTS: uint32_t* file_sys_start - a pointer to the boot block in memory
 *    OUTPUTS: none
 *    RETURN VALUE: none
 *    SIDE EFFECTS: Adds a boot_block_t pointer to the start of the file system
 */
void file_system_init(uint32_t* file_sys_start){
  /* Create a boot block pointer to access boot block values */
  boot_block = (boot_block_t*)file_sys_start;

  /* Store starting address of the file system */
  fs_start = file_sys_start;
}

/*
 * find_dentry
 *    DESCRIPTION: Finds a dentry given a filename
 *    INPUTS: const uint8_t* filename - Name of the dentry to find
 *    OUTPUTS: none
 *    RETURN VALUE: dentry_t* - a pointer to the found dentry
 *    SIDE EFFECTS: none
 */
dentry_t* find_dentry(const uint8_t* filename){
  int i; /* Loop variable */

  /* Go through all dentries */
  for(i = 0; i < boot_block->num_dentries; i++){
    /* Check if dentry matches the filename */
    if(strncmp((const int8_t*)(boot_block->dentries[i].file_name), (const int8_t*)filename, strlen((int8_t*)filename)) == 0
   && strncmp((const int8_t*)(boot_block->dentries[i].file_name), (const int8_t*)filename, NAME_LENGTH) == 0){
      /* Return address of the dentry */
      return &(boot_block->dentries[i]);
    }
  }

  /* Dentry not found */
  return NULL;
}

/*
 * read_dentry_by_name
 *    DESCRIPTION: Copies the dentry by filename
 *    INPUTS: const uint8_t* filename - Name of the dentry to copy
 *            dentry_t* dentry - dentry to copy to
 *    OUTPUTS: none
 *    RETURN VALUE: 0 for success or -1 for failure
 *    SIDE EFFECTS: none
 */
int32_t read_dentry_by_name(const uint8_t* filename, dentry_t* dentry){
  /* Get a pointer to the correct dentry */
  dentry_t *file_dentry = find_dentry(filename);

  /* Check if filename was valid */
  if(file_dentry == NULL){
    /* Return failure */
    return -1;
  }

  /* Copy the file name, file type, and inode number to the given dentry */
  strncpy((int8_t*)dentry, (int8_t*)file_dentry, NAME_LENGTH);
  dentry->file_type = file_dentry->file_type;
  dentry->inode_num = file_dentry->inode_num;

  /* Return success */
  return 0;
}

/*
 * read_dentry_by_index
 *    DESCRIPTION: Copies a dentry by the given index
 *    INPUTS: uint32_t index - the index of the dentry to copy
 *            dentry_t* dentry - the dentry to copy to
 *    OUTPUTS: none
 *    RETURN VALUE: 0 for success or -1 for failure
 *    SIDE EFFECTS: none
 */
int32_t read_dentry_by_index(uint32_t index, dentry_t* dentry){
  /* Check if index is greater than the number of files in directory */
  if(index >= boot_block->num_dentries){
    /* Return failure */
    return -1;
  }

  /* Pointer to the dentry at the given index */
  dentry_t* file_dentry = &(boot_block->dentries[index]);

  /* Copy the file name, file type, and inode number to the given dentry */
  strncpy((int8_t*)dentry, (int8_t*)file_dentry, NAME_LENGTH);
  dentry->file_type = file_dentry->file_type;
  dentry->inode_num = file_dentry->inode_num;

  /* Return success */
  return 0;
}

/*
 * read_data
 *    DESCRIPTION: Reads the data of a file
 *    INPUTS: uint32_t inode - the file's inode number
 *            uint32_t offset - how far in the file to start reading
 *            uint8_t* buf - where to copy the data to
 *            uint32_t length - how many bytes to read
 *    OUTPUTS: none
 *    RETURN VALUE: The number of bytes read, or -1 for failure
 *    SIDE EFFECTS: none
 */
int32_t read_data(uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t length){
  /* Check for a valid inode */
  if(inode >= (boot_block->num_inodes)){
    /* Return failure */
    return -1;
  }

  /* Address of given inode */
  uint8_t* inode_addr = (uint8_t*)fs_start+(FOUR_KB * (inode+1));

  /* Size of file */
  uint32_t file_size;

  /* Get the size of the file */
  memcpy(&file_size, inode_addr, 4);

  /* Check for a valid offset */
  if(offset > file_size){
    /* Return 0 bytes read */
    return 0;
  }

  /* Ending point in the file  */
  uint32_t end = (offset + length) > file_size ? file_size : offset + length;

  /* Current data block number */
  uint32_t dblock_num;

  /* Current point in the file */
  uint32_t cur = offset;

  /* Get the current dblock number */
  uint8_t* num_addr = inode_addr + 4 + (4 * (offset / FOUR_KB));
  memcpy(&dblock_num, num_addr, 4);

  /* Address of the dblock */
  uint8_t* dblock_addr = (uint8_t*)fs_start + (boot_block->num_inodes + 1)*FOUR_KB + (dblock_num*FOUR_KB);

  /* Total amount of data copied */
  uint32_t copied_length = 0;

  /* Loop until the end point is reached */
  while(cur != end){
    /* Amount copied per iteration: either the of the dblock or the set end point */
    uint32_t copyLength = FOUR_KB - (cur % FOUR_KB) > (end - cur) ? (end - cur) : FOUR_KB - (cur % FOUR_KB);

    /* Copy to the buffer */
    memcpy(buf + copied_length, dblock_addr + (cur % FOUR_KB), copyLength);

    /* Update current point in the file */
    cur += copyLength;

    /* Update amount copied */
    copied_length += copyLength;

    /* Get next dblock number */
    num_addr += 4;
    memcpy(&dblock_num, num_addr, 4);

    /* Get the address of the next dblock */
    dblock_addr = (uint8_t*)fs_start + (boot_block->num_inodes + 1)*FOUR_KB + (dblock_num*FOUR_KB);
  }

  /* Number of byte read */
  return copied_length;
}

/*
 * file_open
 *    DESCRIPTION: Opens a file and stores the file info
 *    INPUTS: const uint8_t* filename - the file to open
 *    OUTPUTS: none
 *    RETURN VALUE: 0 for success, -1 for failure
 *    SIDE EFFECTS: Initializes the file offset to 0 and stores the current inode number
 */
int32_t file_open(const uint8_t* filename){
  /* Return success */
  return 0;
}

/*
 * file_close
 *    DESCRIPTION: Closes a file
 *    INPUTS: none
 *    OUTPUTS: none
 *    RETURN VALUE: 0 for success
 *    SIDE EFFECTS: Sets the current inode number to -1 and offset to 0
 */
int32_t file_close(int32_t fd){
  /* Return success */
  return 0;
}

/*
 * file_read
 *    DESCRIPTION: Reads the data in a file to a buffer
 *    INPUTS: int32_t fd - file to read
 *            void* buf - the buffer to copy to
 *            uint32_t num_bytes - the number of bytes to read
 *    OUTPUTS: none
 *    RETURN VALUE: Number of bytes read
 *    SIDE EFFECTS: Updates the current file offset
 */
int32_t file_read(int32_t fd, void* buf, int32_t num_bytes){
  /* Check if a file is open */
  pcb_t* pcb = get_pcb_add();
  if(pcb->fdt[fd].inode == -1){
    /* Return failure */
    return -1;
  }

  /* Number of bytes read */
  int32_t read_ret;

  /* Read data, and check if file reading worked */
  if((read_ret = read_data(pcb->fdt[fd].inode, pcb->fdt[fd].file_position, (uint8_t*)buf, num_bytes)) == -1){
    /* Return failure */
    return -1;
  }

  /* Update point in file */
  pcb->fdt[fd].file_position += read_ret;

  /* Number of bytes read */
	return read_ret;
}

/*
 * file_write
 *    DESCRIPTION: Writes to a file
 *    INPUTS: none
 *    OUTPUTS: none
 *    RETURN VALUE: -1 for failure
 *    SIDE EFFECTS: none
 */
int32_t file_write(int32_t fd, const void* buf, int32_t num_bytes){
  /* File system is read only, so return failure */
  return -1;
}

/*
 * dir_open
 *    DESCRIPTION: Opens the directory
 *    INPUTS: const uint8_t* filename - name of the directory to open
 *    OUTPUTS: none
 *    RETURN VALUE: 0 for success
 *    SIDE EFFECTS: Sets the current index of the dentry to 0
 */
int32_t dir_open(const uint8_t* filename){
  /* Return success */
  return 0;
}

/*
 * dir_close
 *    DESCRIPTION: Close the directory
 *    INPUTS: none
 *    OUTPUTS: none
 *    RETURN VALUE: 0 for success
 *    SIDE EFFECTS: Sets dentry index to -1 to indicate that it's closed
 */
int32_t dir_close(int32_t fd){
  /* Return success */
  return 0;
}

/*
 * dir_read
 *    DESCRIPTION: Read a file name in the directory
 *    INPUTS: int32_t fd - directory to read from
 *            void* buf - buffer to read to
 *            nbytes - number of bytes the buffer can hold
 *    OUTPUTS: none
 *    RETURN VALUE: 0 for success, -1 for failure
 *    SIDE EFFECTS: Increments the dentry index to the next file
 */
int32_t dir_read(int32_t fd, void* buf, int32_t nbytes){
  /* Dentry struct to copy to */
  dentry_t dentry;
  pcb_t* pcb = get_pcb_add();

  /* Check for a valid index */
  if(pcb->fdt[fd].file_position == -1){
    /* Return failure */
    return -1;
  }

  /* Check if directory is done reading */
  if(pcb->fdt[fd].file_position >= boot_block->num_dentries){
    /* Return success */
    return 0;
  }

  /* Read dentry at the index, and check if read worked */
	if(-1 == read_dentry_by_index(pcb->fdt[fd].file_position, &dentry)){
    return -1;
  }

  /* Copy the file name to the buffer */
  strncpy((int8_t*)buf, (const int8_t*)dentry.file_name, NAME_LENGTH);

  /* Go to the next entry */
  pcb->fdt[fd].file_position++;

  /* Number of bytes wirtten is length of file name */
  return NAME_LENGTH;
}

/*
 * dir_write
 *    DESCRIPTION: Write to the directory
 *    INPUTS: none
 *    OUTPUTS: none
 *    RETURN VALUE: -1 for failure
 *    SIDE EFFECTS: none
 */
int32_t dir_write(int32_t fd, const void* buf, int32_t num_bytes){
  /* File system is read only, so return failure */
  return -1;
}
