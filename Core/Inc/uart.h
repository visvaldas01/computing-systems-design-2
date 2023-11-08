#ifndef _UART_DRIVER
#define _UART_DRIVER

#include "main.h"
#include <stdint.h>

typedef enum uart_driver_state {
	READY,
	BUSY_SENDING,
	BUSY_RECEIVING,
	TIMEOUT
} UART_State;

typedef enum uart_driver_mode {
	INT_DISABLE,
	INT_ENABLE
} UART_Mode;

typedef enum uart_driver_ret_code {
	UART_SUCCESS,
	BUFFER_FULL,
	BUFFER_EMPTY,
	UART_ERROR
} UART_Ret_Code;

void UART_Set_Mode(UART_Mode mode);
void UART_Init(UART_HandleTypeDef * interface);
UART_Ret_Code UART_Read_Data(uint8_t *const pData, const uint16_t Size);
UART_Ret_Code UART_Send_Data(uint8_t const * const pData, const uint16_t Size);
UART_State UART_Get_State();

#endif
