#ifndef __SD_DISKIO_H__
#define __SD_DISKIO_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

void SD_SPI_Setup(void);
uint8_t SD_GetLastErrorStep(void);
uint8_t SD_GetLastCommand(void);
uint8_t SD_GetLastCommandResponse(void);

#ifdef __cplusplus
}
#endif

#endif /* __SD_DISKIO_H__ */
