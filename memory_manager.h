#ifndef HEADER_FILE
#define HEADER_FILE

	/* type declarations */
	
	typedef enum {
		MM_FALSE,
		MM_TRUE
	} vm_bool_t;
	
	struct vm_page_family_; //forward declaration
	
	typedef struct vm_page_family_ {
		
		char struct_name [MM_MAX_STRUCT_NAME];
		uint32_t struct_size;
	
	} vm_page_family_t;

	typedef struct vm_page_families_ {
			
		struct vm_page_for_families_ *next; //pointer to the next vm family page
		vm_page_family_t vm_page_family[0]; //pointer to the array of page families
			
	} vm_page_for_families_t;
	
	typedef struct block_meta_data_ {
		
		vm_bool_t is_free;
		uint32_t block_size;
		struct block_meta_data_ *prev_block;
		struct block_meta_data_ *next_block;
		uint32_t offset;
		
	} block_meta_data_t;
	
	typedef struct vm_page_ {
		
		struct vm_page_ *next;
		struct vm_page_ *prev;
		struct vm_page_family_ *pg_family; //points back to the the page_family
		block_meta_data_t block_meta_data; //the first meta-block on the vm page
		char page_memeory[0]; //first data-block in the vm page
		
	} vm_page_t;
		
	/* macro declarations */

	#define MAX_FAMILIES_PER_VM_PAGE \
		//the page size - the memory needed for the pointer to the next family page, devided by the size of memory needed by the general page-family structure
		(SYSTEM_PAGE_SIZE - sizeof(vm_page_for_families_t *)) / sizeof(vm_page_family_t)
		
	#define ITERATE_PAGE_FAMILIES_BEGIN(vm_page_for_families_ptr, curr)	\
	{																	\
		uint32_t count = 0;												\
		for(curr = (vm_page_family_t *)&vm_page_for_families_ptr->vm_page_family[0];	\
			curr->struct_size && count < MAX_FAMILIES_PER_VM_PAGE; 	\ //make sure we do not go-over the maximum # of families per page
			curr++, count++) {
			
	#define ITERATE_PAGE_FAMILIES_END(vm_page_for_families_ptr, curr)	}}
	
	#define offset_of(struct_name, field_name)	\
		//point a null struct pointer to the specific field, grab the address and turn it into an int offset
		(int) (&((struct_name *)0)->field_name)
		
	#define MM_GET_PAGE_FROM_META_BLOCK(block_meta_data_ptr)	\
		((void *)((char *)block_meta_data_ptr - block_meta_data_ptr->offset))
		
	#define NEXT_META_BLOCK(block_meta_data_ptr)	\
		(block_meta_data_ptr->_next_block)
		
	#define NEXT_META_VLOCK_BY_SIZE(block_meta_data_ptr)	\
		(block_meta_data_t *)((char *)(block_meta_data_t + 1) + block_meta_data_ptr->block_size)
	
	#define PREV_META_BLOCK(block_meta_data_ptr)	\
		(block_meta_data_ptr->_prev_block)
		
	#define mm_bind_blocks_for_allocation(allocated_meta_block, free_meta_block) \
		free_meta_block->prev_block = allocated_meta_block;		\
		free_meta_block->next_block = allocated_meta_block->next_block;		\
		allocated_meta_block->next_block = free_meta_block;		\
		if (free_meta_nlock->next_block)		\
			free_meta_block->next_block->prev_block = free_meta_block
			
	#define MARK_VM_PAGE_EMPTY(vm_page_t_ptr)				\
		//sets a pre-existing vm page to empty (most-likely so it can be freed)
		vm_page_t_ptr->block_meta_data.next_block = NULL;	\
		vm_page_t_ptr->block_meta_data.prev_block = NULL;	\
		vm_page_t_ptr->block_meta_data.is_free = MM_TRUE
		
	#define ITERATE_VM_PAGE_BEGIN(vm_page_family_ptr, curr)	\
		for (curr = ; ; curr ) {
	
	#define ITERATE_VM_PAGE_END(vm_page_family_ptr, curr) }}
		
	/* function declarations */

	static void *mm_get_new_vm_page_from_kernal(int units);
	static void *mm_return_vm_page_to_kernel(void *vm_page, int units);
	vm_page_family_t *lookup_page_family_by_name(char *struct_name);
	static void mm_union_free_blocks(block_meta_data_t *first, block_meta_data_t *second);
	vm_bool_t *mm_is_vm_page_empty(vm_page_t *vm_page);
	static inline uint32_t mm_max_page_allocatable_memory(int units);
	vm_page_t * allocate_vm_page(vm_page_family_t *vm_page_family);
	void mm_vm_page_delete_and_free(vm_page_t *vm_page);

#endif