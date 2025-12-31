#include "lib.h"
#include "x86_desc.h"

#include "tests.h"
#include "page.h"

#include "./drivers/fsys.h"
#include "./drivers/rtc.h"
#include "./drivers/terminal.h"

/* choose whether or not to print some large things */
#define PRINTING 1

#define PASS 1
#define FAIL 0

/* format these macros as you see fit */
#define TEST_HEADER 	\
	printf("[TEST %s] Running %s at %s:%d\n", __FUNCTION__, __FUNCTION__, __FILE__, __LINE__)
#define TEST_OUTPUT(name, result)	\
	printf("[TEST %s] Result = %s\n", name, (result) ? "PASS" : "FAIL");

static inline void assertion_failure() {
	/* Use exception #15 for assertions, otherwise
	   reserved by Intel */
	asm volatile("int $15");
}

/* Checkpoint 1 tests */

/* IDT Test - Example
 * 		Asserts that first 10 IDT entries are not NULL
 * 
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage: Load IDT, IDT definition
 * Files: x86_desc.h/S
 */
int idt_test() {
	TEST_HEADER;

	int i;
	int result = PASS;
	for (i = 0; i < 10; ++i){
		if ((idt[i].offset_15_00 == NULL) && 
			(idt[i].offset_31_16 == NULL)){
			result = FAIL;
		}
	}

	return result;
}

/* IDT Initialization Test
 * 		Checks that the first 19 IDT entries have been initialized properly
 * 
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage: Load IDT, IDT definition
 * Files: x86_desc.h/S, asm_wrapper.h/S, intr.h/c
 */
int idt_initialization_test() {
	TEST_HEADER;

	int i;
	int result = PASS;

	for (i = 0; i < 20; i++) {
		// check IDT entry is present
		if (!idt[i].present) {
			result = FAIL;
		}
		// check handler exists
		if ((idt[i].offset_15_00 == NULL) && (idt[i].offset_31_16 == NULL)) {
			result = FAIL;
		}
		// check DPL (except Assertion Failure)
		if (i != 0x0F && idt[i].dpl != DPL_PRIVILEGED) {
			result = FAIL;
		}
		// check IDT entries are TRAP gates (except NMIs)
		if (i != 2 && 
			idt[i].reserved3 == 0x0 && 
			idt[i].reserved2 == 0x1 && 
			idt[i].reserved1 == 0x1) {
			result = FAIL;
		}
	}
	// check Assertion Failure is unprivileged
	if (idt[0x0F].dpl == DPL_PRIVILEGED) {
		result = FAIL;
	}
	// check NMI is an INT gate
	if (idt[2].reserved3 == 0x1 && 
		idt[2].reserved2 == 0x1 && 
		idt[2].reserved1 == 0x1) {
		result = FAIL;
	}
	

	return result;
}

/* Divide Error Test
 * 		Checks the Divide Error exception
 * 
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: blue screens the OS
 * Coverage: Divide Error
 * Files: intr.c
 */
int divide_error_test() {
	TEST_HEADER;

	int a = 0;
	int b = 1;
	b = b/a; // purposefully divide by 0

	return FAIL;
}

/* Breakpoint Test
 * 		Checks the Breakpoint exception
 * 
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: blue screens the OS
 * Coverage: Breakpoint
 * Files: intr.c
 */
int breakpoint_test() {
	TEST_HEADER;

	asm volatile("int $3"); // purposefully trigger interrupt vector 0x03

	return FAIL;
}

/* Invalid Opcode Test
 * 		Checks the Invalid Opcode fault
 * 
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: blue screens the OS
 * Coverage: Invalid Opcode
 * Files: intr.c
 */
int invalid_opcode_test() {
	TEST_HEADER;

	asm volatile("ud2"); // purposefully run undefined instruction

	return FAIL;
}

/* General Protection Test
 * 		Checks the General Protection fault
 * 
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: blue screens the OS
 * Coverage: General Protection
 * Files: intr.c
 */
int general_protection_test() {
	TEST_HEADER;

	asm volatile("int $13");

	return FAIL;
}

/* interrupt tests
 * 		Tests interrupts by forcefully calling them using asm int
 * 
 * Inputs: None
 * Outputs: no pass testing blue screen
 * Side Effects: blue screens the OS
 * Coverage: all
 * Files: intr.c
 */
int interrupt_tests(uint32_t intnum) {
	TEST_HEADER;

	if(intnum > 20){
		printf("Outside valid interrupt vector range");
		return FAIL;
	}
	switch(intnum){
		case 0:
			asm volatile("int $0");
			break;
		case 1:
			asm volatile("int $1");
			break;
		case 2:
			asm volatile("int $2");
			break;
		case 3:
			asm volatile("int $3");
			break;
		case 4:
			asm volatile("int $4");
			break;
		case 5:
			asm volatile("int $5");
			break;
		case 6:
			asm volatile("int $6");
			break;
		case 7:
			asm volatile("int $7");
			break;
		case 8:
			asm volatile("int $8");
			break;
		case 9:
			asm volatile("int $9");
			break;
		case 10:
			asm volatile("int $10");
			break;
		case 11:
			asm volatile("int $11");
			break;
		case 12:
			asm volatile("int $12");
			break;
		case 13:
			asm volatile("int $13");
			break;
		case 14:
			asm volatile("int $14");
			break;
		case 15:
			asm volatile("int $15");
			break;
		case 16:
			asm volatile("int $16");
			break;
		case 17:
			asm volatile("int $17");
			break;
		case 18:
			asm volatile("int $18");
			break;
		case 19:
			asm volatile("int $19");
			break;
		case 20:
			asm volatile("int $20");
			break;
		default:
			break;
	}

	return FAIL;
}

/* dereference_null_test
 * 		Tests dereferencing null for page fault
 * 
 * Inputs: None
 * Outputs: no pass 
 * Side Effects: blue screens the OS returns page fault
 * Coverage: Page Fault
 * Files: intr.c
 */
int dereference_null_test() {
	uint32_t bad;
	uint32_t* ptr = NULL;
	bad = *ptr;

	return FAIL;
}

/* dereference_outside_page_test
 * 		Tests dereferencing outside page for page fault
 * 
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: blue screens the OS
 * Coverage: Page Fault
 * Files: intr.c
 */
int dereference_outside_page_test() {
	uint32_t bad;
	uint32_t* ptr = (uint32_t*)(KERNEL_MEM_BASE_ADDR - 1);
	bad = *ptr;

	return FAIL;
}

/* dereference_inside_page_test
 * 		Tests dereferencing inside a page
 * 
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage: Page Fault
 * Files: intr.c
 */
int dereference_inside_page_test() {
	uint32_t good;
	uint32_t* ptr = (uint32_t*)(KERNEL_MEM_BASE_ADDR + 1);
	good = *ptr;

	return PASS; // we better pass this
}

/* Checkpoint 2 tests */

/* read_dentry_by_name_test
 * 		tests reading dentry by name
 * 
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage: read_dentry_by_name
 * Files: fsys.c
 */
int read_dentry_by_name_test() {
	dentry_t dentry;
	char* test = "hello";

	// provide bad values 
	strncpy(dentry.file_name, test, 32);
	dentry.file_type = 7;
	dentry.inode_idx = 9;
	
	// find ls program
	char* name = "ls";
	if (read_dentry_by_name((uint8_t*)name, &dentry) != 0) { // must return 0
		return FAIL;
	}
	if (strncmp(name, dentry.file_name, 32) != 0) { // must be "ls"
		return FAIL;
	}
	if (dentry.file_type != 2) { // "ls" is a regular file
		return FAIL;
	}
	if (dentry.inode_idx != 5) { // "ls" contains inode index 5
		return FAIL;
	}

	// provide bad values 
	strncpy(dentry.file_name, test, 32);
	dentry.file_type = 7;
	dentry.inode_idx = 9;
	
	// find . directory
	name = ".";
	if (read_dentry_by_name((uint8_t*)name, &dentry) != 0) { // must return 0
		return FAIL;
	}
	if (strncmp(name, dentry.file_name, 32) != 0) { // must be "."
		return FAIL;
	}
	if (dentry.file_type != 1) { // "." is a directory
		return FAIL;
	}
	
	// provide bad values 
	strncpy(dentry.file_name, test, 32);
	dentry.file_type = 7;
	dentry.inode_idx = 9;

	// find smth that doesnt exist
	name = "file doesnt exist";
	if (read_dentry_by_name((uint8_t*)name, &dentry) != -1) { // must return -1
		return FAIL;
	}

	return PASS; // no failures
}

/* read_dentry_by_index_test
 * 		tests reading dentry by index
 * 
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage: read_dentry_by_index
 * Files: fsys.c
 */
int read_dentry_by_index_test() {
	dentry_t dentry;
	char test[32] = "hello";

	// provide bad values 
	strncpy(dentry.file_name, test, 32);
	dentry.file_type = 7;
	dentry.inode_idx = 9;
	
	// find ls program (boot block index = 13)
	char* name = "ls";
	if (read_dentry_by_index(13, &dentry) != 0) { // must return 0
		return FAIL;
	}
	if (strncmp(name, dentry.file_name, 32) != 0) { // must be "ls"
		return FAIL;
	}
	if (dentry.file_type != 2) { // "ls" is a regular file
		return FAIL;
	}
	if (dentry.inode_idx != 5) { // "ls" contains inode index 5
		return FAIL;
	}

	// provide bad values 
	strncpy(dentry.file_name, test, 32);
	dentry.file_type = 7;
	dentry.inode_idx = 9;
	
	// find . directory (boot block index = 1)
	name = ".";
	if (read_dentry_by_index(1, &dentry) != 0) { // must return 0
		return FAIL;
	}
	if (strncmp(name, dentry.file_name, 32) != 0) { // must be "."
		return FAIL;
	}
	if (dentry.file_type != 1) { // "." is a directory
		return FAIL;
	}
	
	// find index outside (outside range [1, 17])
	if (read_dentry_by_index(18, &dentry) != -1) { // must return -1
		return FAIL;
	}

	// find index outside (outside range [1, 17])
	if (read_dentry_by_index(0, &dentry) != -1) { // must return -1
		return FAIL;
	}

	return PASS; // no failures
}

/* read_data_small_file_test
 * 		tests reading data on frame0.txt
 * 
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: prints frame0.txt to terminal
 * Coverage: read_dentry_by_name, read_data_small_file
 * Files: fsys.c
 */
int read_data_small_file_test() {
	//should print the fish frame0
	dentry_t dentry;
	char name[32] = "frame0.txt";
	char buf[187];

	read_dentry_by_name((uint8_t*)name, &dentry);

	if (read_data(dentry.inode_idx, 0, (uint8_t*)buf, 186) != 186) {
		return FAIL;
	}
	if (read_data(dentry.inode_idx, 0, (uint8_t*)buf, 187) != 0) {
		return FAIL;
	}

	if (PRINTING) {
		printf(buf);
		printf("\n\n");
		rtc_read(0, NULL, 0);
		rtc_read(0, NULL, 0);
	}
	

	return PASS;
}

/* read_data_exe_file_test
 * 		tests reading data on grep executable
 * 
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: prints beginning and ending 100 characters of grep to terminal
 * Coverage: read_dentry_by_name, read_data
 * Files: fsys.c
 */
int read_data_exe_file_test() {
	dentry_t dentry;
	char name[32] = "grep";
	char buf[100];

	read_dentry_by_name((uint8_t*)name, &dentry);

	if (read_data(dentry.inode_idx, 0, (uint8_t*)buf, 100) != 100) {
		return FAIL;
	}
	if (PRINTING) {
		printf(buf);
		printf("\n");
		rtc_read(0, NULL, 0);
		rtc_read(0, NULL, 0);
	}

	if (read_data(dentry.inode_idx, 6149-100, (uint8_t*)buf, 99) != 99) {
		return FAIL;
	}
	if (PRINTING) {
		printf(buf);
		printf("\n");
		rtc_read(0, NULL, 0);
		rtc_read(0, NULL, 0);
	}

	return PASS;
}

/* read_data_large_file_test
 * 		tests reading data on verylargetextwithverylongname.tx
 * 
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: prints contents of verylargetextwithverylongname.tx to terminal
 * Coverage: read_dentry_by_name, read_data
 * Files: fsys.c
 */
int read_data_large_file_test() {
	dentry_t dentry;
	unsigned char name[32] = "verylargetextwithverylongname.tx";
	char buf[5277];

	read_dentry_by_name(name, &dentry);

	if (read_data(dentry.inode_idx, 0, (uint8_t*)buf, 5276) != 5276) {
		return FAIL;
	}
	if (read_data(dentry.inode_idx, 0, (uint8_t*)buf, 5277) != 0) {
		return FAIL;
	}
	
	if (PRINTING) {
		printf(buf);
		printf("\n\n");
		rtc_read(0, NULL, 0);
		rtc_read(0, NULL, 0);
	}

	return PASS;
}

/* file_open_test
 * 		tests file open syscall, opens/closes verylargetextwithverylongname.tx
 * 
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage: file_open, file_close
 * Files: fsys.c
 */
int file_open_test() {
	char* name;

	// try to open a directory
	name = ".";
	if (file_open((uint8_t*)name) != -1) {
		return FAIL;
	}
	// try to open nonexistent file
	name = "i dont exist";
	if (file_open((uint8_t*)name) != -1) {
		return FAIL;
	}
	// try to open an existing file
	name = "verylargetextwithverylongname.tx";
	if (file_open((uint8_t*)name) != 0) {
		return FAIL;
	}
	// try to open another existing file
	name = "ls";
	if (file_open((uint8_t*)name) != -1) {
		return FAIL;
	}
	// try closing the file to complete this test
	if (file_close(0) != 0) {
		return FAIL;
	}

	return PASS;
}

/* file_close_test
 * 		tests file close syscall, opens/closes ls
 * 
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage: file_open, file_close
 * Files: fsys.c
 */
int file_close_test() {
	char* name;

	// try to close before opening
	if (file_close(0) != -1) {
		return FAIL;
	}

	// open, then close
	name = "ls";
	if (file_open((uint8_t*)name) != 0) {
		return FAIL;
	}
	if (file_close(0) != 0) {
		return FAIL;
	}

	// make sure its closed
	if (file_close(0) != -1) {
		return FAIL;
	}

	return PASS;
}

/* file_read_test
 * 		tests file read syscall, reads frame0.txt
 * 
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage: file_open, file_close, file_read
 * Files: fsys.c
 */
int file_read_test() {
	char* name;
	char buf[187];
	
	// try to reading before opening
	if (file_read(0, NULL, 0) != -1) {
		return FAIL;
	}

	// open, then read
	name = "frame0.txt";
	if (file_open((uint8_t*)name) != 0) {
		return FAIL;
	}
	// try reading a line
	if (file_read(0, buf, 25) != 25) {
		return FAIL;
	}
	name = "/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\\n";
	if (strncmp(name, buf, 25) != 0) {
		return FAIL;
	}
	if (PRINTING) {
		printf(buf);
		rtc_read(0, NULL, 0);
		rtc_read(0, NULL, 0);
	}

	// try reading another line
	if (file_read(0, buf, 11) != 11) {
		return FAIL;
	}
	name = "         o\n";
	if (strncmp(name, buf, 11) != 0) {
		return FAIL;
	}
	if (PRINTING) {
		printf(buf);
		rtc_read(0, NULL, 0);
		rtc_read(0, NULL, 0);
	}

	// try reading the rest of the file
	if (file_read(0, buf, 200) != 187-25-11) {
		return FAIL;
	}
	name = "           o    o\n       o\n             o\n        o     O\n    _    \\\n |\\/.\\   | \\/  /  /\n |=  _>   \\|   \\ /\n |/\\_/    |/   |/\n----------M----M--------";
	if (strncmp(name, buf, 187-25-11-1) != 0) {
		return FAIL;
	}
	if (PRINTING) {
		printf(buf);
		rtc_read(0, NULL, 0);
		rtc_read(0, NULL, 0);
	}

	// try reading past the end of the file
	if (file_read(0, buf, 10) != 0) {
		return FAIL;
	}

	// close file and complete test
	if (file_close(0) != 0) {
		return FAIL;
	}

	// make sure its closed
	if (file_read(0, NULL, 0) != -1) {
		return FAIL;
	}

	return PASS;
}

/* file_write_test
 * 		tests file write test syscall
 * 
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage: file_write
 * Files: fsys.c
 */
int file_write_test() {
	// try writing anything
	if (file_write(0, NULL, 0) != -1) {
		return FAIL;
	}

	return PASS;
}

/* directory_open_test
 * 		tests directory open test syscall, opens "."
 * 
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage: directory_open, directory_close
 * Files: fsys.c
 */
int directory_open_test() {
	char* name;

	// try to open a file
	name = "frame0.txt";
	if (directory_open((uint8_t*)name) != -1) {
		return FAIL;
	}
	// try to open nonexistent directory
	name = "i dont exist";
	if (directory_open((uint8_t*)name) != -1) {
		return FAIL;
	}
	// try to open an existing directory
	name = ".";
	if (directory_open((uint8_t*)name) != 0) {
		return FAIL;
	}
	// try to open another existing directory
	name = ".";
	if (directory_open((uint8_t*)name) != -1) {
		return FAIL;
	}
	// try closing the directory to complete this test
	if (directory_close(0) != 0) {
		return FAIL;
	}

	return PASS;
}

/* directory_close_test
 * 		tests directory close test syscall, opens "."
 * 
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage: directory_open, directory_close
 * Files: fsys.c
 */
int directory_close_test() {
	char* name;

	// try to close before opening
	if (directory_close(0) != -1) {
		return FAIL;
	}

	// open, then close
	name = ".";
	if (directory_open((uint8_t*)name) != 0) {
		return FAIL;
	}
	if (directory_close(0) != 0) {
		return FAIL;
	}

	// make sure its closed
	if (directory_close(0) != -1) {
		return FAIL;
	}

	return PASS;
}

/* directory_read_test
 * 		tests directory read test syscall, opens "."
 * 
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage: directory_open, directory_close, directory_read
 * Files: fsys.c
 */
int directory_read_test() {
	int i;
	char* name;
	char buf[33];

	for (i = 0; i < 33; i++) {
		buf[i] = 0x00; // zero the buffer for cleaniness
	}
	
	// try to reading before opening
	if (directory_read(0, NULL, 0) != -1) {
		return FAIL;
	}

	// open, then read
	name = ".";
	if (directory_open((uint8_t*)name) != 0) {
		return FAIL;
	}
	// try reading over 32 bytes from filename
	if (directory_read(0, buf, 33) != -1) {
		return FAIL;
	}
	// try reading first file
	if (directory_read(0, buf, 2) != 2) {
		return FAIL;
	}
	name = ".";
	if (strncmp(name, buf, 2) != 0) {
		return FAIL;
	}
	if (PRINTING) {
		printf(buf);
		printf("\n");
	}

	// try reading second file, read more characters than needed
	if (directory_read(0, buf, 11) != 11) {
		return FAIL;
	}
	name = "sigtest";
	if (strncmp(name, buf, 7) != 0) {
		return FAIL;
	}
	if (PRINTING) {
		printf(buf);
		printf("\n");
	}
	
	// try reading third file, read fewer characters than needed
	if (directory_read(0, buf, 5) != 5) {
		return FAIL;
	}
	name = "shell";
	if (strncmp(name, buf, 5) != 0) {
		return FAIL;
	}
	if (PRINTING) {
		printf(buf);
		printf("\n");
	}

	// try reading the rest of the directory
	for (i = 0; i < 17-3; i++) {
		if (directory_read(0, buf, 32) != 32) {
			return FAIL;
		}
		if (PRINTING) {
			printf(buf);
			printf("\n");
		}
	}

	// try reading past the end of the directory
	if (directory_read(0, buf, 10) != 0) {
		return FAIL;
	}

	// close directory
	if (directory_close(0) != 0) {
		return FAIL;
	}

	// make sure its closed
	if (directory_read(0, NULL, 0) != -1) {
		return FAIL;
	}

	if (PRINTING) {
		printf("\n");
		rtc_read(0, NULL, 0);
		rtc_read(0, NULL, 0);
	}

	return PASS;
}

/* directory_write_test
 * 		tests directory write test syscall
 * 
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage: directory_write
 * Files: fsys.c
 */
int directory_write_test() {
	// try writing anything
	if (directory_write(0, NULL, 0) != -1) {
		return FAIL;
	}

	return PASS;
}

/* rtc_demo_test
 * 		tests rtc
 * 
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: prints "$" character at RTC speed using rtc_read
 * Coverage: rtc_read, rtc_write, rtc_open
 * Files: rtc.c
 */
int rtc_demo_test() {
	uint32_t i = 0;
	uint32_t start = 0;
	uint32_t buf[1];
    buf[0] = 2;

	rtc_open(NULL);

	if (rtc_write(NULL, buf, 4)) { //initial speed
		return FAIL;
    }
	printf("2Hz");
	while(i < 10) {
		rtc_read(0x0000, NULL, 0x0000);
		printf("$");
		start++;
        if (start == 40) {
            if (rtc_write(NULL, buf, 4)) {
				return FAIL;
            }
            buf[0] *= 2;
			i++;
			printf("\n%dHz", buf[0]);
			start = 0;
		}
	}
	return PASS;
}

/* rtc_write_test
 * 		tests rtc
 * 
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: none
 * Coverage: rtc_write
 * Files: rtc.c
 */
int rtc_write_test() {
	TEST_HEADER;

	uint32_t buf[1];
	rtc_open(NULL);

	if (PRINTING) {
		printf("Writing bad RTC values\n");
	}
	
	buf[0] = 1025;
	if (rtc_write(NULL, buf, 4) != -1) {
		return FAIL;
	}

	buf[0] = 2048;
	if (rtc_write(NULL, buf, 4) != -1) {
		return FAIL;
	}

	buf[0] = 5;
	if (rtc_write(NULL, buf, 4) != -1) {
		return FAIL;
	}

	buf[0] = 2;
	if (rtc_write(NULL, buf, 4) != 0) {
		return FAIL;
	}

	return PASS;
}

/* Checkpoint 3 tests */
/* Checkpoint 4 tests */
/* Checkpoint 5 tests */


/* Test suite entry point */
void launch_tests() {
	TEST_OUTPUT("idt_test", idt_test());
	// launch your tests here
	/* checkpoint 1 */
	TEST_OUTPUT("idt_initialization_test", idt_initialization_test());
	
	// TEST_OUTPUT("divide_error_test", divide_error_test());
	// TEST_OUTPUT("breakpoint_test", breakpoint_test());
	// TEST_OUTPUT("invalid_opcode_test", invalid_opcode_test());
	// TEST_OUTPUT("general_protection_test", general_protection_test());
	// TEST_OUTPUT("general_interrupt_test", interrupt_tests(19));

	// TEST_OUTPUT("dereference_null_test", dereference_null_test());
	TEST_OUTPUT("dereference_inside_page_test", dereference_inside_page_test());
	// TEST_OUTPUT("dereference_outside_page_test", dereference_outside_page_test());

	/* checkpoint 2 */
	TEST_OUTPUT("rtc_write_test", rtc_write_test());
	TEST_OUTPUT("rtc_demo_test", rtc_demo_test());
	
	// rtc_open(NULL); // set RTC frequency to 2 Hz

	// TEST_OUTPUT("read_dentry_by_name_test", read_dentry_by_name_test());
	// TEST_OUTPUT("read_dentry_by_index_test", read_dentry_by_index_test());
	// TEST_OUTPUT("read_data_small_file_test", read_data_small_file_test());
	// TEST_OUTPUT("read_data_large_file_test", read_data_large_file_test());
	// TEST_OUTPUT("read_data_exe_file_test", read_data_exe_file_test());
	
	// TEST_OUTPUT("file_open_test", file_open_test());
	// TEST_OUTPUT("file_close_test", file_close_test());
	// TEST_OUTPUT("file_write_test", file_write_test());
	// TEST_OUTPUT("file_read_test", file_read_test());

	// TEST_OUTPUT("directory_open_test", directory_open_test());
	// TEST_OUTPUT("directory_close_test", directory_close_test());
	// TEST_OUTPUT("directory_write_test", directory_write_test());
	// TEST_OUTPUT("directory_read_test", directory_read_test());

	printf("Tests complete!\n");
}

