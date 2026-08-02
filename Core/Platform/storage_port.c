#include "storage_port.h"

#include "sd_diskio.h"

void StoragePort_Setup(void)
{
  SD_SPI_Setup();
}

uint8_t StoragePort_GetLastErrorStep(void)
{
  return SD_GetLastErrorStep();
}

uint8_t StoragePort_GetLastCommand(void)
{
  return SD_GetLastCommand();
}

uint8_t StoragePort_GetLastCommandResponse(void)
{
  return SD_GetLastCommandResponse();
}
