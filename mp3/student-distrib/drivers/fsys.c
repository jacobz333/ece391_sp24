#include "fsys.h"
#include "../process.h"

extern pcb_t* current_PCB;

/* file operations jumptable for files */
fops_jumptable_t file_jmptable = {
    file_read,
    file_write,
    file_open,
    file_close
};

/* file operations jumptable for directories */
fops_jumptable_t directory_jmptable = {
    directory_read,
    directory_write,
    directory_open,
    directory_close
};

/* important addresses */
static uint32_t disk_mem_base_addr;     /* base address of the whole disk */
static uint32_t file_dir_arr_base_addr; /* base address of the directory table within boot block */
static uint32_t inodes_arr_base_addr;   /* base address of inodes array */

/* structs */
static boot_dentry_t* boot_block;   // 0th directory block containing the number of inodes, dentries, and datablocks
static dentry_t* file_dir_arr;      // dentry array of length 63 for the directory 
static inode_t* inodes_arr;         // inode array of dynamic length

/* init_fsys - file system initialization
 * 
 * Inputs: uint32_t starting addr
 * Outputs: None
 * Side Effects: sets the base address for the memory mapped disk as well as useful base addresses
 */
void init_fsys(uint32_t starting_addr) {
    disk_mem_base_addr = starting_addr;
    file_dir_arr_base_addr = disk_mem_base_addr + DENTRY_SIZE;  // dentry directory base address
    inodes_arr_base_addr = disk_mem_base_addr + DISK_BLOCK_SIZE; // inode blocks base address
    
    boot_block = (boot_dentry_t*) disk_mem_base_addr;           // 64 byte bootblock
    file_dir_arr = (dentry_t*) file_dir_arr_base_addr;
    inodes_arr = (inode_t*) inodes_arr_base_addr;
}

/* read_dentry_by_name - finding the dentry by the user readable name
 * 
 * Inputs: uint8_t* fname - dentry name, dentry_t* dentry - empty dentry struct
 * Outputs: int32_t = 0 indicates if the read was successful else return -1
 * Side Effects: Populates the input dentry pointer with dentry items from the file read
 */
int32_t read_dentry_by_name(uint8_t* fname, dentry_t* dentry) {
    uint32_t i;         /* loop index */
    
    // loop over directory entries to find file name
    for (i = 0; i < boot_block->num_dir_entries; i++) {
        if (strncmp(file_dir_arr[i].file_name, (char*)fname, 32) == 0) {
            memcpy(dentry, &file_dir_arr[i], DENTRY_SIZE); // copy the whole dentry
            return 0;
        }
    }

    // failure when fname not found
    return -1;                                                      //if none found return 0
}

/* read_dentry_by_index - finding the dentry by the index in the directory
 *      note that index is the index in the boot block, NOT the inode index
 * 
 * Inputs:  uint32_t index - target index in boot block
 *          dentry_t* dentry - dentry struct to fill
 * Outputs: int32_t = 0 indicates if the read was successful else return -1
 * Side Effects: Populates the input dentry pointer with dentry items from the file read
 */
int32_t read_dentry_by_index(uint32_t index, dentry_t* dentry) {
    // failure if index is boot dentry or outside the number of dentries
    if (index == 0 || index > boot_block->num_dir_entries) {
        return -1;                                              
    }

    // copy the whole dentry
    memcpy(dentry, &file_dir_arr[index-1], DENTRY_SIZE);
    return 0;
}

/* read_data - read specified number of bytes starting at offset (in bytes) within file from inode
 * 
 * Inputs:  uint32_t inode - inode number
 *          uint32_t offset - offset within file
 *          uint8* buf - output buffer
 *          uint32_t length - number of bytes to read
 * Outputs: int32_t, either a failed read or the number of bytes that were read
 * Side Effects: populates buffer with contents from memory
 */
int32_t read_data(uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t length) {
    uint32_t i;                 /* loop index */
    uint32_t bytes_to_copy;     /* number of bytes to copy */
    inode_t* curr_inode;        /* pointer to inode in disk */
    uint32_t block_base_addr;   /* starting address of beginning of data blocks in disk */
    uint32_t startingblock_idx; /* starting data block index within inode */
    uint32_t startingbyte_addr; /* address of starting byte within the starting data block */
    uint32_t endingblock_idx;   /* ending data block index within inode */
    uint32_t endingbyte_addr;   /* address of ending byte within the ending data block */
    uint32_t currentblock_addr; /* base address of data block for loop */
    uint32_t bytes_read;        /* total number of bytes read so far */

    // check that inode number is in range
    if (inode >= boot_block->num_inodes) {
        return -1;
    }

    // set useful variables
    curr_inode = &inodes_arr[inode];
    block_base_addr = (boot_block->num_inodes)*DISK_BLOCK_SIZE + inodes_arr_base_addr;

    // check that beginning byte is within file size
    if (offset >= curr_inode->file_size) {
        return -1;
    }

    // check that ending byte is within file size
    if (offset + length > curr_inode->file_size) {
        return -1;
    }

    // check for bad data block indices
    for (i = 0; i < 63; i++) {
        if (curr_inode->block_idx_arr[i] >= boot_block->num_dblocks) {
            return -1;
        }
    }

    // setup bounds
    startingblock_idx = offset / DISK_BLOCK_SIZE;           // starting block index within the inode
    startingbyte_addr = offset % DISK_BLOCK_SIZE;           // starting byte address within starting data block
    endingblock_idx = (offset + length) / DISK_BLOCK_SIZE;  // ending block index within the inode
    endingbyte_addr = (offset + length) % DISK_BLOCK_SIZE;  // ending byte address within ending data block
    
    /* order of operations for the copy
    *  1.Calculate the number of bytes to copy(depending on the situation)
    *  2.Calculates current block base address to copy from (current block_addr)
    *  3.memcopy using the base address for the block and the buffer. 
    *    The buffer starting address is incremented by the current bytes read
    *  4.increment the bytes read by the bytes copied
    */
    bytes_read = 0;
    if (startingblock_idx == endingblock_idx) { // data to copy is within one data block
        bytes_to_copy = length;                 
        currentblock_addr = curr_inode->block_idx_arr[startingblock_idx]*DISK_BLOCK_SIZE + block_base_addr;
        memcpy((void*)buf, (void*)(currentblock_addr + startingbyte_addr), bytes_to_copy);
        bytes_read += bytes_to_copy;

    } else { // data to copy is in multiple data blocks

        /* starting block */
        bytes_to_copy = DISK_BLOCK_SIZE - startingbyte_addr; // get remaining bytes in starting data block
        currentblock_addr = curr_inode->block_idx_arr[startingblock_idx]*DISK_BLOCK_SIZE + block_base_addr;
        memcpy((void*)buf, (void*)(currentblock_addr + startingbyte_addr), bytes_to_copy); // copy from within starting data block
        bytes_read += bytes_to_copy;

        /* blocks in between */
        for (i = startingblock_idx + 1; i < endingblock_idx; i++) {
            bytes_to_copy = DISK_BLOCK_SIZE; // get all bytes in data block
            currentblock_addr = curr_inode->block_idx_arr[i]*DISK_BLOCK_SIZE + block_base_addr;
            memcpy((void*)(buf + bytes_read), (void*)currentblock_addr, DISK_BLOCK_SIZE); // copy from start of data block
            bytes_read += DISK_BLOCK_SIZE;
        }

        /* ending block */
        bytes_to_copy = endingbyte_addr; // get beginning bytes in ending block
        currentblock_addr = curr_inode->block_idx_arr[endingblock_idx]*DISK_BLOCK_SIZE + block_base_addr;
        memcpy((void*)(buf + bytes_read), (void*)currentblock_addr, bytes_to_copy); // copy from start of ending data block
        bytes_read += bytes_to_copy;
    }
    
    // return 0 if we read up to the end of the file
    if (offset + bytes_read == curr_inode->file_size) {
        return 0;
    } else { // otherwise return number of bytes read
        return bytes_read;  
    }
}


int32_t get_file_length(char* fname) {
    inode_t* inode_ptr;     /* pointer to inode in disk */
    dentry_t dentry;        /* dentry to fill when we find the file */

    // try to find the file
    if (read_dentry_by_name((uint8_t*)fname, &dentry) == -1) {
        return -1;
    }

    // check the file type is a normal file
    if (dentry.file_type != REGULAR_FILE_TYPE) { 
        return -1;
    }

    // check that inode number is in range
    if (dentry.inode_idx >= boot_block->num_inodes) {
        return -1;
    }

    // get pointer to inode
    inode_ptr = &inodes_arr[dentry.inode_idx];
    return inode_ptr->file_size;
}

/* file_read - file read syscall
 *      fd is already verified to be in use by read syscall
 *      attempt to read some bytes from file, copies read data to given buffer
 * 
 * Inputs: fd - index of file descriptor
 *         buf - buffer to fill
 *         nbytes - number of bytes to read
 * Outputs: number of bytes read
 * Side Effects: fills buf, increments file position in file
 */
int32_t file_read(int32_t fd, void* buf, int32_t nbytes) {
    file_desc_t* file_desc_ptr = &(current_PCB->file_desc_arr[fd]); /* current file desciptor */
    inode_t* inode_ptr = &inodes_arr[file_desc_ptr->inode_num];  /* current inode */

    // return 0 when file position in file is already past file size
    if (file_desc_ptr->file_pos >= inode_ptr->file_size) {
        return 0;
    }

    // copy up till the end of the file
    if (file_desc_ptr->file_pos + nbytes >= inode_ptr->file_size) {
        nbytes = inode_ptr->file_size - file_desc_ptr->file_pos;
    }

    // read data and increment file position in file descriptor
    read_data(file_desc_ptr->inode_num, file_desc_ptr->file_pos, buf, nbytes);
    file_desc_ptr->file_pos += nbytes;

    // return number of bytes read
    return nbytes;
}

/* file_write - file write syscall
 *      attempt to write onto file, always fails for read-only filesystem
 * 
 * Inputs: fd - unused
 *         buf - unused
 *         nbytes - unused
 * Outputs: -1 for failure
 * Side Effects: None
 */
int32_t file_write(int32_t fd, const void* buf, int32_t nbytes) {
    return -1; // read-only file system
}

/* file_open - file open syscall
 *      filename is already found by open syscall
 *      file type is verified by open syscall
 * 
 * Inputs: filename - unused
 * Outputs: 0 for success
 * Side Effects: None
 */
int32_t file_open(const uint8_t* filename) {
    return 0;
}

/* file_close - file close syscall
 *      fd is already verified to be in use by close syscall
 * 
 * Inputs: fd - unused
 * Outputs: 0 for success
 * Side Effects: None
 */
int32_t file_close(int32_t fd) {
    return 0;
}

/* directory_read - directory read syscall
 *      fd is already verified to be in use by read syscall
 *      attempt to read the next file name in directory, copies file name to given buffer
 *      fails if no directory is open
 * 
 * Inputs: fd - index of file descriptor
 *         buf - buffer to fill
 *         nbytes - number of bytes to read
 * Outputs: number of bytes read
 * Side Effects: fills buf, increments offset in file
 */
int32_t directory_read(int32_t fd, void* buf, int32_t nbytes) {
    file_desc_t* file_desc_ptr = &(current_PCB->file_desc_arr[fd]); /* pointer to current file desciptor */

    // only copy up to 32 bytes of the file name
    if (nbytes > 32) {
        nbytes = 32;
    }

    // return 0 when idx in directory is already past file size
    if (file_desc_ptr->file_pos >= boot_block->num_dir_entries) {
        return 0;
    }
    
    // copy file name and increment file (directory) position
    strncpy(buf, file_dir_arr[file_desc_ptr->file_pos].file_name, nbytes);
    file_desc_ptr->file_pos++;

    // return number of bytes read
    return nbytes;
}

/* directory_write - directory write syscall
 *      attempt to write onto directory, always fails for read-only file system
 * 
 * Inputs: fd - unused
 *         buf - unused
 *         nbytes - unused
 * Outputs: -1 for failure
 * Side Effects: None
 */
int32_t directory_write(int32_t fd, const void* buf, int32_t nbytes) {
    return -1; // read-only file system
}

/* directory_open - directory open syscall
 *      filename is already found by open syscall
 *      file type is verified by open syscall
 * 
 * Inputs: filename - unused
 * Outputs: 0 for success
 * Side Effects: 
 */
int32_t directory_open(const uint8_t* filename) {
    return 0;
}


/* directory_close - directory close syscall
 *      fd is already verified to be in use by close syscall
 * 
 * Inputs: fd - unused
 * Outputs: 0 for success
 * Side Effects: 
 */
int32_t directory_close(int32_t fd) {
    return 0;
}

