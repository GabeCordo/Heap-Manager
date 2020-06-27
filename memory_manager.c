#include <stdio.h>
#include <memory.h>
#include <unistd.h>
#include "memory_manager.h"

static vm_page_for_families_t *first_vm_page_for_families = NULL; //point to the most recent vm family page
static size_t SYSTEM_PAGE_SIZE = 0; //stores the size of the virtual memory page

void init() {
	SYSTEM_PAGE_SIZE = getpagesize(); //defined in unistd.h
}

/*
 * Function:  mm_get_new_vm_page_from_kernal
 * -----------------------------------------
 * requests a new virtual memory page from the kernal
 *
 *  units: the number of vm pages
 *
 *  returns: the pointer to the begining of the virtual
 *			 memory page received from the kernal
 *	error: the function will return NULL if the req fails
 */
static void *mm_get_new_vm_page_from_kernal(int units) {
	
	char *vm_page = mmap(
		0, //null
		units * SYSTEM_PAGE_SIZE, //the amount of memory we needed (n pages * page-size)
		PROT_READ|PROT_WRITE|PROT_EXEC, //the permissions on the virtual memory pages
		MAP_ANON|MAP_PRIVATE,
		0, //null
		0 //null
	);
	if (vm_page == MAP_FAILED)
		return NULL; //if the kernal request fails, we will return null
	memset(vm_page, 0, units * SYSTEM_PAGE_SIZE);
	return (void *)vm_page;
}

/*
 * Function:  mm_return_vm_page_to_kernel
 * --------------------------------------
 * returns the virtual memory pages back to the kernal
 *
 *  *vm_page: the pointer to the vm page start
 *	units: the number of vm pages
 *
 *	error: the function will return NULL if the req fails
 */
static void *mm_return_vm_page_to_kernel(void *vm_page, int units) {
	
	//munmap will return a boolean, so if it fails the condition will be triggered
	if(munmap(vm_page, units * SYSTEM_PAGE_SIZE))
		return NULL;
}

/*
 * Function:  mm_instance_new_page_family
 * --------------------------------------
 * allocates a new page-family registration for our
 * new dynamic structure, if there is not enough room
 * on the current vm page, we request for the kernal 
 * to give us a new one
 *
 *  *struct_name: array of chars for the struct-name
 *	struct_size: size of bytes the structures takes up
 *
 *	returns: nothing
 *  errors: on erroring, the system will be crashed
 *	paramaters: the struct_name must not be used twice
 */
void mm_instance_new_page_family(char *struct_name, uint32_t struct_size) {
	
	vm_page_for_family_t *vm_page_family_curr = NULL;
	vm_page_for_families_t *new_vm_page_for_families = NULL;
	
	//if the struct_size > maximum bits per page size, we cannot support vm storage of the struct
	if (struct_size > SYSTEM_PAGE_SIZE) {
		return; //aka error out
	}
	
	//if we do not have a vm page currently, we need to request one from the kernal
	if (!first_vm_page_for_families) {
		first_vm_page_for_families = (vm_page_for_families_t *)mm_get_new_vm_page_from_kernal(1);
		first_vm_page_for_families->next = NULL; //there are no families after the first family, since we are creating the first ref
		strncpy(first_vm_page_for_families->vm_page_family[0].struct_name, struct_name, MM_MAX_STRUCT_NAME); //we use offset 0 because it is the first family
		first_vm_page_for_families->vm_page_family[0].struct_size = struct_size;
		return; //close the function
	}
	
	uint32_t count = 0;
	
	ITERATE_PAGE_FAMILIES_BEGIN(first_vm_page_for_families, vm_page_family_curr) {
		
		if(strncmp(vm_page_family_curr->struct_name, struct_name, MM_MAX_STRUCT_NAME) != 0) {
			count++;
			continue;
		}
		
		assert(0); //we cannot allocate two structures with the same name CRASH IT BEFORE IT BURNS
		
	} ITERATE_PAGE_FAMILIES_END(first_vm_page_for_families, vm_page_family_curr);
	
	//check to see if the old page-family can acomidate a new structure
	if (count == MAX_FAMILIES_PER_VM_PAGE) {
		
		//if not we need to request a new page to write data to from the kernal
		new_vm_page_for_families = (vm_page_for_families_t *)mm_get_new_vm_page_from_kernal(1);
		new_vm_page_for_families->next = first_vm_page_for_families; //point to the old family-page
		first_vm_page_for_families = new_vm_page_for_families; //update the global variable to point to the most-recent page
		
		vm_page_family_curr = &first_vm_page_for_families->vm_page_family[0]; //set vm_page_cur to == the bottom struct of the new page
		
	}
	
	//there is room on the current page, no need to request a new vm page
	strncpy(vm_page_family_curr->struct_name, struct_name, MM_MAX_STRUCT_NAME);
	vm_page_family_curr->struct_size = struct_size;
	vm_page_family_curr->first_page = NULL;
}

/*
 * Function:  mm_print_registered_page_families
 * --------------------------------------------
 * print out all the page-family registrations found
 * within the virtual memory pages on the lmm
 *
 */
void mm_print_registered_page_families() {
	
	vm_page_for_family_t *vm_page_family_curr = NULL;
	vm_page_for_families_t *new_vm_page_for_families_curr = NULL;
	
	for(new_vm_page_for_families_curr = first_vm_page_for_families;
		first_vm_page_for_families_curr; //iterate over each pahe till next is a NULL value
		new_vm_page_for_families_curr = new_vm_page_for_families_curr->next) {
			
		ITERATE_PAGE_FAMILIES_BEGIN(new_vm_page_for_families_curr, vm_page_family_curr) {
				
			printf("Page Family:%s, Size = %u\n", 
				vm_page_family_curr->struct_name,
				vm_page_family_curr->struct_size);
				
		} ITERATE_PAGE_FAMILIES_END(new_vm_page_for_families_curr, vm_page_family_curr);
	}
}

/*
 * Function:  lookup_page_family_by_name
 * -------------------------------------
 * iterates over every page for families starting with
 * the one most-recently created by the application
 * untill the page_family is matched by name
 *
 *  *struct_name: an array of characters representing 
 *				  the page-family name we arte trying
 *				  to find
 *
 *	returns: the pointer to the page-family that matches
 *			 the name provided as an argument
 */
vm_page_family_t *lookup_page_family_by_name(char *struct_name) {
	
	vm_page_for_family_t *vm_page_family_curr = NULL;
	vm_page_for_families_t *new_vm_page_for_families_curr = NULL;
		
	for(new_vm_page_for_families_curr = first_vm_page_for_families;
		first_vm_page_for_families_curr; //iterate over each pahe till next is a NULL value
		new_vm_page_for_families_curr = new_vm_page_for_families_curr->next) {
				
		ITERATE_PAGE_FAMILIES_BEGIN(new_vm_page_for_families_curr, vm_page_family_curr) {
					
			if(strncpy(vm_page_family_curr->struct_name, struct_name, MM_MAX_STRUCT_NAME) == 0) {
				return vm_page_family_curr;
			}
					
		} ITERATE_PAGE_FAMILIES_END(new_vm_page_for_families_curr, vm_page_family_curr);		
	}
	
	return NULL;
}

/*
 * Function:  mm_union_free_blocks
 * -------------------------------
 * merge two meta-blocks and there data-blocks together
 * in order to form one meta-block/data-block that if
 * the two are CONSECUTIVLY FREE
 *
 *  *first: the meta-block belonging to a lower memory
 *			address on the vm page for families
 *  *second: the meta-block belonging to a higher memory
 * 			 address on the vm page for families
 *
 *	returns: nothing
 */
static void mm_union_free_blocks(block_meta_data_t *first, block_meta_data_t *second) {
	
	//we only want to perform a merge if both values are currently free
	assert(first->is_free == MM_TRUE && second->is_free == MM_TRUE);
	
	first->block_size += sizeof(block_meta_data_t) + second->block_size;
	first->next_block = second->next_block;
	
	//only update the prev pointer of the ante-penultimate if it is NOT NULL
	if (second->next_block)
		second->next_block->prev_block = first;
}

/*
 * Function:  mm_is_vm_page_empty
 * -------------------------------
 * merge two meta-blocks and there data-blocks together
 * in order to form one meta-block/data-block that if
 * the two are CONSECUTIVLY FREE
 *
 *  *vm_page: a pointer to the vm data page (NOT FAMILY)
 *
 *	returns: MM_TRUE if the vm page given as a function
 *			 argument is complelty free else it returns
 * 			 MM_FALSE (type vm_bool_t)
 */
vm_bool_t *mm_is_vm_page_empty(vm_page_t *vm_page) {
	
	if (vm_page_t_ptr->_next_block == NULL &&
		vm_page_t_ptr->_prev_block == NULL &&
		vm_page_t_ptr->is_free == MM_TRUE) {
		
		return MM_TRUE;
	}
	return MM_FALSE;
}

/*
 * Function:  mm_max_page_allocatable_memory
 * -----------------------------------------
 * determines the maximum allocatable memory (bytes)
 * available to n vm-pages, taking into acount the bytes
 * needed to store the meta-data of the page-memory
 *
 *  units: an integer indicating the number of vm pages
 *
 *	returns: a uint32 representing the number of bytes that
 *			 can be allocated by another process
 */
static inline uint32_t mm_max_page_allocatable_memory(int units) {
	
	return (uint32_t)((SYSTEM_PAGE_SIZE * units) - offset_of(vm_page_t, page_memory));
}

/*
 * Function:  allocate_vm_page
 * ---------------------------
 * allocates a new vm page for data-storage. It will be appended to
 * the front of the linked list connecting all data vm-pages
 *
 *  *vm_page_family: the pointer to the vm_page_family the vm data
 *					 pointer will belong to
 *
 *	returns: a newely allocated virtual page with a configured
 *			 page-memory segment for pf_family, next, prev pointers
 */
vm_page_t * allocate_vm_page(vm_page_family_t *vm_page_family) {
	
	vm_page_t *vm_page = mm_get_new_vm_page_from_kernal(1); //get a new vm-page from the kernal
	MARK_VM_PAGE_EMPTY(vm_page); //initialize the vm-page to have a meta-block attached
	
	vm_page->block_meta_data.block_size = mm_max_page_allocatable_memory(1);
	vm_page->block_meta_data.offset = offset_of(vm_page_t, block_meta_data);
	
	vm_page->next = NULL;
	vm_page->prev = NULL;
	vm_page->pg_family = vm_page_family; //the pointer must direct to the vm_page_family argument
	
	//check to see if the new vm data page is the first in the vm-page-family
	if(!vm_page_family->first_page) {
		vm_page_family->first_page = vm_page;
		return vm_page;
	}
	
	//place the new vm data page at the head of the ll
	vm_page->next = vm_page_family->first_page; //point to the old first-vmpage of the ll
	vm_page_family->first_page = vm_page;
	return vm_page;
}

/*
 * Function:  mm_vm_page_delete_and_free
 * -------------------------------------
 * delete's the vm_page passed as an argument from the linked list
 * of vm data pages belonging to a page_family
 *
 *  *vm_page: the pointer to the vm data page that was connected to
 * 			  the vm_family_page
 *
 *	returns: nothing
 */
void mm_vm_page_delete_and_free(vm_page_t *vm_page) {
	
	vm_page_family_t *vm_page_family = vm_page->pg_family;
	
	//check to see if the vm-page is at the front of the linked list
	if(vm_page_family->first_page == vm_page) {
		vm_page_family->first_page = vm_page->next;
		if(vm_page->next)
			vm_page->next->prev = NULL;
		vm_page->next = NULL;
		vm_page->prev = NULL;
		mm_return_vm_page_to_kernel((void *)vm_page, 1);
		return;
	}
	
	//check to see if the vm-page is in the middle of the linked list
	if(vm_page->next) //check to see if we get a null value (if we do, it's the end of the ll)
		vm_page->next->prev = vm_page->prev;
	vm_page->prev->next = vm_page->next;
	mm_return_vm_page_to_kernel((void *)vm_page, 1);
}