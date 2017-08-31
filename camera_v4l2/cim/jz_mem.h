/*
 *  Copyright (C) 2017, Monk Su<rongjin.su@ingenic.com, MonkSu@outlook.com>
 *
 *  Ingenic Linux plarform SDK project
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under  the terms of the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the License, or (at your
 *  option) any later version.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */
#ifndef _JZ_MEM_H_
#define _JZ_MEM_H_
#include <types.h>
/*
 * Extern functions
 */
void *jz_mem_alloc(int align, int size);
void jz_free_alloc_mem();
uint32_t jz_get_vaddr(uint32_t paddr);
uint32_t jz_get_phy_addr(uint32_t vaddr);

#endif