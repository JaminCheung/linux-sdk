#ifndef _JZ_MEM_H_
#define _JZ_MEM_H_

/*
 * Extern functions
 */
void *JZMalloc(int align, int size);
void jz47_free_alloc_mem();
unsigned int get_phy_addr(unsigned int vaddr);

#endif