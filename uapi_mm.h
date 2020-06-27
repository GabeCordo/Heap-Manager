#ifndef HEADER_FILE
#define HEADER_FILE

	#include <stdint.h>

	/* function declarations */

	void mm_init();
	void mm_instantiate_new_page_family(char *struct_name, uint32_t struct_size);
	void mm_print_registered_page_families(); //needs to be implemented
	vm_page_family_t * lookup_page_family_by_name(char *struct_name); //needs to be implemented

	/* macro declarations */
	
	#define MM_REG_STRUCT (struct_name)
		(mm_instantiate_new_page_family(#struct_name, sizeof(struct_name)))
		
#endif 