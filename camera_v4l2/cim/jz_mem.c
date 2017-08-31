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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <types.h>
#include <sys/mman.h>
#include <utils/log.h>
#include "jz_mem.h"

#define LOG_TAG                         "jz_mem"

#define PAGE_SIZE                       (4096)
#define MEM_ALLOC_DEV_NUM               1

struct JZ_MEM_DEV {
    uint32_t vaddr;
    uint32_t paddr;
    uint32_t totalsize;
    uint32_t usedsize;
} jz_memdev[MEM_ALLOC_DEV_NUM];

struct IMEM_INFO
{
    char *name;
    uint32_t power;
    uint32_t vaddr;
}imem_info[4] = { {"/proc/jz/mem/imem", 12, 0x0},
                  {"/proc/jz/mem/imem1", 0, 0x0},
                  {"/proc/jz/mem/imem2", 0, 0x0},
                  {"/proc/jz/mem/imem3", 0, 0x0}};

static int memdev_count = 0, mem_init = 0;
int mmapfd = 0;

uint32_t jz_get_phy_addr(uint32_t vaddr)
{
    int i;
    int high_4bits;

    for (i = 0; i < memdev_count; i++) {
        if (vaddr >= jz_memdev[i].vaddr &&
            vaddr < jz_memdev[i].vaddr + jz_memdev[i].totalsize)
            return jz_memdev[i].paddr + (vaddr - jz_memdev[i].vaddr);
    }

    high_4bits = (vaddr >> 28) & 0xf;
    LOGD("high_4bits is 0x%x\n", high_4bits);

    if ((high_4bits & 0xb))
        return vaddr & 0x1FFFFFFF;

    LOGE("vaddr 0x%x get phy addr failed\n", vaddr);

    return -1;
}

uint32_t jz_get_vaddr(uint32_t paddr)
{
    int i;

    for (i = 0; i < memdev_count; i++) {
        if (paddr >= jz_memdev[i].paddr &&
            paddr < jz_memdev[i].paddr + jz_memdev[i].totalsize)
            return jz_memdev[i].vaddr + (paddr - jz_memdev[i].paddr);
    }

    return 0;
}

static void jz_exit_memalloc (void *p)
{
    int i;
    char cmdbuf[128];

    for (i = 0; i < memdev_count; i++) {
        if (jz_memdev[i].vaddr) {
            munmap((void *)jz_memdev[i].vaddr, jz_memdev[i].totalsize);
            memset (&jz_memdev[i], 0, sizeof(struct JZ_MEM_DEV));
        }

        sprintf(cmdbuf, "echo FF > %s", imem_info[i].name);
        system (cmdbuf);
    }
}

void jz_free_alloc_mem()
{
    int i;

    jz_exit_memalloc(NULL);

    for (i = 0; i < memdev_count; i++)
        jz_memdev[i].usedsize = 0;

    memdev_count = 0;
    mem_init = 0;

    if (mmapfd)
        close(mmapfd);
    mmapfd = 0;
}

static void jz_alloc_memdev()
{
    int power, fd;
    int pages = 1;
    char cmdbuf[128];
    uint32_t vaddr, paddr;

    /* Free all proc memory.  */
    if (mem_init == 0) {
        sprintf(cmdbuf, "echo FF > %s", imem_info[0].name);
        // echo FF > /proc/jz/imem
        system (cmdbuf);
        mem_init = 1;
    }

    /* open /dev/mem for map the memory.  */
    if (mmapfd == 0)
        mmapfd = open ("/dev/mem", O_RDWR);
    if (mmapfd <= 0) {
        LOGD ("+++ Jz alloc frame failed: can not mmap the memory +++\n");
        return;
    }

    /* Get the memory device.  */
    /* request max mem size of 2 ^ n pages  */
    power = imem_info[memdev_count].power;
    paddr = 0;

    LOGD ("+++ Jz:  power 0x%x memory, imem_info[memdev_count].name = %s.\n",
                                        power, imem_info[memdev_count].name);

    while (1) {
        // request mem
        sprintf (cmdbuf, "echo %x > %s", power, imem_info[memdev_count].name);
        system (cmdbuf);

        // get paddr
        fd = open (imem_info[memdev_count].name, O_RDWR);
        if (fd >= 0) {
            read (fd, &paddr, 4);
            close (fd);
        }
        if (paddr != 0 || power < 8)
            break;
        power--;
    }

    /* total page numbers. */
    pages = 1 << power;
    /* Set the memory device info. */
    if (paddr == 0) {
        LOGD ("+++ Jz: Can not get address of the reserved 4M memory.\n");
        return;
    } else {
        LOGD ("+++ Jz:  paddr 0x%x memory, pages =%d.\n", paddr, pages);
        LOGD ("+++ Jz:  imem_info[memdev_count].vaddr 0x%x.\n",
                                            imem_info[memdev_count].vaddr);
        /* mmap the memory and record vaddr and paddr. */
        /* since mips TLB table mmap is double pages.  */
        vaddr = (uint32_t)mmap ((void *)imem_info[memdev_count].vaddr,
                        pages * PAGE_SIZE,
                        (PROT_READ|PROT_WRITE), MAP_SHARED, mmapfd, paddr);

        // touch the space to create TLB table in CPU
        LOGE ("+++ Jz:  get address 0x%x memory.\n", vaddr);
        uint32_t pc = vaddr;
        int pg = 0;
        for (pg = 0; pg < pages; pg++) {
            *(volatile uint32_t *)pc = 0x0;
            pc += PAGE_SIZE;
        }
        pc--;
        LOGD ("+++ Jz:  get address pc 0x%x memory.\n", pc);
        jz_memdev[memdev_count].vaddr = vaddr;
        jz_memdev[memdev_count].paddr = paddr;
        jz_memdev[memdev_count].totalsize = (pages * PAGE_SIZE);
        jz_memdev[memdev_count].usedsize = 0;
        memdev_count++;
        LOGD ("Jz Dev alloc: vaddr = 0x%x, paddr = 0x%x, size = 0x%x ++\n",
            vaddr, paddr, (pages * PAGE_SIZE));
    }

    return;
}


void *jz_mem_alloc(int align, int size)
{
    int i, alloc_size, used_size, total_size;
    uint32_t vaddr;

    /* use the valid align value.  */
    if (align >= 1024)  align = 1024;
    else if (align >= 512)  align = 512;
    else if (align >= 256)  align = 256;
    else if (align >= 128)  align = 128;
    else if (align >= 64)  align = 64;
    else if (align >= 32)  align = 32;
    else if (align >= 16)  align = 16;
    else if (align >= 8)  align = 8;
    else if (align >= 4)  align = 4;
    else if (align >= 2)  align = 2;
    else align = 1;

    /* alloc the memory.  */
    for (i = 0; i < MEM_ALLOC_DEV_NUM; i++) {
        if (i >= memdev_count) {
            jz_alloc_memdev();
        }
        used_size = jz_memdev[i].usedsize;
        used_size = (used_size + align - 1) & ~ (align - 1);
        jz_memdev[i].usedsize = used_size;
        // get memory size for allocated.  */
        total_size = jz_memdev[i].totalsize;
        alloc_size = total_size - used_size;
        if (alloc_size >= size) {
            if ((((jz_memdev[i].vaddr & 0xFFFFFF) + used_size)&0x1C00000) !=
               (((jz_memdev[i].vaddr & 0xFFFFFF) + used_size + size) & 0x1C00000)) {
                vaddr = ((jz_memdev[i].vaddr + used_size + size) & 0xFFC00000);
                jz_memdev[i].usedsize = size + (vaddr -jz_memdev[i].vaddr);
            }else{
                jz_memdev[i].usedsize = used_size + size;
                vaddr = jz_memdev[i].vaddr + used_size;
            }
            return (void *)vaddr;
        }
    }

    LOGD ("++ JZ alloc: memory allocated is failed (size = %d) ++\n", size);

    return NULL;
}
