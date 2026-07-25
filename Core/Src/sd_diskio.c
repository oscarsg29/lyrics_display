#include "sd_diskio.h"

#include "ff.h"
#include "diskio.h"
#include "spi.h"

#include <stdbool.h>

#define SD_DUMMY_BYTE        0xFFU
#define SD_TOKEN_START_BLOCK 0xFEU
#define SD_READY_TIMEOUT_MS  1000U
#define SD_INIT_TIMEOUT_MS   10000U

#define SD_CMD0              0U
#define SD_CMD1              1U
#define SD_CMD8              8U
#define SD_CMD9              9U
#define SD_CMD12             12U
#define SD_CMD16             16U
#define SD_CMD17             17U
#define SD_CMD24             24U
#define SD_CMD55             55U
#define SD_CMD58             58U
#define SD_ACMD41            41U

#define SD_CARD_TYPE_NONE    0x00U
#define SD_CARD_TYPE_SD1     0x01U
#define SD_CARD_TYPE_SD2     0x02U
#define SD_CARD_TYPE_BLOCK   0x04U

static DSTATUS sd_status = STA_NOINIT;
static uint8_t sd_card_type = SD_CARD_TYPE_NONE;
static uint8_t sd_last_error_step = 0U;
static uint8_t sd_last_command = 0U;
static uint8_t sd_last_command_response = 0xFFU;

static void SD_Select(void);
static void SD_Deselect(void);
static uint8_t SD_SPI_TxRx(uint8_t data);
static uint8_t SD_WaitReady(uint32_t timeout_ms);
static uint8_t SD_SendCommand(uint8_t cmd, uint32_t arg);
static uint8_t SD_SendApplicationCommand(uint8_t cmd, uint32_t arg);
static uint8_t SD_ReceiveDataBlock(uint8_t *buffer, uint16_t length);
static DRESULT SD_ReadSingleBlock(uint8_t *buffer, LBA_t sector);

void SD_SPI_Setup(void)
{
  HAL_GPIO_WritePin(SD_CS_GPIO_Port, SD_CS_Pin, GPIO_PIN_SET);
}

uint8_t SD_GetLastErrorStep(void)
{
  return sd_last_error_step;
}

uint8_t SD_GetLastCommand(void)
{
  return sd_last_command;
}

uint8_t SD_GetLastCommandResponse(void)
{
  return sd_last_command_response;
}

static void SD_Select(void)
{
  HAL_GPIO_WritePin(SD_CS_GPIO_Port, SD_CS_Pin, GPIO_PIN_RESET);
}

static void SD_Deselect(void)
{
  HAL_GPIO_WritePin(SD_CS_GPIO_Port, SD_CS_Pin, GPIO_PIN_SET);
  (void)SD_SPI_TxRx(SD_DUMMY_BYTE);
}

static uint8_t SD_SPI_TxRx(uint8_t data)
{
  uint8_t received = SD_DUMMY_BYTE;

  if (HAL_SPI_TransmitReceive(&hspi2, &data, &received, 1U, HAL_MAX_DELAY) != HAL_OK)
  {
    return SD_DUMMY_BYTE;
  }

  return received;
}

static uint8_t SD_WaitReady(uint32_t timeout_ms)
{
  uint32_t started_at = HAL_GetTick();
  uint8_t response;

  do
  {
    response = SD_SPI_TxRx(SD_DUMMY_BYTE);
    if (response == SD_DUMMY_BYTE)
    {
      return 1U;
    }
  } while ((HAL_GetTick() - started_at) < timeout_ms);

  return 0U;
}

static uint8_t SD_SendCommand(uint8_t cmd, uint32_t arg)
{
  uint8_t crc = 0x01U;
  uint8_t response;
  uint8_t actual_cmd = cmd & 0x7FU;

  if (cmd & 0x80U)
  {
    cmd &= 0x7FU;
    response = SD_SendCommand(SD_CMD55, 0U);
    if (response > 1U)
    {
      return response;
    }
  }

  sd_last_command = actual_cmd;

  SD_Deselect();
  SD_Select();

  if (!SD_WaitReady(SD_READY_TIMEOUT_MS))
  {
    SD_Deselect();
    return 0xFFU;
  }

  if (cmd == SD_CMD0)
  {
    crc = 0x95U;
  }
  else if (cmd == SD_CMD8)
  {
    crc = 0x87U;
  }

  SD_SPI_TxRx(0x40U | cmd);
  SD_SPI_TxRx((uint8_t)(arg >> 24));
  SD_SPI_TxRx((uint8_t)(arg >> 16));
  SD_SPI_TxRx((uint8_t)(arg >> 8));
  SD_SPI_TxRx((uint8_t)arg);
  SD_SPI_TxRx(crc);

  if (cmd == SD_CMD12)
  {
    SD_SPI_TxRx(SD_DUMMY_BYTE);
  }

  for (uint8_t i = 0U; i < 10U; i++)
  {
    response = SD_SPI_TxRx(SD_DUMMY_BYTE);
    if ((response & 0x80U) == 0U)
    {
      sd_last_command_response = response;
      return response;
    }
  }

  sd_last_command_response = 0xFFU;
  return 0xFFU;
}

static uint8_t SD_SendApplicationCommand(uint8_t cmd, uint32_t arg)
{
  uint8_t response = 0xFFU;

  for (uint8_t attempt = 0U; attempt < 5U; attempt++)
  {
    response = SD_SendCommand(SD_CMD55, 0U);
    if (response <= 1U)
    {
      response = SD_SendCommand(0x80U | cmd, arg);
      break;
    }

    SD_Deselect();
    HAL_Delay(10U);
  }

  return response;
}

static uint8_t SD_ReceiveDataBlock(uint8_t *buffer, uint16_t length)
{
  uint32_t started_at = HAL_GetTick();
  uint8_t token;

  do
  {
    token = SD_SPI_TxRx(SD_DUMMY_BYTE);
    if (token == SD_TOKEN_START_BLOCK)
    {
      break;
    }
  } while ((HAL_GetTick() - started_at) < SD_READY_TIMEOUT_MS);

  if (token != SD_TOKEN_START_BLOCK)
  {
    return 0U;
  }

  for (uint16_t i = 0U; i < length; i++)
  {
    buffer[i] = SD_SPI_TxRx(SD_DUMMY_BYTE);
  }

  SD_SPI_TxRx(SD_DUMMY_BYTE);
  SD_SPI_TxRx(SD_DUMMY_BYTE);

  return 1U;
}

static DRESULT SD_ReadSingleBlock(uint8_t *buffer, LBA_t sector)
{
  DRESULT result = RES_ERROR;

  if (!(sd_card_type & SD_CARD_TYPE_BLOCK))
  {
    sector *= 512U;
  }

  if (SD_SendCommand(SD_CMD17, (uint32_t)sector) == 0U)
  {
    result = SD_ReceiveDataBlock(buffer, 512U) ? RES_OK : RES_ERROR;
  }

  SD_Deselect();
  return result;
}

DSTATUS disk_initialize(BYTE pdrv)
{
  uint8_t response;
  uint8_t ocr[4];
  uint32_t started_at;

  if (pdrv != 0U)
  {
    sd_last_error_step = 1U;
    return STA_NOINIT;
  }

  sd_last_error_step = 0U;
  sd_card_type = SD_CARD_TYPE_NONE;
  SD_Deselect();
  HAL_Delay(100U);

  for (uint8_t i = 0U; i < 10U; i++)
  {
    SD_SPI_TxRx(SD_DUMMY_BYTE);
  }

  if (SD_SendCommand(SD_CMD0, 0U) != 1U)
  {
    SD_Deselect();
    sd_last_error_step = 2U;
    sd_status = STA_NOINIT;
    return sd_status;
  }

  response = SD_SendCommand(SD_CMD8, 0x1AAU);
  if (response == 1U)
  {
    for (uint8_t i = 0U; i < 4U; i++)
    {
      ocr[i] = SD_SPI_TxRx(SD_DUMMY_BYTE);
    }

    if (ocr[2] != 0x01U || ocr[3] != 0xAAU)
    {
      SD_Deselect();
      sd_last_error_step = 3U;
      sd_status = STA_NOINIT;
      return sd_status;
    }

    started_at = HAL_GetTick();
    do
    {
      response = SD_SendApplicationCommand(SD_ACMD41, 0x40000000U);
    } while (response != 0U && ((HAL_GetTick() - started_at) < SD_INIT_TIMEOUT_MS));

    if (response == 0U && SD_SendCommand(SD_CMD58, 0U) == 0U)
    {
      for (uint8_t i = 0U; i < 4U; i++)
      {
        ocr[i] = SD_SPI_TxRx(SD_DUMMY_BYTE);
      }
      sd_card_type = SD_CARD_TYPE_SD2 | ((ocr[0] & 0x40U) ? SD_CARD_TYPE_BLOCK : 0U);
    }
    else
    {
      sd_last_error_step = 4U;
    }
  }
  else
  {
    bool use_acmd41 = true;

    if (response != 0x05U)
    {
      use_acmd41 = false;
    }

    started_at = HAL_GetTick();
    do
    {
      response = use_acmd41 ? SD_SendApplicationCommand(SD_ACMD41, 0U) : SD_SendCommand(SD_CMD1, 0U);
    } while (response != 0U && ((HAL_GetTick() - started_at) < SD_INIT_TIMEOUT_MS));

    if (response == 0U && SD_SendCommand(SD_CMD16, 512U) == 0U)
    {
      sd_card_type = SD_CARD_TYPE_SD1;
    }
  }

  SD_Deselect();

  sd_status = (sd_card_type == SD_CARD_TYPE_NONE) ? STA_NOINIT : 0U;
  if (sd_status & STA_NOINIT)
  {
    sd_last_error_step = 5U;
  }
  return sd_status;
}

DSTATUS disk_status(BYTE pdrv)
{
  if (pdrv != 0U)
  {
    return STA_NOINIT;
  }

  return sd_status;
}

DRESULT disk_read(BYTE pdrv, BYTE* buff, LBA_t sector, UINT count)
{
  if (pdrv != 0U || buff == NULL || count == 0U)
  {
    return RES_PARERR;
  }

  if (sd_status & STA_NOINIT)
  {
    return RES_NOTRDY;
  }

  while (count-- > 0U)
  {
    if (SD_ReadSingleBlock(buff, sector++) != RES_OK)
    {
      return RES_ERROR;
    }
    buff += 512U;
  }

  return RES_OK;
}

DRESULT disk_write(BYTE pdrv, const BYTE* buff, LBA_t sector, UINT count)
{
  (void)pdrv;
  (void)buff;
  (void)sector;
  (void)count;
  return RES_WRPRT;
}

DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void* buff)
{
  if (pdrv != 0U)
  {
    return RES_PARERR;
  }

  if (sd_status & STA_NOINIT)
  {
    return RES_NOTRDY;
  }

  switch (cmd)
  {
    case CTRL_SYNC:
      return SD_WaitReady(SD_READY_TIMEOUT_MS) ? RES_OK : RES_ERROR;

    case GET_SECTOR_SIZE:
      *(WORD*)buff = 512U;
      return RES_OK;

    case GET_BLOCK_SIZE:
      *(DWORD*)buff = 1U;
      return RES_OK;

    default:
      return RES_PARERR;
  }
}
