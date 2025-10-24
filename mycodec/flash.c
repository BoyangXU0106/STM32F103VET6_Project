#include "flash.h"
#include "spi.h"
#include "gpio.h"
#include "log.h"
#include <string.h>
#include <stdlib.h>

/* 私有变量 */
static bool g_flash_initialized = false;
static uint32_t g_next_record_id = 1;
static uint32_t g_next_write_address = W25Q64_DATA_AREA_START;
static uint32_t g_total_records = 0;

/* RAM缓存 */
static CacheEntry_t g_cache_entries[W25Q64_MAX_CACHE_ENTRIES];
static uint32_t g_cache_count = 0;
static uint32_t g_cache_start_id = 0;  /* 缓存中最小记录ID */

/* SPI Flash命令定义 */
#define W25Q64_CMD_WRITE_ENABLE      0x06
#define W25Q64_CMD_WRITE_DISABLE     0x04
#define W25Q64_CMD_READ_STATUS_REG   0x05
#define W25Q64_CMD_WRITE_STATUS_REG  0x01
#define W25Q64_CMD_PAGE_PROGRAM      0x02
#define W25Q64_CMD_QUAD_PAGE_PROGRAM 0x32
#define W25Q64_CMD_BLOCK_ERASE       0xD8
#define W25Q64_CMD_SECTOR_ERASE      0x20
#define W25Q64_CMD_CHIP_ERASE        0xC7
#define W25Q64_CMD_POWER_DOWN        0xB9
#define W25Q64_CMD_HIGH_PERFORMANCE  0xA3
#define W25Q64_CMD_CONTINUOUS_READ    0x03
#define W25Q64_CMD_FAST_READ         0x0B
#define W25Q64_CMD_QUAD_READ          0x6B
#define W25Q64_CMD_READ_JEDEC_ID     0x9F
#define W25Q64_CMD_READ_UNIQUE_ID    0x4B
#define W25Q64_CMD_READ_ID           0x90
#define W25Q64_CMD_RELEASE_POWER_DOWN 0xAB

/* 状态寄存器位定义 */
#define W25Q64_STATUS_BUSY          0x01
#define W25Q64_STATUS_WEL           0x02

/* 私有函数声明 */
static FlashResult_t Flash_WaitForReady(void);
static FlashResult_t Flash_WriteEnable(void);
static FlashResult_t Flash_ReadJEDECID(uint32_t *id);
static FlashResult_t Flash_WritePage(uint32_t address, const uint8_t *data, uint32_t length);
static FlashResult_t Flash_ReadDataInternal(uint32_t address, uint8_t *buffer, uint32_t length);
static FlashResult_t Flash_EraseInternal(uint32_t address, uint32_t size);
static void Flash_AddToCache(uint32_t record_id, uint32_t address, uint32_t length);
static FlashResult_t Flash_ForceReset(void);
static FlashResult_t Flash_ReadStatus(uint8_t *status);
static bool Flash_FindInCache(uint32_t record_id, uint32_t *address, uint32_t *length);
static FlashResult_t Flash_ScanDataAreaInternal(void);
static FlashResult_t Flash_ResetSPI(void);
static FlashResult_t Flash_RecoverFromError(void);

/* CRC16计算表 */
static const uint16_t crc16_table[256] = {
    0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7,
    0x8108, 0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF,
    0x1231, 0x0210, 0x3273, 0x2252, 0x52B5, 0x4294, 0x72F7, 0x62D6,
    0x9339, 0x8318, 0xB37B, 0xA35A, 0xD3BD, 0xC39C, 0xF3FF, 0xE3DE,
    0x2462, 0x3443, 0x0420, 0x1401, 0x64E6, 0x74C7, 0x44A4, 0x5485,
    0xB56A, 0xA54B, 0x9528, 0x8509, 0xF5EE, 0xE5CF, 0xD5AC, 0xC58D,
    0x3653, 0x2672, 0x1611, 0x0630, 0x76D7, 0x66F6, 0x5695, 0x46B4,
    0xB75B, 0xA77A, 0x9719, 0x8738, 0xF7DF, 0xE7FE, 0xD79D, 0xC7BC,
    0x48C4, 0x58E5, 0x6886, 0x78A7, 0x0840, 0x1861, 0x2802, 0x3823,
    0xC9CC, 0xD9ED, 0xE98E, 0xF9AF, 0x8948, 0x9969, 0xA90A, 0xB92B,
    0x5AF5, 0x4AD4, 0x7AB7, 0x6A96, 0x1A71, 0x0A50, 0x3A33, 0x2A12,
    0xDBFD, 0xCBDC, 0xFBBF, 0xEB9E, 0x9B79, 0x8B58, 0xBB3B, 0xAB1A,
    0x6CA6, 0x7C87, 0x4CE4, 0x5CC5, 0x2C22, 0x3C03, 0x0C60, 0x1C41,
    0xEDAE, 0xFD8F, 0xCDEC, 0xDDCD, 0xAD2A, 0xBD0B, 0x8D68, 0x9D49,
    0x7E97, 0x6EB6, 0x5ED5, 0x4EF4, 0x3E13, 0x2E32, 0x1E51, 0x0E70,
    0xFF9F, 0xEFBE, 0xDFDD, 0xCFFC, 0xBF1B, 0xAF3A, 0x9F59, 0x8F78,
    0x9188, 0x81A9, 0xB1CA, 0xA1EB, 0xD10C, 0xC12D, 0xF14E, 0xE16F,
    0x1080, 0x00A1, 0x30C2, 0x20E3, 0x5004, 0x4025, 0x7046, 0x6067,
    0x83B9, 0x9398, 0xA3FB, 0xB3DA, 0xC33D, 0xD31C, 0xE37F, 0xF35E,
    0x02B1, 0x1290, 0x22F3, 0x32D2, 0x4235, 0x5214, 0x6277, 0x7256,
    0xB5EA, 0xA5CB, 0x95A8, 0x8589, 0xF56E, 0xE54F, 0xD52C, 0xC50D,
    0x34E2, 0x24C3, 0x14A0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
    0xA7DB, 0xB7FA, 0x8799, 0x97B8, 0xE75F, 0xF77E, 0xC71D, 0xD73C,
    0x26D3, 0x36F2, 0x0691, 0x16B0, 0x6657, 0x7676, 0x4615, 0x5634,
    0xD94C, 0xC96D, 0xF90E, 0xE92F, 0x99C8, 0x89E9, 0xB98A, 0xA9AB,
    0x5844, 0x4865, 0x7806, 0x6827, 0x18C0, 0x08E1, 0x3882, 0x28A3,
    0xCB7D, 0xDB5C, 0xEB3F, 0xFB1E, 0x8BF9, 0x9BD8, 0xABBB, 0xBB9A,
    0x4A75, 0x5A54, 0x6A37, 0x7A16, 0x0AF1, 0x1AD0, 0x2AB3, 0x3A92,
    0xFD2E, 0xED0F, 0xDD6C, 0xCD4D, 0xBDAA, 0xAD8B, 0x9DE8, 0x8DC9,
    0x7C26, 0x6C07, 0x5C64, 0x4C45, 0x3CA2, 0x2C83, 0x1CE0, 0x0CC1,
    0xEF1F, 0xFF3E, 0xCF5D, 0xDF7C, 0xAF9B, 0xBFBA, 0x8FD9, 0x9FF8,
    0x6E17, 0x7E36, 0x4E55, 0x5E74, 0x2E93, 0x3EB2, 0x0ED1, 0x1EF0
};

/**
 * @brief 强制重置Flash
 * @return FlashResult_t 操作结果
 */
static FlashResult_t Flash_ForceReset(void)
{
    Log_Warn("Flash: Performing force reset...");
    
    /* 确保CS引脚为高电平 */
    HAL_GPIO_WritePin(FLASH_CS_GPIO_Port, FLASH_CS_Pin, GPIO_PIN_SET);
    osDelay(10);
    
    /* 发送软件复位命令 */
    uint8_t reset_cmd = 0x66;  // 使能复位命令
    HAL_GPIO_WritePin(FLASH_CS_GPIO_Port, FLASH_CS_Pin, GPIO_PIN_RESET);
    if (HAL_SPI_Transmit(&hspi1, &reset_cmd, 1, 50) != HAL_OK) {
        HAL_GPIO_WritePin(FLASH_CS_GPIO_Port, FLASH_CS_Pin, GPIO_PIN_SET);
        Log_Error("Flash: Failed to send reset enable command");
        return FLASH_ERROR_READ;
    }
    HAL_GPIO_WritePin(FLASH_CS_GPIO_Port, FLASH_CS_Pin, GPIO_PIN_SET);
    osDelay(1);
    
    /* 发送复位命令 */
    uint8_t reset_execute = 0x99;  // 执行复位命令
    HAL_GPIO_WritePin(FLASH_CS_GPIO_Port, FLASH_CS_Pin, GPIO_PIN_RESET);
    if (HAL_SPI_Transmit(&hspi1, &reset_execute, 1, 50) != HAL_OK) {
        HAL_GPIO_WritePin(FLASH_CS_GPIO_Port, FLASH_CS_Pin, GPIO_PIN_SET);
        Log_Error("Flash: Failed to send reset execute command");
        return FLASH_ERROR_READ;
    }
    HAL_GPIO_WritePin(FLASH_CS_GPIO_Port, FLASH_CS_Pin, GPIO_PIN_SET);
    
    /* 等待复位完成 */
    osDelay(50);
    
    /* 验证复位是否成功 */
    uint32_t jedec_id;
    FlashResult_t result = Flash_ReadJEDECID(&jedec_id);
    if (result != FLASH_OK || jedec_id != 0xEF4017) {
        Log_Error("Flash: Force reset verification failed, JEDEC ID: 0x%08X", jedec_id);
        return FLASH_ERROR_READ;
    }
    
    Log_Info("Flash: Force reset completed successfully");
    return FLASH_OK;
}

/**
 * @brief 读取Flash状态寄存器
 * @param status 状态寄存器值指针
 * @return FlashResult_t 操作结果
 */
static FlashResult_t Flash_ReadStatus(uint8_t *status)
{
    if (status == NULL) {
        return FLASH_ERROR_INVALID_PARAM;
    }
    
    uint8_t cmd = W25Q64_CMD_READ_STATUS_REG;
    uint8_t status_value;
    
    /* 发送读取状态寄存器命令 */
    HAL_GPIO_WritePin(FLASH_CS_GPIO_Port, FLASH_CS_Pin, GPIO_PIN_RESET);
    if (HAL_SPI_Transmit(&hspi1, &cmd, 1, 50) != HAL_OK) {
        HAL_GPIO_WritePin(FLASH_CS_GPIO_Port, FLASH_CS_Pin, GPIO_PIN_SET);
        Log_Error("Flash: Failed to send read status command");
        return FLASH_ERROR_READ;
    }
    
    /* 读取状态寄存器值 */
    if (HAL_SPI_Receive(&hspi1, &status_value, 1, 50) != HAL_OK) {
        HAL_GPIO_WritePin(FLASH_CS_GPIO_Port, FLASH_CS_Pin, GPIO_PIN_SET);
        Log_Error("Flash: Failed to read status register");
        return FLASH_ERROR_READ;
    }
    
    HAL_GPIO_WritePin(FLASH_CS_GPIO_Port, FLASH_CS_Pin, GPIO_PIN_SET);
    
    *status = status_value;
    return FLASH_OK;
}

/**
 * @brief 初始化Flash存储系统
 * @return FlashResult_t 操作结果
 */
FlashResult_t Flash_Init(void)
{
    if (g_flash_initialized) {
        Log_Warn("Flash: Already initialized");
        return FLASH_OK;
    }
    
    Log_Info("Flash: Initializing W25Q64...");
    
    /* 初始化SPI */
    //MX_SPI1_Init();  // SPI已在main函数中初始化
    
    /* 等待Flash就绪 */
    if (Flash_WaitForReady() != FLASH_OK) {
        Log_Error("Flash: Failed to wait for ready");
        return FLASH_ERROR_INIT;
    }
    
    /* 读取并验证JEDEC ID */
    uint32_t jedec_id;
    if (Flash_ReadJEDECID(&jedec_id) != FLASH_OK) {
        Log_Error("Flash: Failed to read JEDEC ID");
        return FLASH_ERROR_INIT;
    }
    
    if (jedec_id != 0xEF4017) {
        Log_Error("Flash: Invalid JEDEC ID, expected 0xEF4017, got 0x%08X", jedec_id);
        return FLASH_ERROR_INIT;
    }
    
    /* 初始化变量 */
    g_next_record_id = 1;
    g_next_write_address = W25Q64_DATA_AREA_START;
    g_total_records = 0;
    g_cache_count = 0;
    g_cache_start_id = 0;
    
    /* 加载索引表 */
    FlashResult_t result = Flash_LoadIndexTable();
    if (result != FLASH_OK) {
        Log_Warn("Flash: Failed to load index table, scanning data area...");
        result = Flash_ScanDataArea();
        if (result != FLASH_OK) {
            Log_Error("Flash: Failed to scan data area");
            return result;
        }
    }
    
    /* 验证存储连续性 */
    if (g_total_records > 0) {
        Log_Info("Flash: Verifying storage continuity...");
        Log_Info("Flash: Found %lu existing records, continuing from ID %lu", 
                 g_total_records, g_next_record_id);
    } else {
        Log_Info("Flash: No existing records found, starting fresh");
    }
    
    g_flash_initialized = true;
    Log_Info("Flash: Initialization completed - Records: %lu, Next ID: %lu, Next address: 0x%08X", 
             g_total_records, g_next_record_id, g_next_write_address);
    
    return FLASH_OK;
}

/**
 * @brief 反初始化Flash存储系统
 * @return FlashResult_t 操作结果
 */
FlashResult_t Flash_DeInit(void)
{
    if (!g_flash_initialized) {
        return FLASH_OK;
    }
    
    /* 保存索引表 */
    Flash_SaveIndexTable();
    
    g_flash_initialized = false;
    Log_Info("Flash: Deinitialized");
    
    return FLASH_OK;
}

/**
 * @brief 等待Flash就绪
 * @return FlashResult_t 操作结果
 */
static FlashResult_t Flash_WaitForReady(void)
{
    uint8_t status;
    uint32_t timeout = 1000;  /* 1秒超时，减少等待时间 */
    uint32_t retry_count = 0;
    
    do {
        /* 读取状态寄存器 */
        uint8_t cmd = W25Q64_CMD_READ_STATUS_REG;
        HAL_GPIO_WritePin(FLASH_CS_GPIO_Port, FLASH_CS_Pin, GPIO_PIN_RESET);
        
        if (HAL_SPI_Transmit(&hspi1, &cmd, 1, 50) != HAL_OK) {
            HAL_GPIO_WritePin(FLASH_CS_GPIO_Port, FLASH_CS_Pin, GPIO_PIN_SET);
            Log_Error("Flash: Failed to send status register command (retry %lu)", retry_count);
            osDelay(10);
            retry_count++;
            if (retry_count > 10) {
                Log_Error("Flash: Too many SPI transmit failures");
                return FLASH_ERROR_READ;
            }
            continue;
        }
        
        if (HAL_SPI_Receive(&hspi1, &status, 1, 50) != HAL_OK) {
            HAL_GPIO_WritePin(FLASH_CS_GPIO_Port, FLASH_CS_Pin, GPIO_PIN_SET);
            Log_Error("Flash: Failed to receive status register (retry %lu)", retry_count);
            osDelay(10);
            retry_count++;
            if (retry_count > 10) {
                Log_Error("Flash: Too many SPI receive failures");
                return FLASH_ERROR_READ;
            }
            continue;
        }
        
        HAL_GPIO_WritePin(FLASH_CS_GPIO_Port, FLASH_CS_Pin, GPIO_PIN_SET);
        
        /* 每100次检查打印一次状态 */
//        if (timeout % 100 == 0) {
//            Log_Debug("Flash: Wait for ready - status: 0x%02X, timeout: %lu", status, timeout);
//        }
        
        /* 检查Flash状态异常 - 如果状态为0x03且等待时间过长，强制重置 */
        if (status == 0x03 && (1000 - timeout) > 200) {
            Log_Warn("Flash: Detected stuck programming state (0x03), forcing reset...");
            FlashResult_t reset_result = Flash_ForceReset();
            if (reset_result == FLASH_OK) {
                Log_Info("Flash: Reset successful, continuing...");
                return FLASH_OK;
            } else {
                Log_Error("Flash: Reset failed, continuing with timeout...");
            }
        }
        
        if (!(status & W25Q64_STATUS_BUSY)) {
//            Log_Debug("Flash: Ready after %lu attempts", 1000 - timeout);
            return FLASH_OK;
        }
        
        osDelay(1);
        timeout--;
    } while (timeout > 0);
    
    Log_Error("Flash: Wait for ready timeout after %lu attempts", 1000 - timeout);
    
    /* 强制重置Flash状态 */
    Log_Warn("Flash: Forcing Flash reset due to timeout...");
    FlashResult_t reset_result = Flash_ForceReset();
    if (reset_result != FLASH_OK) {
        Log_Error("Flash: Force reset failed");
        return FLASH_ERROR_READ;
    }
    
    return FLASH_ERROR_READ;
}

/**
 * @brief 写使能
 * @return FlashResult_t 操作结果
 */
static FlashResult_t Flash_WriteEnable(void)
{
    uint8_t cmd = W25Q64_CMD_WRITE_ENABLE;
    
    HAL_GPIO_WritePin(FLASH_CS_GPIO_Port, FLASH_CS_Pin, GPIO_PIN_RESET);
    if (HAL_SPI_Transmit(&hspi1, &cmd, 1, 100) != HAL_OK) {
        HAL_GPIO_WritePin(FLASH_CS_GPIO_Port, FLASH_CS_Pin, GPIO_PIN_SET);
        Log_Error("Flash: Failed to send write enable command");
        return FLASH_ERROR_WRITE;
    }
    HAL_GPIO_WritePin(FLASH_CS_GPIO_Port, FLASH_CS_Pin, GPIO_PIN_SET);
    
    return FLASH_OK;
}

/**
 * @brief 读取JEDEC ID
 * @param id JEDEC ID指针
 * @return FlashResult_t 操作结果
 */
static FlashResult_t Flash_ReadJEDECID(uint32_t *id)
{
    uint8_t cmd = W25Q64_CMD_READ_JEDEC_ID;
    uint8_t data[3];
    
    HAL_GPIO_WritePin(FLASH_CS_GPIO_Port, FLASH_CS_Pin, GPIO_PIN_RESET);
    if (HAL_SPI_Transmit(&hspi1, &cmd, 1, 100) != HAL_OK) {
        HAL_GPIO_WritePin(FLASH_CS_GPIO_Port, FLASH_CS_Pin, GPIO_PIN_SET);
        Log_Error("Flash: Failed to send JEDEC ID command");
        return FLASH_ERROR_READ;
    }
    if (HAL_SPI_Receive(&hspi1, data, 3, 50) != HAL_OK) {
        HAL_GPIO_WritePin(FLASH_CS_GPIO_Port, FLASH_CS_Pin, GPIO_PIN_SET);
        Log_Error("Flash: Failed to receive JEDEC ID data");
        return FLASH_ERROR_READ;
    }
    HAL_GPIO_WritePin(FLASH_CS_GPIO_Port, FLASH_CS_Pin, GPIO_PIN_SET);
    
    *id = (data[0] << 16) | (data[1] << 8) | data[2];
    
    return FLASH_OK;
}

/**
 * @brief 写入页面数据
 * @param address 地址
 * @param data 数据指针
 * @param length 数据长度
 * @return FlashResult_t 操作结果
 */
static FlashResult_t Flash_WritePage(uint32_t address, const uint8_t *data, uint32_t length)
{
    if (length > W25Q64_PAGE_SIZE) {
        Log_Error("Flash: Write page length %lu exceeds page size %lu", length, W25Q64_PAGE_SIZE);
        return FLASH_ERROR_INVALID_PARAM;
    }
    
    /* 检查Flash是否被保护 */
    uint8_t status_reg;
    HAL_GPIO_WritePin(FLASH_CS_GPIO_Port, FLASH_CS_Pin, GPIO_PIN_RESET);
    uint8_t cmd = W25Q64_CMD_READ_STATUS_REG;
    if (HAL_SPI_Transmit(&hspi1, &cmd, 1, 100) != HAL_OK) {
        HAL_GPIO_WritePin(FLASH_CS_GPIO_Port, FLASH_CS_Pin, GPIO_PIN_SET);
        Log_Error("Flash: Failed to read status register");
        return FLASH_ERROR_READ;
    }
    if (HAL_SPI_Receive(&hspi1, &status_reg, 1, 50) != HAL_OK) {
        HAL_GPIO_WritePin(FLASH_CS_GPIO_Port, FLASH_CS_Pin, GPIO_PIN_SET);
        Log_Error("Flash: Failed to receive status register");
        return FLASH_ERROR_READ;
    }
    HAL_GPIO_WritePin(FLASH_CS_GPIO_Port, FLASH_CS_Pin, GPIO_PIN_SET);
    
    /* 写使能 */
    if (Flash_WriteEnable() != FLASH_OK) {
        Log_Error("Flash: Write enable failed");
        return FLASH_ERROR_WRITE;
    }
    
    /* 等待就绪 */
    if (Flash_WaitForReady() != FLASH_OK) {
        Log_Error("Flash: Wait for ready failed before write");
        return FLASH_ERROR_WRITE;
    }
    
    /* 发送页编程命令 */
    uint8_t cmd_array[4];
    cmd_array[0] = W25Q64_CMD_PAGE_PROGRAM;
    cmd_array[1] = (address >> 16) & 0xFF;
    cmd_array[2] = (address >> 8) & 0xFF;
    cmd_array[3] = address & 0xFF;
    
    HAL_GPIO_WritePin(FLASH_CS_GPIO_Port, FLASH_CS_Pin, GPIO_PIN_RESET);
    
    /* 发送命令和地址 */
    if (HAL_SPI_Transmit(&hspi1, cmd_array, 4, 100) != HAL_OK) {
        HAL_GPIO_WritePin(FLASH_CS_GPIO_Port, FLASH_CS_Pin, GPIO_PIN_SET);
        Log_Error("Flash: Failed to send page program command");
        return FLASH_ERROR_WRITE;
    }
    
    /* 发送数据 */
    if (HAL_SPI_Transmit(&hspi1, (uint8_t*)data, length, 1000) != HAL_OK) {
        HAL_GPIO_WritePin(FLASH_CS_GPIO_Port, FLASH_CS_Pin, GPIO_PIN_SET);
        Log_Error("Flash: Failed to send page program data");
        return FLASH_ERROR_WRITE;
    }
    
    HAL_GPIO_WritePin(FLASH_CS_GPIO_Port, FLASH_CS_Pin, GPIO_PIN_SET);
    
    /* 等待写入完成 */
    FlashResult_t result = Flash_WaitForReady();
    if (result != FLASH_OK) {
        Log_Error("Flash: Wait for ready failed after write");
        return result;
    }
    return FLASH_OK;
}

/**
 * @brief 内部读取数据
 * @param address 地址
 * @param buffer 缓冲区
 * @param length 长度
 * @return FlashResult_t 操作结果
 */
static FlashResult_t Flash_ReadDataInternal(uint32_t address, uint8_t *buffer, uint32_t length)
{
    uint8_t cmd[4];
    uint8_t dummy[4] = {0};  // 用于接收命令响应
    cmd[0] = W25Q64_CMD_CONTINUOUS_READ;
    cmd[1] = (address >> 16) & 0xFF;
    cmd[2] = (address >> 8) & 0xFF;
    cmd[3] = address & 0xFF;
    
    Log_Debug("Flash: Reading %lu bytes from address 0x%08lX", length, address);
    
    /* 检查Flash状态，如果异常则重置 */
    uint8_t status;
    if (Flash_ReadStatus(&status) == FLASH_OK) {
        if (status == 0x03) {
            Log_Warn("Flash: Detected programming state before read, forcing reset...");
            if (Flash_ForceReset() != FLASH_OK) {
                Log_Error("Flash: Force reset failed before read");
                return FLASH_ERROR_READ;
            }
        }
    }
    
    HAL_GPIO_WritePin(FLASH_CS_GPIO_Port, FLASH_CS_Pin, GPIO_PIN_RESET);
    
    /* 发送命令和地址，同时接收响应 */
    if (HAL_SPI_TransmitReceive(&hspi1, cmd, dummy, 4, 50) != HAL_OK) {
        HAL_GPIO_WritePin(FLASH_CS_GPIO_Port, FLASH_CS_Pin, GPIO_PIN_SET);
        Log_Error("Flash: Failed to send read command to address 0x%08lX", address);
        
        /* 尝试错误恢复 */
        if (Flash_RecoverFromError() == FLASH_OK) {
            Log_Info("Flash: Retrying read after recovery...");
            /* 重试一次 */
            HAL_GPIO_WritePin(FLASH_CS_GPIO_Port, FLASH_CS_Pin, GPIO_PIN_RESET);
            if (HAL_SPI_TransmitReceive(&hspi1, cmd, dummy, 4, 50) != HAL_OK) {
                HAL_GPIO_WritePin(FLASH_CS_GPIO_Port, FLASH_CS_Pin, GPIO_PIN_SET);
                Log_Error("Flash: Retry failed after recovery");
                return FLASH_ERROR_READ;
            }
        } else {
            return FLASH_ERROR_READ;
        }
    }
    
    /* 接收数据 - 大幅减少超时时间，避免长时间阻塞 */
    uint32_t timeout = (length > 50) ? 50 : 20;  // 大幅减少超时时间
    if (HAL_SPI_Receive(&hspi1, buffer, length, timeout) != HAL_OK) {
        HAL_GPIO_WritePin(FLASH_CS_GPIO_Port, FLASH_CS_Pin, GPIO_PIN_SET);
        Log_Error("Flash: Failed to receive read data from address 0x%08lX, length %lu", address, length);
        
        /* 尝试错误恢复 */
        if (Flash_RecoverFromError() == FLASH_OK) {
            Log_Info("Flash: Retrying read after recovery...");
            /* 重新发送命令并接收数据 */
            HAL_GPIO_WritePin(FLASH_CS_GPIO_Port, FLASH_CS_Pin, GPIO_PIN_RESET);
            if (HAL_SPI_TransmitReceive(&hspi1, cmd, dummy, 4, 50) != HAL_OK) {
                HAL_GPIO_WritePin(FLASH_CS_GPIO_Port, FLASH_CS_Pin, GPIO_PIN_SET);
                Log_Error("Flash: Retry command failed after recovery");
                return FLASH_ERROR_READ;
            }
            if (HAL_SPI_Receive(&hspi1, buffer, length, timeout) != HAL_OK) {
                HAL_GPIO_WritePin(FLASH_CS_GPIO_Port, FLASH_CS_Pin, GPIO_PIN_SET);
                Log_Error("Flash: Retry receive failed after recovery");
                return FLASH_ERROR_READ;
            }
        } else {
            return FLASH_ERROR_READ;
        }
    }
    
    HAL_GPIO_WritePin(FLASH_CS_GPIO_Port, FLASH_CS_Pin, GPIO_PIN_SET);
    
    Log_Debug("Flash: Successfully read %lu bytes from address 0x%08lX", length, address);
    return FLASH_OK;
}

/**
 * @brief 擦除扇区或块
 * @param address 地址
 * @param size 大小
 * @return FlashResult_t 操作结果
 */
static FlashResult_t Flash_EraseInternal(uint32_t address, uint32_t size)
{
    uint8_t cmd[4];
    uint8_t erase_cmd;
    
    if (size >= W25Q64_BLOCK_SIZE) {
        erase_cmd = W25Q64_CMD_BLOCK_ERASE;
    } else {
        erase_cmd = W25Q64_CMD_SECTOR_ERASE;
    }
    
    cmd[0] = erase_cmd;
    cmd[1] = (address >> 16) & 0xFF;
    cmd[2] = (address >> 8) & 0xFF;
    cmd[3] = address & 0xFF;
    
    /* 写使能 */
    if (Flash_WriteEnable() != FLASH_OK) {
        Log_Error("Flash: Write enable failed before erase");
        return FLASH_ERROR_ERASE;
    }
    
    /* 等待就绪 */
    if (Flash_WaitForReady() != FLASH_OK) {
        Log_Error("Flash: Wait for ready failed before erase");
        return FLASH_ERROR_ERASE;
    }
    
    /* 发送擦除命令 */
    HAL_GPIO_WritePin(FLASH_CS_GPIO_Port, FLASH_CS_Pin, GPIO_PIN_RESET);
    if (HAL_SPI_Transmit(&hspi1, cmd, 4, 100) != HAL_OK) {
        HAL_GPIO_WritePin(FLASH_CS_GPIO_Port, FLASH_CS_Pin, GPIO_PIN_SET);
        Log_Error("Flash: Failed to send erase command");
        return FLASH_ERROR_ERASE;
    }
    HAL_GPIO_WritePin(FLASH_CS_GPIO_Port, FLASH_CS_Pin, GPIO_PIN_SET);
    
    /* 等待擦除完成 */
    FlashResult_t result = Flash_WaitForReady();
    if (result != FLASH_OK) {
        Log_Error("Flash: Wait for ready failed after erase");
        return result;
    }
    return FLASH_OK;
}

/**
 * @brief 计算CRC16
 * @param data 数据指针
 * @param length 数据长度
 * @return uint16_t CRC16值
 */
uint16_t Flash_CalculateCRC16(const uint8_t *data, uint32_t length)
{
    uint16_t crc = 0xFFFF;
    
    for (uint32_t i = 0; i < length; i++) {
        crc = (crc << 8) ^ crc16_table[((crc >> 8) ^ data[i]) & 0xFF];
    }
    
    return crc;
}

/**
 * @brief 添加条目到缓存
 * @param record_id 记录ID
 * @param address Flash地址
 * @param length 数据长度
 */
static void Flash_AddToCache(uint32_t record_id, uint32_t address, uint32_t length)
{
    /* 如果缓存已满，移除最旧的条目 */
    if (g_cache_count >= W25Q64_MAX_CACHE_ENTRIES) {
        /* 移动所有条目向前一位 */
        for (uint32_t i = 0; i < W25Q64_MAX_CACHE_ENTRIES - 1; i++) {
            g_cache_entries[i] = g_cache_entries[i + 1];
        }
        g_cache_count = W25Q64_MAX_CACHE_ENTRIES - 1;
        g_cache_start_id = g_cache_entries[0].record_id;
    }
    
    /* 添加新条目 */
    g_cache_entries[g_cache_count].record_id = record_id;
    g_cache_entries[g_cache_count].flash_address = address;
    g_cache_entries[g_cache_count].data_length = length;
    g_cache_count++;
    
    if (g_cache_count == 1) {
        g_cache_start_id = record_id;
    }
}

/**
 * @brief 在缓存中查找记录
 * @param record_id 记录ID
 * @param address 输出Flash地址
 * @param length 输出数据长度
 * @return bool 是否找到
 */
static bool Flash_FindInCache(uint32_t record_id, uint32_t *address, uint32_t *length)
{
    for (uint32_t i = 0; i < g_cache_count; i++) {
        if (g_cache_entries[i].record_id == record_id) {
            *address = g_cache_entries[i].flash_address;
            *length = g_cache_entries[i].data_length;
            return true;
        }
    }
    return false;
}

/**
 * @brief 存储数据到Flash
 * @param data 数据指针
 * @param length 数据长度
 * @param record_id 输出记录ID
 * @return FlashResult_t 操作结果
 */
FlashResult_t Flash_StoreData(const uint8_t *data, uint32_t length, uint32_t *record_id)
{
    if (!g_flash_initialized) {
        return FLASH_ERROR_INIT;
    }
    
    if (data == NULL || length == 0 || record_id == NULL) {
        return FLASH_ERROR_INVALID_PARAM;
    }
    
    /* 检查数据区空间 */
    if (g_next_write_address + W25Q64_DATA_HEADER_SIZE + length > W25Q64_DATA_AREA_START + W25Q64_DATA_AREA_SIZE) {
        Log_Error("Flash: Data area full");
        return FLASH_ERROR_FULL;
    }
    
    /* 准备数据头 */
    DataHeader_t header;
    header.magic = W25Q64_DATA_HEADER_MAGIC;
    header.record_id = g_next_record_id;
    header.data_length = length;
    header.crc16 = Flash_CalculateCRC16(data, length);
    
    /* 计算需要擦除的扇区 */
    uint32_t total_size = sizeof(DataHeader_t) + length;
    uint32_t sector_size = W25Q64_SECTOR_SIZE;
    uint32_t sector_address = (g_next_write_address / sector_size) * sector_size;
    
    /* 擦除扇区 */
    if (Flash_EraseInternal(sector_address, sector_size) != FLASH_OK) {
        Log_Error("Flash: Failed to erase sector before write");
        return FLASH_ERROR_ERASE;
    }
    
    /* 写入数据头 */
    uint32_t header_address = g_next_write_address;
    if (Flash_WritePage(header_address, (uint8_t*)&header, sizeof(DataHeader_t)) != FLASH_OK) {
        Log_Error("Flash: Failed to write data header");
        return FLASH_ERROR_WRITE;
    }
    
    /* 立即验证数据头写入 */
    DataHeader_t verify_header;
    if (Flash_ReadDataInternal(header_address, (uint8_t*)&verify_header, sizeof(DataHeader_t)) != FLASH_OK) {
        Log_Error("Flash: Failed to read back header for verification");
        return FLASH_ERROR_READ;
    }
    
    Log_Debug("Flash: Header verify - magic: 0x%04X->0x%04X, ID: %lu->%lu", 
              header.magic, verify_header.magic, header.record_id, verify_header.record_id);
    
    if (verify_header.magic != header.magic || verify_header.record_id != header.record_id) {
        Log_Error("Flash: Header verification failed - data corruption detected");
        return FLASH_ERROR_CRC;
    }
    
    /* 写入数据 */
    uint32_t data_address = header_address + sizeof(DataHeader_t);
    if (Flash_WritePage(data_address, data, length) != FLASH_OK) {
        Log_Error("Flash: Failed to write data");
        return FLASH_ERROR_WRITE;
    }
    
    /* 立即验证数据写入 */
    uint8_t *verify_data = (uint8_t*)malloc(length);
    if (verify_data == NULL) {
        Log_Error("Flash: Failed to allocate memory for verification");
        return FLASH_ERROR_MEMORY;
    }
    
    if (Flash_ReadDataInternal(data_address, verify_data, length) != FLASH_OK) {
        Log_Error("Flash: Failed to read back data for verification");
        free(verify_data);
        return FLASH_ERROR_READ;
    }
    
    /* 比较数据 */
    bool data_match = true;
    for (uint32_t i = 0; i < length; i++) {
        if (verify_data[i] != data[i]) {
            Log_Error("Flash: Data verification failed at byte %lu - written: 0x%02X, read: 0x%02X", 
                     i, data[i], verify_data[i]);
            data_match = false;
            break;
        }
    }
    
    free(verify_data);
    
    if (!data_match) {
        Log_Error("Flash: Data verification failed - data corruption detected");
        return FLASH_ERROR_CRC;
    }
    
    Log_Debug("Flash: Write verification successful - %lu bytes verified", length);
    
    /* 更新缓存 */
    Flash_AddToCache(g_next_record_id, header_address, length);
    
    /* 更新全局变量 */
    *record_id = g_next_record_id;
    g_next_record_id++;
    g_total_records++;
    g_next_write_address = data_address + length;
    
    /* 对齐到页边界 */
    if (g_next_write_address % W25Q64_PAGE_SIZE != 0) {
        g_next_write_address = ((g_next_write_address / W25Q64_PAGE_SIZE) + 1) * W25Q64_PAGE_SIZE;
    }
    
    Log_Info("Flash: Stored ID:%lu, %lu bytes @0x%08X", 
             *record_id, length, header_address);
    
    /* 保存索引表到Flash */
    if (Flash_SaveIndexTable() != FLASH_OK) {
        Log_Error("Flash: Failed to save index table");
        return FLASH_ERROR_WRITE;
    }
    
    return FLASH_OK;
}

/**
 * @brief 读取数据
 * @param record_id 记录ID
 * @param result 读取结果
 * @return FlashResult_t 操作结果
 */
FlashResult_t Flash_ReadData(uint32_t record_id, ReadResult_t *result)
{
    if (!g_flash_initialized) {
        return FLASH_ERROR_INIT;
    }
    
    if (result == NULL) {
        return FLASH_ERROR_INVALID_PARAM;
    }
    
    result->valid = false;
    result->record_id = record_id;
    result->data_length = 0;
    
    uint32_t address, length;
    
    /* 先在缓存中查找 */
    if (Flash_FindInCache(record_id, &address, &length)) {
        Log_Debug("Flash: Found record %lu in cache", record_id);
    } else {
        Log_Warn("Flash: Record %lu not found in cache", record_id);
        return FLASH_ERROR_NOT_FOUND;
    }
    
    /* 读取数据头 */
    DataHeader_t header;
    uint8_t raw_header[sizeof(DataHeader_t)];
    Log_Debug("Flash: Reading header from address 0x%08lX, size: %lu", address, sizeof(DataHeader_t));
    if (Flash_ReadDataInternal(address, raw_header, sizeof(DataHeader_t)) != FLASH_OK) {
        Log_Error("Flash: Failed to read data header");
        return FLASH_ERROR_READ;
    }
    
    /* 打印原始字节数据 */
    Log_Debug("Flash: Raw header bytes: %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X",
              raw_header[0], raw_header[1], raw_header[2], raw_header[3], 
              raw_header[4], raw_header[5], raw_header[6], raw_header[7],
              raw_header[8], raw_header[9], raw_header[10], raw_header[11]);
    
    /* 复制到结构体 */
    memcpy(&header, raw_header, sizeof(DataHeader_t));
    
    /* 验证数据头 */
    Log_Debug("Flash: Read header - magic: 0x%04X, expected: 0x%04X, record_id: %lu, expected: %lu", 
              header.magic, W25Q64_DATA_HEADER_MAGIC, header.record_id, record_id);
    
    /* 检查字节序问题 - 尝试字节序转换 */
    uint16_t magic_be = __REV16(header.magic);
    Log_Debug("Flash: Byte-swapped magic: 0x%04X", magic_be);
    
    if (header.magic != W25Q64_DATA_HEADER_MAGIC && magic_be != W25Q64_DATA_HEADER_MAGIC) {
        Log_Error("Flash: Invalid data header - magic mismatch");
        return FLASH_ERROR_CRC;
    }
    
    if (header.record_id != record_id) {
        Log_Error("Flash: Invalid data header - record_id mismatch");
        return FLASH_ERROR_CRC;
    }
    
    /* 检查数据长度 */
    if (header.data_length > sizeof(result->data)) {
        Log_Error("Flash: Data too large for buffer");
        return FLASH_ERROR_INVALID_PARAM;
    }
    
    /* 读取数据 */
    uint32_t data_address = address + sizeof(DataHeader_t);
    if (Flash_ReadDataInternal(data_address, result->data, header.data_length) != FLASH_OK) {
        Log_Error("Flash: Failed to read data");
        return FLASH_ERROR_READ;
    }
    
    /* 验证CRC */
    uint16_t calculated_crc = Flash_CalculateCRC16(result->data, header.data_length);
    if (calculated_crc != header.crc16) {
        Log_Error("Flash: CRC mismatch, expected 0x%04X, got 0x%04X", header.crc16, calculated_crc);
        return FLASH_ERROR_CRC;
    }
    
    result->valid = true;
    result->data_length = header.data_length;
    
    Log_Info("Flash: Read record %lu, length %lu", record_id, header.data_length);
    
    return FLASH_OK;
}

/**
 * @brief 读取最新的N条记录
 * @param count 要读取的记录数
 * @param results 结果数组
 * @param actual_count 实际读取的记录数
 * @return FlashResult_t 操作结果
 */
FlashResult_t Flash_ReadLatestRecords(uint32_t count, ReadResult_t *results, uint32_t *actual_count)
{
    if (!g_flash_initialized) {
        return FLASH_ERROR_INIT;
    }
    
    if (results == NULL || actual_count == NULL || count == 0) {
        return FLASH_ERROR_INVALID_PARAM;
    }
    
    *actual_count = 0;
    
    /* 从缓存中获取最新的记录 */
    uint32_t start_index = 0;
    if (g_cache_count > count) {
        start_index = g_cache_count - count;
    }
    
    for (uint32_t i = start_index; i < g_cache_count && *actual_count < count; i++) {
        uint32_t record_id = g_cache_entries[i].record_id;
        
        if (Flash_ReadData(record_id, &results[*actual_count]) == FLASH_OK) {
            (*actual_count)++;
        }
    }
    
    Log_Info("Flash: Read %lu latest records", *actual_count);
    
    return FLASH_OK;
}

/**
 * @brief 加载索引表
 * @return FlashResult_t 操作结果
 */
FlashResult_t Flash_LoadIndexTable(void)
{
    Log_Info("Flash: Loading index table...");
    
    /* 清空缓存 */
    g_cache_count = 0;
    g_cache_start_id = 0;
    
    /* 从Flash索引区读取索引表 */
    uint32_t index_address = W25Q64_INDEX_AREA_START;
    uint32_t max_entries = W25Q64_INDEX_AREA_SIZE / W25Q64_INDEX_ENTRY_SIZE;
    uint32_t loaded_count = 0;
    
    for (uint32_t i = 0; i < max_entries && loaded_count < W25Q64_MAX_CACHE_ENTRIES; i++) {
        IndexEntry_t entry;
        
        if (Flash_ReadDataInternal(index_address, (uint8_t*)&entry, sizeof(IndexEntry_t)) != FLASH_OK) {
            break;
        }
        
        /* 检查索引条目是否有效 */
        if (entry.magic != W25Q64_INDEX_ENTRY_MAGIC) {
            break;
        }
        
        /* 添加到缓存 */
        Flash_AddToCache(entry.record_id, entry.flash_address, entry.data_length);
        loaded_count++;
        
        /* 更新全局变量 - 确保记录ID的连续性 */
        if (entry.record_id >= g_next_record_id) {
            g_next_record_id = entry.record_id + 1;
        }
        
        index_address += sizeof(IndexEntry_t);
    }
    
    g_total_records = loaded_count;
    
    /* 验证索引表的完整性 */
    if (loaded_count > 0) {
        /* 检查记录ID的连续性，如果有缺失则进行修复 */
        uint32_t expected_id = 1;
        bool has_gaps = false;
        
        for (uint32_t i = 0; i < g_cache_count; i++) {
            if (g_cache_entries[i].record_id != expected_id) {
                Log_Warn("Flash: Found gap in record IDs - expected %lu, found %lu", 
                        expected_id, g_cache_entries[i].record_id);
                has_gaps = true;
                break;
            }
            expected_id++;
        }
        
        if (has_gaps) {
            Log_Warn("Flash: Index table has gaps, will scan data area for complete recovery");
            /* 如果发现缺失，重新扫描数据区以确保完整性 */
            FlashResult_t scan_result = Flash_ScanDataArea();
            if (scan_result == FLASH_OK) {
                Log_Info("Flash: Data area scan completed, recovered %lu records", g_total_records);
            }
        }
    }
    
    /* 计算下一个写入地址 */
    if (loaded_count > 0) {
        /* 找到最后一个记录的位置 - 查找最大的record_id */
        uint32_t last_address = 0;
        uint32_t last_length = 0;
        uint32_t max_record_id = 0;
        
        for (uint32_t i = 0; i < g_cache_count; i++) {
            if (g_cache_entries[i].record_id > max_record_id) {
                max_record_id = g_cache_entries[i].record_id;
                last_address = g_cache_entries[i].flash_address;
                last_length = g_cache_entries[i].data_length;
            }
        }
        
        if (last_address > 0) {
            /* 计算下一个写入地址 */
            g_next_write_address = last_address + sizeof(DataHeader_t) + last_length;
            
            /* 对齐到页边界 */
            if (g_next_write_address % W25Q64_PAGE_SIZE != 0) {
                g_next_write_address = ((g_next_write_address / W25Q64_PAGE_SIZE) + 1) * W25Q64_PAGE_SIZE;
            }
            
            Log_Debug("Flash: Found last record ID: %lu at address 0x%08X, next write address: 0x%08X", 
                     max_record_id, last_address, g_next_write_address);
        }
    }
    
    Log_Info("Flash: Loaded %lu index entries, next write address: 0x%08X", 
             loaded_count, g_next_write_address);
    
    return FLASH_OK;
}

/**
 * @brief 保存索引表
 * @return FlashResult_t 操作结果
 */
FlashResult_t Flash_SaveIndexTable(void)
{
    Log_Info("Flash: Saving index table...");
    
    /* 擦除索引区 */
    if (Flash_EraseInternal(W25Q64_INDEX_AREA_START, W25Q64_INDEX_AREA_SIZE) != FLASH_OK) {
        Log_Error("Flash: Failed to erase index area");
        return FLASH_ERROR_ERASE;
    }
    
    /* 写入索引表 */
    uint32_t index_address = W25Q64_INDEX_AREA_START;
    
    for (uint32_t i = 0; i < g_cache_count; i++) {
        IndexEntry_t entry;
        entry.magic = W25Q64_INDEX_ENTRY_MAGIC;
        entry.record_id = g_cache_entries[i].record_id;
        entry.flash_address = g_cache_entries[i].flash_address;
        entry.data_length = g_cache_entries[i].data_length;
        entry.crc16 = 0;  /* 索引条目不需要CRC */
        entry.reserved = 0;
        
        if (Flash_WritePage(index_address, (uint8_t*)&entry, sizeof(IndexEntry_t)) != FLASH_OK) {
            Log_Error("Flash: Failed to write index entry %lu", i);
            return FLASH_ERROR_WRITE;
        }
        
        index_address += sizeof(IndexEntry_t);
    }
    
    Log_Info("Flash: Saved %lu index entries", g_cache_count);
    
    return FLASH_OK;
}

/**
 * @brief 扫描数据区构建索引
 * @return FlashResult_t 操作结果
 */
FlashResult_t Flash_ScanDataArea(void)
{
    Log_Info("Flash: Scanning data area...");
    
    /* 清空缓存 */
    g_cache_count = 0;
    g_cache_start_id = 0;
    g_total_records = 0;
    
    uint32_t address = W25Q64_DATA_AREA_START;
    uint32_t max_address = W25Q64_DATA_AREA_START + W25Q64_DATA_AREA_SIZE;
    uint32_t record_count = 0;
    
    while (address < max_address) {
        DataHeader_t header;
        
        /* 读取数据头 */
        if (Flash_ReadDataInternal(address, (uint8_t*)&header, sizeof(DataHeader_t)) != FLASH_OK) {
            break;
        }
        
        /* 检查是否为有效数据头 */
        if (header.magic != W25Q64_DATA_HEADER_MAGIC) {
            break;
        }
        
        /* 验证数据长度合理性 */
        if (header.data_length > 1024 || header.data_length == 0) {
            Log_Warn("Flash: Invalid data length %lu at address 0x%08X", header.data_length, address);
            break;
        }
        
        /* 添加到缓存 */
        Flash_AddToCache(header.record_id, address, header.data_length);
        record_count++;
        
        /* 更新全局变量 */
        if (header.record_id >= g_next_record_id) {
            g_next_record_id = header.record_id + 1;
        }
        
        /* 计算下一个记录地址 */
        address += sizeof(DataHeader_t) + header.data_length;
        
        /* 对齐到页边界 */
        if (address % W25Q64_PAGE_SIZE != 0) {
            address = ((address / W25Q64_PAGE_SIZE) + 1) * W25Q64_PAGE_SIZE;
        }
    }
    
    g_total_records = record_count;
    g_next_write_address = address;
    
    Log_Info("Flash: Scanned %lu records, next write address: 0x%08X", record_count, address);
    
    return FLASH_OK;
}

/**
 * @brief 内部扫描数据区
 * @return FlashResult_t 操作结果
 */
static FlashResult_t Flash_ScanDataAreaInternal(void)
{
    return Flash_ScanDataArea();
}

/**
 * @brief 获取存储信息
 * @param used_space 已使用空间
 * @param free_space 剩余空间
 * @param record_count 记录数量
 * @return FlashResult_t 操作结果
 */
FlashResult_t Flash_GetStorageInfo(uint32_t *used_space, uint32_t *free_space, uint32_t *record_count)
{
    if (!g_flash_initialized) {
        return FLASH_ERROR_INIT;
    }
    
    if (used_space == NULL || free_space == NULL || record_count == NULL) {
        return FLASH_ERROR_INVALID_PARAM;
    }
    
    *record_count = g_total_records;
    *used_space = g_next_write_address - W25Q64_DATA_AREA_START;
    *free_space = W25Q64_DATA_AREA_SIZE - *used_space;
    
    return FLASH_OK;
}

/**
 * @brief 打印状态信息
 */
void Flash_PrintStatus(void)
{
    if (!g_flash_initialized) {
        Log_Info("Flash: Not initialized");
        return;
    }
    
    uint32_t used_space, free_space, record_count;
    Flash_GetStorageInfo(&used_space, &free_space, &record_count);
    
    Log_Info("=== Flash Status ===");
    Log_Info("Initialized: Yes");
    Log_Info("Total records: %lu", record_count);
    Log_Info("Next record ID: %lu", g_next_record_id);
    Log_Info("Next write address: 0x%08X", g_next_write_address);
    Log_Info("Used space: %lu bytes", used_space);
    Log_Info("Free space: %lu bytes", free_space);
    Log_Info("Cache entries: %lu", g_cache_count);
    Log_Info("==================");
}

/**
 * @brief 打印缓存状态
 */
void Flash_PrintCacheStatus(void)
{
    Log_Info("=== Cache Status ===");
    Log_Info("Cache count: %lu", g_cache_count);
    Log_Info("Cache start ID: %lu", g_cache_start_id);
    
    for (uint32_t i = 0; i < g_cache_count; i++) {
        Log_Info("Cache[%lu]: ID=%lu, Addr=0x%08X, Len=%lu", 
                 i, g_cache_entries[i].record_id, 
                 g_cache_entries[i].flash_address, 
                 g_cache_entries[i].data_length);
    }
    Log_Info("===================");
}

/**
 * @brief Flash任务初始化
 */
void Flash_TaskInit(void)
{
    /* 初始化Flash存储系统 */
    FlashResult_t result = Flash_Init();
    if (result != FLASH_OK) {
        Log_Error("Flash Task: Initialization failed with code %d", result);
        return;
    }
}

/**
 * @brief Flash任务处理
 */
void Flash_TaskProcess(void)
{
    static uint32_t task_counter = 0;
    static uint32_t last_status_time = 0;
    
    task_counter++;
    
    /* 每10秒打印一次状态 */
    if (task_counter % 10000 == 0) {
        uint32_t current_time = osKernelGetTickCount();
        if (current_time - last_status_time >= 10000) {
            Flash_PrintStatus();
            last_status_time = current_time;
        }
    }
    
    /* 这里可以添加其他Flash任务处理逻辑 */
    /* 例如：定期保存索引表、清理过期数据等 */
    
    osDelay(1);
}

/**
 * @brief 重置SPI通信
 * @return FlashResult_t 操作结果
 */
static FlashResult_t Flash_ResetSPI(void)
{
    Log_Warn("Flash: Resetting SPI communication...");
    
    /* 确保CS引脚为高电平 */
    HAL_GPIO_WritePin(FLASH_CS_GPIO_Port, FLASH_CS_Pin, GPIO_PIN_SET);
    osDelay(10);
    
    /* 发送释放掉电命令 */
    uint8_t cmd = W25Q64_CMD_RELEASE_POWER_DOWN;
    HAL_GPIO_WritePin(FLASH_CS_GPIO_Port, FLASH_CS_Pin, GPIO_PIN_RESET);
    if (HAL_SPI_Transmit(&hspi1, &cmd, 1, 50) != HAL_OK) {
        HAL_GPIO_WritePin(FLASH_CS_GPIO_Port, FLASH_CS_Pin, GPIO_PIN_SET);
        Log_Error("Flash: Failed to send release power down command");
        return FLASH_ERROR_READ;
    }
    HAL_GPIO_WritePin(FLASH_CS_GPIO_Port, FLASH_CS_Pin, GPIO_PIN_SET);
    osDelay(10);
    
    /* 等待Flash就绪 */
    if (Flash_WaitForReady() != FLASH_OK) {
        Log_Error("Flash: Flash not ready after reset");
        return FLASH_ERROR_READ;
    }
    
    Log_Info("Flash: SPI communication reset completed");
    return FLASH_OK;
}

/**
 * @brief 从错误中恢复
 * @return FlashResult_t 操作结果
 */
static FlashResult_t Flash_RecoverFromError(void)
{
    Log_Warn("Flash: Attempting error recovery...");
    
    /* 重置SPI通信 */
    FlashResult_t result = Flash_ResetSPI();
    if (result != FLASH_OK) {
        Log_Error("Flash: SPI reset failed during recovery");
        return result;
    }
    
    /* 重新读取JEDEC ID验证通信 */
    uint32_t jedec_id;
    result = Flash_ReadJEDECID(&jedec_id);
    if (result != FLASH_OK) {
        Log_Error("Flash: Failed to read JEDEC ID during recovery");
        return result;
    }
    
    if (jedec_id != 0xEF4017) {
        Log_Error("Flash: Invalid JEDEC ID during recovery, got 0x%08X", jedec_id);
        return FLASH_ERROR_INIT;
    }
    
    Log_Info("Flash: Error recovery completed successfully");
    return FLASH_OK;
}

