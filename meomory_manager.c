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