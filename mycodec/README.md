# STM32F103VET6 Log System User Guide

## Overview
This log system is implemented based on UART1 DMA + Queue + Log Task for printing program runtime information.

## File Structure
- `mycodeh/log.h` - Log system header file
- `mycodec/log.c` - Log system source file
- `Core/Src/freertos.c` - LogTask function implementation
- `Core/Src/main.c` - Test code

## Features
1. **Multi-level logging**: ERROR, WARN, INFO, DEBUG four levels
2. **Timestamp**: Each log contains system runtime timestamp
3. **Formatted output**: Supports printf-style formatted strings
4. **Queue buffering**: Uses FreeRTOS message queue to buffer log messages
5. **DMA transmission**: Uses UART1 DMA for efficient data transmission
6. **Level filtering**: Can dynamically set log output level

## Usage

### 1. Basic Log Output
```c
#include "log.h"

// Different level log outputs
Log_Error("Error message: %s", error_msg);
Log_Warn("Warning message: Temperature too high");
Log_Info("System startup completed");
Log_Debug("Debug info: Variable value = %d", value);
```

### 2. Generic Log Output
```c
Log_Print(LOG_LEVEL_INFO, "Generic log: %d", value);
```

### 3. Log Level Control
```c
// Set log level
Log_SetLevel(LOG_LEVEL_INFO);  // Only output INFO and above levels

// Get current log level
LogLevel_t level = Log_GetLevel();
```

## Output Format
Log output format:
```
[timestamp] level: message content
```

For example:
```
[  123] INFO : System startup completed
[  456] DEBUG: Debug info: Variable value = 100
[  789] ERROR: Error message: Memory insufficient
```

## Configuration
- **UART Configuration**: 115200 baud rate, 8 data bits, 1 stop bit, no parity
- **DMA Channel**: DMA1_Channel4 (USART1_TX)
- **Queue Size**: 10 messages, 64 bytes each
- **Task Stack Size**: 512*4 = 2048 bytes
- **Task Priority**: Normal priority

## Notes
1. Ensure FreeRTOS scheduler is started before calling log functions
2. Maximum log message length is 56 bytes (including terminator)
3. If queue is full, new messages will be discarded
4. Log task will block until DMA transmission completes
5. Timestamp unit is milliseconds (based on osKernelGetTickCount)

## Test Code
main.c contains complete test code demonstrating:
- Output of different level logs
- Dynamic log level switching
- Usage of formatted strings
- Cyclic log output

After compiling and running the program, you can view log output through serial debug assistant.
