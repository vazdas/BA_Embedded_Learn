#ifndef EEPROM_H
#define EEPROM_H

#include <stdint.h>
#include "main.h"

#define EEPROM_ADDR 0xA0
#define MEM_ADDR 0x00

#ifdef __cplusplus
extern "C" {
#endif

void EEPROM_Write_uint16(uint16_t data);
uint16_t EEPROM_Read_uint16(void);

#ifdef __cplusplus
}
#endif

#endif
