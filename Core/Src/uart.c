#include "uart.h"
#include "main.h"
#include <string.h>
#include <stdbool.h>

#define BUFFER_SIZE 2048

static UART_Mode uart_mode;

static UART_HandleTypeDef *huart6;

static uint8_t send_buffer[BUFFER_SIZE];
static uint16_t send_buffer_start;
static uint16_t send_buffer_end;

static uint8_t receive_buffer[BUFFER_SIZE];
static uint16_t receive_buffer_start;
static uint16_t receive_buffer_end;
uint8_t receive_cell;
uint8_t transmit_cell;

bool push_to_send_buffer(uint8_t* data, uint16_t len) {
	if (BUFFER_SIZE - send_buffer_end < len) {
		return false;
	}
	memcpy(send_buffer + send_buffer_end, data, len);
	send_buffer_end += len;
	return true;
}

static bool get_from_send_buffer(uint8_t* data) {
	if (send_buffer_end > 0) {
		*data = send_buffer[send_buffer_start];
		send_buffer_start += 1;
		if (send_buffer_start == send_buffer_end) {
			send_buffer_start = 0;
			send_buffer_end = 0;
		}
		return true;
	}
	return false;
}

bool receive_buffer_has_data(uint16_t len) {
	return receive_buffer_end - receive_buffer_start >= len;
}

bool receive_buffer_get_data(uint8_t* data, uint16_t len) {
	if (!receive_buffer_has_data(len)) return false;
	memcpy(data, receive_buffer + receive_buffer_start, len);
	receive_buffer_start += len;
	if (receive_buffer_start == receive_buffer_end) {
		receive_buffer_start = 0;
		receive_buffer_end = 0;
	}
	return true;
}

static bool push_to_receive_buffer(uint8_t* data, uint16_t len) {
	if (BUFFER_SIZE - receive_buffer_end < len) {
		return false;
	}
	memcpy(receive_buffer + receive_buffer_end, data, len);
	receive_buffer_end += len;
	return true;
}

void UART_Set_Mode(UART_Mode mode) {
	if (mode == INT_DISABLE) {
		HAL_UART_Abort_IT(huart6);
		HAL_NVIC_DisableIRQ(USART6_IRQn);
	}
	if (mode == INT_ENABLE) {
		HAL_NVIC_EnableIRQ(USART6_IRQn);
		HAL_UART_Receive_IT(huart6, &receive_cell, 1);
	}
	uart_mode = mode;
}

void UART_Init(UART_HandleTypeDef * interface) {
	huart6 = interface;
}

UART_Ret_Code UART_Read_Data_Int_Disable(uint8_t *const pData, const uint16_t Size) {
	if (HAL_UART_Receive(huart6, pData, Size, 3) == HAL_OK) {
	    return UART_SUCCESS;
	}
	return BUFFER_EMPTY;
};

UART_Ret_Code UART_Read_Data_Int_Enable(uint8_t *const pData, const uint16_t Size) {
	if (receive_buffer_has_data(Size)) {
		receive_buffer_get_data(pData, Size);
	    return UART_SUCCESS;
	}
	return BUFFER_EMPTY;
};

UART_Ret_Code UART_Send_Data_Int_Disable(uint8_t *const pData, const uint16_t Size) {
	if (HAL_UART_Transmit(huart6, pData, Size, 1000) == HAL_OK) {
	    return UART_SUCCESS;
	}
	return UART_ERROR;
};

UART_Ret_Code UART_Send_Data_Int_Enable(uint8_t *const pData, const uint16_t Size) {
	if (!push_to_send_buffer(pData, Size)) {
		return BUFFER_FULL;
	}
	HAL_UART_StateTypeDef state = HAL_UART_GetState(huart6);
	if (state == HAL_UART_STATE_BUSY || state == HAL_UART_STATE_BUSY_TX || state == HAL_UART_STATE_BUSY_TX_RX) {
		return UART_SUCCESS;
	}
	get_from_send_buffer(&transmit_cell);
	HAL_UART_Transmit_IT(huart6, &transmit_cell, 1);
	return UART_SUCCESS;
};

UART_Ret_Code UART_Read_Data(uint8_t *const pData, const uint16_t Size) {
	if (uart_mode == INT_DISABLE) {
		return UART_Read_Data_Int_Disable(pData, Size);
	}
	if (uart_mode == INT_ENABLE) {
		return UART_Read_Data_Int_Enable(pData, Size);
	}
	return UART_ERROR;
}

UART_Ret_Code UART_Send_Data(uint8_t const * const pData, const uint16_t Size) {
	if (uart_mode == INT_DISABLE) {
		return UART_Send_Data_Int_Disable(pData, Size);
	}
	if (uart_mode == INT_ENABLE) {
		return UART_Send_Data_Int_Enable(pData, Size);
	}
	return UART_ERROR;
}

UART_State UART_Get_State() {
	HAL_UART_StateTypeDef state = HAL_UART_GetState(huart6);
	bool busy_sending = state == HAL_UART_STATE_BUSY_TX;
	busy_sending |= state == HAL_UART_STATE_BUSY_TX_RX;
	bool busy_receiving = state == HAL_UART_STATE_BUSY_RX;
	busy_receiving |= state == HAL_UART_STATE_BUSY_TX_RX;
	if (busy_sending) return BUSY_SENDING;
	if (busy_receiving) return BUSY_RECEIVING;
	if (state == HAL_UART_STATE_TIMEOUT) return TIMEOUT;
	return READY;
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
	push_to_receive_buffer(&receive_cell, 1);
	HAL_UART_Receive_IT(huart6, &receive_cell, 1);
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
	if (get_from_send_buffer(&transmit_cell)) {
		HAL_UART_Transmit_IT(huart6, &transmit_cell, 1);
	}
}
