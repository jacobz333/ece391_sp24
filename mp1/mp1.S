# ----------------------
# Jacob Zheng - 02/05/2024
# ----------------------
.data					# section declaration
    # Useful offset constants for accessing members of a 
    # struct mp1_blink_struct structure
    LOCATION   = 0    
    ON_CHAR    = 2
    OFF_CHAR   = 3 
    ON_LENGTH  = 4
    OFF_LENGTH = 6
    COUNTDOWN  = 8
    STATUS     = 10
    NEXT       = 12

    STRUCT_SIZE = 16

    # shared constants
    NULLPTR = 0         # nullptr = 0x00000000
    INVALID = 0         # invalid = 0
    VALID = 1           # valid = 1
    MAX_LOC = 2000      # maximum location exclusive; 80*25=2000

    # ESP offsets for dispatcher
    cmd = 8
    # EBP offsets for functions
    arg = 8             # arg parameter shared between ioctls
    ptr = -4            # struct pointer; used in add and find
    # EBP offsets for sync
    loc2 = 8            # lower 2 bytes of arg
    loc1 = 10           # higher 2 bytes of arg
    struct1_ptr = -4    # ptr to struct with loc1
    struct2_ptr = -8    # ptr to struct with loc2



# Pointer to head of list (initialized to NULL)
mp1_list_head: .long   0

.text					# section declaration

# Export the function symbol names

.global mp1_rtc_tasklet
.global mp1_ioctl

# void mp1_poke(void);
#
# Interface: Register-based arguments (not C-style)
#    Inputs: %cl  - The byte you wish to write
#            %eax - Offset from the start of video memory that you wish
#                   to write to
#   Outputs: Text-mode video screen is written to at location %eax with
#            the byte in %cl
# Registers: Clobbers EDX
mp1_poke:
	movl    vmem_base_addr(,1),%edx
	movb    %cl,(%edx,%eax,1)
	ret



# void mp1_rtc_tasklet(void)
#   updates all linked list elements
# Inputs: 
#   None
# Outputs:
#   None
# Registers:
#   %eax - video memory offset to poke
#   %ecx - temp value, character to poke
#   %edx - current struct ptr, saved for mp1_poke
# Stack (EBP offset):
#   return address          | + 4
#   old EBP                 |   0
#   old EDX                 | - 4
#   old ECX                 | - 8
#   saved EDX               | -12
#
mp1_rtc_tasklet:
# callee setup
# ----------------------
    pushl   %ebp                    #
    movl    %esp, %ebp              #
    pushl   %edx                    #
    pushl   %ecx                    #
# function body
# ----------------------
    movl    mp1_list_head, %edx     # struct_ptr = head
_tasklet_loop:
    movl    %edx, %eax              # 
    pushl   %eax                    # 
    call    check_null              # check if struct_ptr is nullptr; check_null(struct_ptr)
    addl    $4, %esp                # discard check_null arguments
    cmpl    $VALID, %eax            # 
    jne     _tasklet_finish         # finish tasklet if struct_ptr == nullptr
    # decrement and check countdown
    decw    COUNTDOWN(%edx)         # (struct_ptr->countdown)--
    jnz     _countdown_nonzero      # jump if struct_ptr->countdown != 0
_countdown_zero:
    # check status
    cmpw    $0, STATUS(%edx)        #
    jne     _status1                # jump if struct_ptr->status != 0
_status0:
    movw    $1, STATUS(%edx)        # struct_ptr->status = 1
    movw    ON_LENGTH(%edx), %cx    # 
    movw    %cx, COUNTDOWN(%edx)    # struct_ptr->countdown = struct_ptr->on_length
    movb    ON_CHAR(%edx), %cl      # 
    jmp     _tasklet_poke           # 
_status1:
    movw    $0, STATUS(%edx)        # struct_ptr->status = 0
    movw    OFF_LENGTH(%edx), %cx   # 
    movw    %cx, COUNTDOWN(%edx)    # struct_ptr->countdown = struct_ptr->off_length
    movb    OFF_CHAR(%edx), %cl     # 
_tasklet_poke:
    # mp1_poke
    pushl   %edx                    # 
    movzwl  LOCATION(%edx), %eax    #
    shll    $1, %eax                #   EAX <- 2*((unsigned long) struct_ptr->location)
    call    mp1_poke                # mp1_poke()
    popl    %edx                    # 
_countdown_nonzero:
    # increment ptr
    movl    NEXT(%edx), %edx        # struct_ptr = struct_ptr->next
    jmp     _tasklet_loop           # 
_tasklet_finish:
# callee teardown
# ----------------------
    popl    %ecx                    #
    popl    %edx                    #
    leave                           #
	ret                             #



# int mp1_ioctl(unsigned long arg, unsigned long cmd)
#   ioctl dispatcher
# Inputs: 
#   unsigned long arg - ioctl argument
#   unsigned long cmd - ioctl opcode
# Outputs:
#   0 on success, -1 on failure
# Registers:
#   %eax - Return Value, temp value
# Stack (ESP offset):
#   unsigned long cmd       | + 8
#   unsigned long arg       | + 4
#   return address          |   0
# 
mp1_ioctl:
    cmpl    $0, cmd(%esp)           #
    jl      _dispatcher_invalid     # return -1 if cmd < 0
    cmpl    $4, cmd(%esp)           #
    jge     _dispatcher_invalid     # return -1 if cmd >= 4
    movl    cmd(%esp), %eax         # 
    jmp     *jump_table(,%eax,4)    # jump to jump_table[cmd]
_dispatcher_invalid:
    movl    $-1, %eax               # store -1 return value
    ret                             #

jump_table: .long mp1_ioctl_add, mp1_ioctl_remove, mp1_ioctl_find, mp1_ioctl_sync



# int mp1_ioctl_add(unsigned long arg);
#   adds the input struct to the start of the linked list
# Inputs: 
#   unsigned long arg - userspace struct pointer
# Outputs:
#   0 on success, -1 on failure
# Registers:
#   %eax - Return Value, temp value
#   %ecx - temp value
#   %edx - saved for mp1_poke
# Stack (EBP offset):
#   unsigned long arg       | + 8
#   return address          | + 4
#   old EBP                 |   0
#   mp1_blink_struct* ptr   | - 4    malloc'ed pointer
#   old ECX                 | - 8
#   saved EDX               | -12
# 
mp1_ioctl_add:
# callee setup
# ----------------------
    pushl   %ebp                    #
    movl    %esp, %ebp              #
# function body
# ----------------------
    # check valid struct pointer
    movl    arg(%ebp), %eax         #
    pushl   %eax                    #
    call    check_null              # check if struct pointer is valid; check_null(arg)
    addl    $4, %esp                # discard check_null arguments
    cmpl    $VALID, %eax            #
    jne     return_neg1             # return -1 if invalid struct pointer

    # malloc
    pushl   $STRUCT_SIZE            # 
    call    mp1_malloc              # malloc(STRUCT_SIZE);
    movl    %eax, ptr(%ebp)         # store malloc ptr to the stack
    # check valid malloc pointer
    pushl   %eax                    # 
    call    check_null              # check if malloc pointer is not null; check_null(ptr)
    addl    $4, %esp                # discard check_null arguments
    cmpl    $VALID, %eax            #
    je      _valid_malloc           # jump if ptr != nullptr
_invalid_malloc:
    addl    $4, %esp                # discard ptr
    jmp     return_neg1             #
_valid_malloc:
    # copy from user to kernel
    pushl   $STRUCT_SIZE            #
    pushl   arg(%ebp)               #
    pushl   ptr(%ebp)               #
    call    mp1_copy_from_user      # mp1_copy_from_user(ptr, arg, STRUCT_SIZE)
    addl    $12, %esp               # discard mp1_copy_from_user arguments
    # check successful copy
    cmpl    $0, %eax                #
    je      _add_valid_copy         # jump if copy successful (EAX == 0)
_add_invalid_copy:
    call    mp1_free                # mp1_free(ptr)
    addl    $4, %esp                # discard ptr
    jmp     return_neg1             # return -1
_add_valid_copy:
    # check valid location
    movl    ptr(%ebp), %eax         # 
    movw    LOCATION(%eax), %ax     # 
    pushw   %ax                     # 
    call    check_loc_inrange       # check if struct location is in range; check_loc_inrange(ptr->location)
    addl    $2, %esp                # discard check_loc_inrange arguments
    cmpl    $VALID, %eax            #
    je      _add_valid_loc          # jump if valid location
_add_invalid_loc:
    call    mp1_free                # mp1_free(ptr)
    addl    $4, %esp                # discard ptr
    jmp     return_neg1             #
_add_valid_loc:
    # setup struct
    pushl   %ecx                    # save ECX
    movl    ptr(%ebp), %eax         # 
    movw    ON_LENGTH(%eax), %cx    # 
    movw    %cx, COUNTDOWN(%eax)    # ptr->countdown = ptr->on_length
    movw    $1, STATUS(%eax)        # ptr->status = 1
    # setup list head
    movl    mp1_list_head, %ecx     #   
    movl    %ecx, NEXT(%eax)        # ptr->next = head
    movl    ptr(%ebp), %ecx         #   
    movl    %ecx, mp1_list_head     # head = ptr
    # mp1 poke
    pushl   %edx                    #
    movb    ON_CHAR(%eax), %cl      # 
    movzwl  LOCATION(%eax), %eax    #
    shll    $1, %eax                #   EAX <- 2*((unsigned long) ptr->location)
    call    mp1_poke                # mp1_poke()
    popl    %edx                    # 
    popl    %ecx                    # restore ECX
# callee teardown
# ----------------------
    addl    $4, %esp                # discard ptr
    jmp     return_0                #



# int mp1_ioctl_remove(unsigned long arg)
#   removes an existing struct given input location
# Inputs: 
#   unsigned long arg - location to remove
# Outputs:
#   0 on success, -1 on failure
# Registers:
#   %eax - Return Value, Temp Value, &(prev_struct_ptr->next)
#   %edx - curr_struct_ptr
#   %ecx - curr_struct_ptr->next
# Stack (EBP offset):
#   unsigned long arg           | + 8
#   return address              | + 4
#   old EBP                     |   0
#   temp EAX / old EDX          | - 4
#   old ECX                     | - 8
# Remove translates to this C-style code:
#   curr_struct_ptr = prev_struct_ptr->next;        definition
#   prev_struct_ptr->next = curr_struct_ptr->next;  repoint pointer to skip curr_struct
#   mp1_free(curr_struct_ptr);                      free curr_struct from dynamic memory
# 
mp1_ioctl_remove:
# callee setup
# ----------------------
    pushl   %ebp                    #
    movl    %esp, %ebp              #
# function body
# ----------------------
    # check valid location
    movw    arg(%ebp), %ax          #
    pushw   %ax                     #
    call    check_loc_inrange       # check if struct location is in range; check_loc_inrange(unsigned short arg)
    addl    $2, %esp                # discard check_loc_inrange arguments
    cmpl    $VALID, %eax            #
    jne     return_neg1             # return -1 if invalid input location
    # try to find location with find_helper
    movw    arg(%ebp), %ax          #
    pushw   %ax                     #
    call    find_helper             # find_helper(unsigned short arg); EAX <- &(prev_struct_ptr->next)
    addl    $2, %esp                # discard find_helper arguments
    # check if find_helper found location
    pushl   %eax                    # save EAX = &(prev_struct_ptr->next)
    movl    (%eax), %eax            #
    pushl   %eax                    #
    call    check_null              # check if struct pointer is valid; check_null(curr_struct_ptr)
    addl    $4, %esp                # discard check_null arguments
    cmpl    $VALID, %eax            #
    je      _remove_found           # jump if location was found
_remove_not_found:
    popl    %eax                    # restore EAX = &(prev_struct_ptr->next)
    jmp     return_neg1             #
_remove_found:
    popl    %eax                    # restore EAX = &(prev_struct_ptr->next)
    # update linked list
    pushl   %edx                    #
    pushl   %ecx                    #
    movl    (%eax), %edx            # 
    movl    NEXT(%edx), %ecx        # 
    movl    %ecx, (%eax)            # prev_struct_ptr->next = curr_struct_ptr->next
    # free struct pointer
    pushl   %edx                    # 
    call    mp1_free                # mp1_free(curr_struct_ptr)
    addl    $4, %esp                # discard mp1_free argument
# callee teardown
# ----------------------
    popl    %ecx                    #
    popl    %edx                    #
    jmp     return_0                #



# int mp1_ioctl_find(unsigned long arg)
#   searches the linked list for an existing struct whose location matches input struct's location
#   copies existing struct to input struct
# Inputs: 
#   unsigned long arg - userspace struct pointer
# Outputs:
#   0 on success, -1 on failure
# Registers:
#   eax - Return Value, temp value
# 
# Stack (EBP offset):   
#   unsigned long arg           | + 8
#   return address              | + 4
#   old EBP                     |   0
#   malloc_ptr                  | - 4
#   temp EAX                    | - 8
#
mp1_ioctl_find:
# callee setup
# ----------------------
    pushl   %ebp                    #
    movl    %esp, %ebp              #
# function body
# ----------------------
    # check valid struct pointer
    movl    arg(%ebp), %eax         #
    pushl   %eax                    #
    call    check_null              # check if struct pointer is valid; check_null(arg)
    addl    $4, %esp                # discard check_null arguments
    cmpl    $VALID, %eax            #
    jne     return_neg1             # return -1 if invalid input struct pointer
    # malloc space to copy to
    pushl   $STRUCT_SIZE            # 
    call    mp1_malloc              # malloc(STRUCT_SIZE);
    movl    %eax, ptr(%ebp)         # store malloc ptr to the stack
    # verify malloc pointer
    pushl   %eax                    # 
    call    check_null              # check if malloc pointer is not null; check_null(ptr)
    addl    $4, %esp                # discard check_null arguments
    cmpl    $VALID, %eax            #
    je      _find_valid_malloc      # jump if ptr != nullptr
_find_invalid_malloc:
    addl    $4, %esp                # discard ptr
    jmp     return_neg1             #
_find_valid_malloc:
    # copy from user to kernel
    pushl   $STRUCT_SIZE            #
    pushl   arg(%ebp)               #
    pushl   ptr(%ebp)               #
    call    mp1_copy_from_user      # mp1_copy_from_user(ptr, arg, STRUCT_SIZE)
    addl    $12, %esp               # discard mp1_copy_from_user arguments
    # verify successful copy
    cmpl    $0, %eax                #
    je      _find_valid_copy_user   # jump if copy successful (EAX == 0)
_find_invalid_copy_user:
    call    mp1_free                # mp1_free(ptr)
    addl    $4, %esp                # discard ptr
    jmp     return_neg1             #
_find_valid_copy_user:
    # check valid location
    movl    ptr(%ebp), %eax         #
    movw    LOCATION(%eax), %ax     #
    pushw   %ax                     #
    call    check_loc_inrange       # check if struct location is in range; check_loc_inrange(ptr->location)
    addl    $2, %esp                # discard check_loc_inrange arguments
    cmpl    $VALID, %eax            #
    je      _find_valid_loc         # jump if valid location
_find_invalid_loc:
    call    mp1_free                # mp1_free(ptr)
    addl    $4, %esp                # discard ptr
    jmp     return_neg1             #
_find_valid_loc:
    # call find_helper
    movl    ptr(%ebp), %eax         #
    movw    LOCATION(%eax), %ax     #
    pushw   %ax                     #
    call    find_helper             # find_helper(ptr->location); EAX <- &(prev_struct_ptr->next)
    addl    $2, %esp                # discard find_helper arguments
    # check find_helper found valid struct ptr
    movl    (%eax), %eax            #   EAX <- *(&(prev_struct_ptr->next)) = curr_struct_ptr
    pushl   %eax                    # save EAX = curr_struct_ptr
    pushl   %eax                    # 
    call    check_null              # check if struct pointer is valid; check_null(curr_struct_ptr)
    addl    $4, %esp                # discard check_null arguments
    cmpl    $VALID, %eax            #
    je      _find_found             # jump if curr_struct_ptr is valid
_find_not_found:
    popl    %eax                    # restore EAX = curr_struct_ptr
    call    mp1_free                # mp1_free(ptr)
    addl    $4, %esp                # discard ptr
    jmp     return_neg1             #
_find_found:
    popl    %eax                    # restore EAX = curr_struct_ptr
    # copy from kernel to user  
    pushl   $STRUCT_SIZE            #
    pushl   %eax                    #
    pushl   arg(%ebp)               #
    call    mp1_copy_to_user        # mp1_copy_to_user(arg, curr_struct_ptr, STRUCT_SIZE)
    addl    $12, %esp               # discard mp1_copy_to_user arguments
    # check successful copy
    cmpl    $0, %eax                #
    je      _find_valid_copy_kernel # jump if copy successful (EAX == 0)
_find_invalid_copy_kernel:  
    call    mp1_free                # mp1_free(ptr)
    addl    $4, %esp                # discard ptr
    jmp     return_neg1             #
_find_valid_copy_kernel:
    call    mp1_free                # mp1_free(ptr)
    addl    $4, %esp                # discard ptr
    jmp     return_0                #



# int mp1_ioctl_sync(unsigned long arg)
#   synchronizes the struct at loc2 to the struct at loc1
# Inputs: 
#   unsigned long arg - top 2 bytes = loc1, bottom 2 bytes = loc2, loc1 and loc2 are two locations
# Outputs:
#   0 on success, -1 on failure
# Registers:
#   %eax - Return Value, temp value, struct1_ptr
#   %edx - struct2_ptr
#   %ecx - temp value
# Stack (EBP offset):
#   unsigned short loc1         | +10
#   unsigned short loc2         | + 8
#   return address              | + 4
#   old EBP                     |   0
#   struct1_ptr                 | - 4
#   struct2_ptr                 | - 8
#   old EDX                     | -12
#   old ECX                     | -16
# 
mp1_ioctl_sync:
# callee setup
# ----------------------
    pushl   %ebp                    #
    movl    %esp, %ebp              #
# function body
# ----------------------
    # try to find loc1
    movw    loc1(%ebp), %ax         #
    pushw   %ax                     #
    call    find_helper             # find_helper(loc1); EAX <- &struct1_ptr
    addl    $2, %esp                # discard find_helper arguments
    # check find_helper found loc1
    movl    (%eax), %eax            #
    pushl   %eax                    # save struct1_ptr
    pushl   %eax                    # 
    call    check_null              # check if struct1 is valid; check_null(struct1_ptr)
    addl    $4, %esp                # discard check_null arguments
    cmpl    $VALID, %eax            #
    je      _sync_loc1_found        # jump if struct1 was found (nullptr means not found)
_sync_loc1_not_found:
    addl    $4, %esp                # discard struct1_ptr
    jmp     return_neg1             #
_sync_loc1_found:
    # try to find loc2
    movw    loc2(%ebp), %ax         #
    pushw   %ax                     #
    call    find_helper             # find_helper(loc2); EAX <- &struct2_ptr
    addl    $2, %esp                # discard find_helper arguments
    # check find_helper found loc2
    movl    (%eax), %eax            #
    pushl   %eax                    # save struct2_ptr
    pushl   %eax                    #
    call    check_null              # check if struct2_ptr is valid; check_null(struct2_ptr)
    addl    $4, %esp                # discard check_null arguments
    cmpl    $VALID, %eax            #
    je      _sync_loc2_found        # jump if struct2 was found (nullptr means not found)
_sync_loc2_not_found:
    addl    $8, %esp                # discard struct1_ptr, struct2_ptr
    jmp     return_neg1             #
_sync_loc2_found:
    pushl   %edx                    #
    pushl   %ecx                    #
    # copy timing information
    movl    struct1_ptr(%ebp), %eax #
    movl    struct2_ptr(%ebp), %edx #
    movw    ON_LENGTH(%eax), %cx    # 
    movw    %cx, ON_LENGTH(%edx)    # struct2_ptr->on_length = struct1_ptr->on_length
    movw    OFF_LENGTH(%eax), %cx   # 
    movw    %cx, OFF_LENGTH(%edx)   # struct2_ptr->off_length = struct1_ptr->off_length
    movw    COUNTDOWN(%eax), %cx    # 
    movw    %cx, COUNTDOWN(%edx)    # struct2_ptr->countdown = struct1_ptr->countdown
    movw    STATUS(%eax), %cx       # 
    movw    %cx, STATUS(%edx)       # struct2_ptr->status = struct1_ptr->status
    # determine char to poke with
    cmpw    $0, %cx                 # 
    jne     _sync_status1           # jump if struct2_ptr->status != 0
_sync_status0:
    movb    OFF_CHAR(%edx), %cl     # CL <- struct2_ptr->off_char
    jmp     _sync_poke_loc2         # jump to poke loc2
_sync_status1:    
    movb    ON_CHAR(%edx), %cl      # CL <- struct2_ptr->on_char
_sync_poke_loc2:
    # poke struct at loc2
    movzwl  LOCATION(%edx), %eax    #
    shll    $1, %eax                #   EAX <- 2*((unsigned long) struct2_ptr->location)
    call    mp1_poke                # mp1_poke()
# callee teardown
# ----------------------
    popl    %ecx                    #
    popl    %edx                    #
    addl    $8, %esp                # discard struct1_ptr, struct2_ptr
    jmp     return_0                #





# helper routine
#   returns -1; jump here on ioctl core function fail
#   does not restore callee stored registers
return_neg1:
    movl    $-1, %eax               #
    leave                           #
    ret                             #

# helper routine
#   returns 0; jump here on ioctl core function success
#   does not restore callee stored registers
return_0:
    movl   $0, %eax                 #
    leave                           #
    ret                             #





# bool check_null(void* ptr);
#   checks if input ptr is a valid pointer
# Inputs:
#   void* ptr - pointer to check against null
# Outputs:
#   returns !(ptr == nullptr)
# Registers:
#   %eax - return value, pointer to check
# Stack (EBP offset):
#   void* ptr                   | + 8
#   return address              | + 4
#   old EBP                     |   0
#
check_null:
# callee setup
# ----------------------
    pushl   %ebp                    #
    movl    %esp, %ebp              #
    movl    8(%ebp), %eax           # EAX <- ptr
# function body
# ----------------------
    cmpl    $NULLPTR, %eax          #
    je      _invalid_ptr            # fail if ptr == nullptr
_valid_ptr:
    movl    $VALID, %eax            #
    jmp     _null_finish            # 
_invalid_ptr:
    movl    $INVALID, %eax          #
_null_finish:
# callee teardown
# ----------------------
    leave                           #
    ret                             #



# bool check_loc_inrange((unsigned short) loc);
#   checks if loc is a location in range
# Inputs:
#   unsigned short loc - location to check
# Outputs:
#   returns !(MAX_LOC <= loc)
# Registers:
#   %eax - return value, %ax=location to check
# Stack (EBP offset):
#   unsigned short loc          | + 8
#   return address              | + 4
#   old EBP                     |   0
# 
check_loc_inrange:
# callee setup
# ----------------------
    pushl   %ebp                    #
    movl    %esp, %ebp              #
    movw    8(%ebp), %ax            # AX <- loc
# function body
# ----------------------
    cmpw    $MAX_LOC, %ax           #
    jae     _invalid_loc            # fail if loc >= MAX_LOC
_valid_loc:
    movl    $VALID, %eax            #
    jmp     _loc_finish             #
_invalid_loc:
    movl    $INVALID, %eax          #
_loc_finish:
# callee teardown
# ----------------------
    leave                           #
    ret                             #



# mp1_blink_struct** find_helper((unsigned short) loc);
#   traverses thru the list to find a struct whose location matches loc
# Inputs:
#   unsigned short loc - location to find
# Outputs:
#   returns address of struct ptr, nullptr if not found
# Registers:
#   %eax - return value, temp value
#   %cx  - location to find
#   %edx - address of curr_struct_ptr
# Stack (EBP offset):
#   unsigned short loc          | + 8
#   return address              | + 4
#   old EBP                     |   0
#   old ECX                     | - 4
#   old EDX                     | - 8
# 
# the function roughly searches using a loop in this code:
#   CX = location to check                                  definition
#   EDX = &curr_struct_ptr                                  definition
#   while(curr_struct_ptr != nullptr) {                     check if we've reached the end of the list
#       #if (curr_struct_ptr->location == CX) {              check location
#           return EDX (= &curr_struct_ptr)                 returns address of struct pointer holding found location
#       }
#       EDX = &(curr_struct_ptr->next)                      increment
#   }
#   return EDX (= nullptr)                                  only reaches here when location could not be found
#
find_helper:
# callee setup
# ----------------------
    pushl   %ebp                    #
    movl    %esp, %ebp              #
    pushl   %ecx                    #
    pushl   %edx                    #
    movw    8(%ebp), %ax            # AX <- loc
# function body
# ----------------------
    movl    $mp1_list_head, %edx    # EDX <- &head
    movw    %ax, %cx                # CX <- location
_find_helper_loop:
    # check that curr_struct_ptr is not nullptr
    movl    (%edx), %eax            #
    pushl   %eax                    #
    call    check_null              # check if curr_struct_ptr is not null; check_null(curr_struct_ptr)
    addl    $4, %esp                # discard check_null arguments
    cmpl    $VALID, %eax            #
    movl    %edx, %eax              # store EAX <- &curr_struct_ptr return value
    jne     _find_helper_finish     # jump if curr_struct_ptr == nullptr (end of linked list)
    # check that curr_struct_ptr->location is equal to location
    movl    (%edx), %eax            #
    cmpw    LOCATION(%eax), %cx     #
    jne     _find_helper_not_found  # jump if curr_struct_ptr->location != location
_find_helper_found:
    movl    %edx, %eax              #
    jmp     _find_helper_finish     # return &curr_struct_ptr
_find_helper_not_found:
    # increment EDX = &curr_struct_ptr
    addl    $NEXT, %eax             #   EAX <- &(curr_struct_ptr->next)
    movl    %eax, %edx              # &curr_struct_ptr = &(curr_struct_ptr->next)
    jmp     _find_helper_loop       # 
_find_helper_finish:
# callee teardown
# ----------------------
    popl    %edx                    #
    popl    %ecx                    #
    leave                           #
    ret                             #



.end
