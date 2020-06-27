#ifndef HEADER_FILE
#define HEADER_FILE

	/* type declarations */

	typedef struct vm_page_family_ {
		
		char struct_name [MM_MAX_STRUCT_NAME];
		uint32_t struct_size;
			
	} vm_page_family_t;

	typedef struct vm_page_families_ {
			
		struct vm_page_for_families_ *next; //pointer to the next vm family page
		vm_page_family_t vm_page_family[0]; //pointer to the array of page families
			
	} vm_page_for_families_t;
		
	/* function declarations */

	static void *mm_get_new_vm_page_from_kernal(int units);
	static void *mm_return_vm_page_to_kernel(void *vm_page, int units);
		
	/* macro declarations */

	#define MAX_FAMILIES_PER_VM_PAGE
		//the page size - the memory needed for the pointer to the next family page, devided by the size of memory needed by the general page-family structure
		(SYSTEM_PAGE_SIZE - sizeof(vm_page_for_families_t *)) / sizeof(vm_page_family_t)
		
	#define ITERATE_PAGE_FAMILIES_BEGIN(vm_page_for_families_ptr, curr)
	{
		uint32_t count = 0;
		for(curr = (vm_page_family_t *)&vm_page_for_families_ptr->vm_page_family[0];
			curr->struct_size && count < MAX_FAMILIES_PER_VM_PAGE; //make sure we do not go-over the maximum # of families per page
			curr++, count++) {
			
	#define ITERATE_PAGE_FAMILIES_END(vm_page_for_families_ptr, curr)	}}

#endif