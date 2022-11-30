#pragma once

/* 
* virtual.h - Virtual Memory Manager
* 
* Implemented in mem/virtual.c
*/

#include <common.h>
#include <spinlock.h>

struct virtual_address_space
{
	void* data;

	/*
	* Useful in implementing fork
	*/
	struct virtual_address_space* copied_from;

    /*
    * To prevent multiple threads from modifying us at the same time
    */
    struct spinlock lock;
};

/*
* Platform independent flags for controlling how pages are to be mapped.
*/
#define VAS_FLAG_WRITABLE		1
#define VAS_FLAG_EXECUTABLE		2
#define VAS_FLAG_USER			4
#define VAS_FLAG_COPY_ON_WRITE	8
#define VAS_FLAG_PRESENT		16
#define VAS_FLAG_LOCKED			32

size_t virt_allocate_krnl_region(size_t bytes) warn_unused;
void virt_deallocate_krnl_region(size_t virt_addr);
void virt_init(void);
size_t virt_allocate_pages(size_t pages, int flags) warn_unused; 
void virt_free_pages(size_t virt_addr);
size_t virt_bytes_to_pages(size_t bytes);

void vas_flush_tlb(void);
struct virtual_address_space* vas_get_current_vas(void) warn_unused;
struct virtual_address_space* vas_create(void) warn_unused;
void vas_destroy(struct virtual_address_space* vas);
void vas_load(struct virtual_address_space* vas);
struct virtual_address_space* vas_copy(struct virtual_address_space* original) warn_unused;
void vas_map(struct virtual_address_space* vas, size_t phys_addr, size_t virt_addr, int flags);

/*
* Returns the physical address being unmapped.
*/
size_t vas_unmap(struct virtual_address_space* vas, size_t virt_addr);
