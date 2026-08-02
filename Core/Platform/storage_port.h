#ifndef STORAGE_PORT_H
#define STORAGE_PORT_H

#include <stdint.h>

void StoragePort_Setup(void);
uint8_t StoragePort_GetLastErrorStep(void);
uint8_t StoragePort_GetLastCommand(void);
uint8_t StoragePort_GetLastCommandResponse(void);

#endif
