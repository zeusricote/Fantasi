#ifndef FANTASI_FLASH_STORAGE_H
#define FANTASI_FLASH_STORAGE_H

#include <stdint.h>
#include <stddef.h>

#define STORAGE_SIZE       (256U * 1024U)
#define STORAGE_PAGE_SIZE  4096U
#define STORAGE_PAGE_COUNT (STORAGE_SIZE / STORAGE_PAGE_SIZE)
#define STORAGE_PROG_SIZE  1U
#define STORAGE_CACHE_SIZE 256U
#define STORAGE_LOOKAHEAD_SIZE 16U

int      storage_flash_init(void);
uint32_t storage_flash_base(void);

int  storage_flash_read(uint32_t offset, void *buf, size_t len);
int  storage_flash_erase(uint32_t page_index);
int  storage_flash_program(uint32_t offset, const void *buf, size_t len);

#endif