#ifndef __FLASH_H
#define __FLASH_H

#include "main.h"
#include "cmsis_os.h"
#include <stdint.h>
#include <stdbool.h>

/* W25Q64存储配置 */
#define W25Q64_TOTAL_SIZE          (8 * 1024 * 1024)    /* W25Q64: 8MB */
#define W25Q64_PAGE_SIZE           256                   /* 页大小 */
#define W25Q64_SECTOR_SIZE         (4 * 1024)            /* 扇区大小 4KB */
#define W25Q64_BLOCK_SIZE          (64 * 1024)           /* 块大小 64KB */

/* 存储区域划分 */
#define W25Q64_INDEX_AREA_START    0x000000              /* 索引区起始地址 */
#define W25Q64_INDEX_AREA_SIZE     (256 * 1024)         /* 索引区大小 256KB */
#define W25Q64_DATA_AREA_START     (256 * 1024)         /* 数据区起始地址 */
#define W25Q64_DATA_AREA_SIZE      (W25Q64_TOTAL_SIZE - W25Q64_INDEX_AREA_SIZE)  /* 数据区大小 */

/* 索引缓存配置 */
#define W25Q64_MAX_CACHE_ENTRIES         200                   /* RAM缓存最大条目数 */
#define W25Q64_INDEX_ENTRY_SIZE          16                    /* 每个索引条目大小 */

/* 数据头结构 */
#define W25Q64_DATA_HEADER_MAGIC         0x55AA                /* 固定标志位 */
#define W25Q64_DATA_HEADER_SIZE          12                    /* 数据头大小 */

/* 索引表结构 */
#define W25Q64_INDEX_ENTRY_MAGIC         0xAA55                /* 索引条目标志位 */
#define W25Q64_INDEX_ENTRY_SIZE          16                    /* 索引条目大小 */

/* 数据头结构体 */
typedef struct {
    uint16_t magic;             /* 固定标志位 0x55AA */
    uint32_t record_id;         /* 数据编号，自增 */
    uint32_t data_length;       /* 数据长度 */
    uint16_t crc16;            /* CRC16校验 */
} __attribute__((packed)) DataHeader_t;

/* 索引条目结构体 */
typedef struct {
    uint16_t magic;             /* 索引标志位 0xAA55 */
    uint32_t record_id;         /* 数据编号 */
    uint32_t flash_address;     /* Flash中的起始地址 */
    uint32_t data_length;       /* 数据长度 */
    uint16_t crc16;            /* CRC16校验 */
    uint16_t reserved;         /* 保留字段，用于对齐 */
} __attribute__((packed)) IndexEntry_t;

/* 缓存索引条目结构体（简化版，仅RAM使用） */
typedef struct {
    uint32_t record_id;         /* 数据编号 */
    uint32_t flash_address;     /* Flash中的起始地址 */
    uint32_t data_length;       /* 数据长度 */
} CacheEntry_t;

/* Flash操作结果枚举 */
typedef enum {
    FLASH_OK = 0,
    FLASH_ERROR_INIT,
    FLASH_ERROR_READ,
    FLASH_ERROR_WRITE,
    FLASH_ERROR_ERASE,
    FLASH_ERROR_CRC,
    FLASH_ERROR_NOT_FOUND,
    FLASH_ERROR_FULL,
    FLASH_ERROR_INVALID_PARAM,
    FLASH_ERROR_MEMORY
} FlashResult_t;

/* 数据记录结构体 */
typedef struct {
    uint32_t record_id;         /* 记录编号 */
    uint32_t data_length;       /* 数据长度 */
    uint8_t *data;             /* 数据指针 */
} DataRecord_t;

/* 读取结果结构体 */
typedef struct {
    uint32_t record_id;         /* 记录编号 */
    uint32_t data_length;       /* 数据长度 */
    uint8_t data[1024];        /* 数据缓冲区（最大1KB） */
    bool valid;                /* 数据是否有效 */
} ReadResult_t;

/* 函数声明 */

/* 初始化和配置 */
FlashResult_t Flash_Init(void);
FlashResult_t Flash_DeInit(void);

/* 基本Flash操作 */
FlashResult_t Flash_Read(uint32_t address, uint8_t *buffer, uint32_t length);
FlashResult_t Flash_Write(uint32_t address, const uint8_t *buffer, uint32_t length);
FlashResult_t Flash_EraseSector(uint32_t address);
FlashResult_t Flash_EraseBlock(uint32_t address);

/* 数据存储操作 */
FlashResult_t Flash_StoreData(const uint8_t *data, uint32_t length, uint32_t *record_id);
FlashResult_t Flash_ReadData(uint32_t record_id, ReadResult_t *result);
FlashResult_t Flash_ReadLatestRecords(uint32_t count, ReadResult_t *results, uint32_t *actual_count);

/* 索引管理 */
FlashResult_t Flash_LoadIndexTable(void);
FlashResult_t Flash_SaveIndexTable(void);
FlashResult_t Flash_ScanDataArea(void);

/* 工具函数 */
uint16_t Flash_CalculateCRC16(const uint8_t *data, uint32_t length);
FlashResult_t Flash_VerifyDataHeader(const DataHeader_t *header, const uint8_t *data);
FlashResult_t Flash_GetNextWriteAddress(uint32_t *address);
FlashResult_t Flash_GetRecordCount(uint32_t *count);

/* 调试和状态 */
void Flash_PrintStatus(void);
void Flash_PrintCacheStatus(void);
FlashResult_t Flash_GetStorageInfo(uint32_t *used_space, uint32_t *free_space, uint32_t *record_count);

/* 任务相关 */
void Flash_TaskInit(void);
void Flash_TaskProcess(void);

#endif /* __FLASH_H */
