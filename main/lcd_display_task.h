#ifndef LCD_DISPLAY_TASK_H
#define LCD_DISPLAY_TASK_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl.h"

// Function declarations
void lcd_display_init(void);
void lcd_display_start(void);

#endif // LCD_DISPLAY_TASK_H 