#include "btn.h"
#include "stm32f4xx_hal.h"

#include <stdint.h>

enum internal_state {
	INTERNAL_NONE,
	INTERNAL_FILTERING,
	INTERNAL_WAITING,
	INTERNAL_LONG,
	INTERNAL_SHORT
};

#define SHORT_THRESHOLD 10
#define LONG_THRESHOLD 500

static _Bool btn_pressed() {
	return HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_15) == GPIO_PIN_RESET;
}

static struct btn_driver {
	enum internal_state state;
	uint32_t state_time;
	uint32_t previous_measured_time;
} driver;

static void process() {
	enum internal_state next_state;
	switch (driver.state) {
	case INTERNAL_NONE:
		if (btn_pressed()) {
			next_state = INTERNAL_FILTERING;
		} else {
			next_state = INTERNAL_NONE;
		}
		break;
	case INTERNAL_FILTERING:
		if (driver.state_time < SHORT_THRESHOLD) {
		    next_state = INTERNAL_FILTERING;
		} else {
			if (btn_pressed()) {
				next_state = INTERNAL_WAITING;
			} else {
				next_state = INTERNAL_NONE;
			}
		}
		break;
	case INTERNAL_WAITING:
		if (btn_pressed()) {
			next_state = INTERNAL_WAITING;
		} else {
			if (driver.state_time < LONG_THRESHOLD) {
				next_state = INTERNAL_SHORT;
			} else {
				next_state = INTERNAL_LONG;
			}
		}
		break;
	case INTERNAL_LONG:
		next_state = INTERNAL_NONE;
		break;
	case INTERNAL_SHORT:
		next_state = INTERNAL_NONE;
		break;
	}
	if (next_state != driver.state) {
		driver.state = next_state;
		driver.state_time = 0;
	} else {
		driver.state_time += HAL_GetTick() - driver.previous_measured_time;
	}
	driver.previous_measured_time = HAL_GetTick();
}

enum ButtonState Get_Button_State() {
	process();
	if (driver.state == INTERNAL_LONG) return LONG;
	if (driver.state == INTERNAL_SHORT) return SHORT;
	return NONE;
}
