/*
 * Copyright (C) OpenTX
 *
 * Based on code named
 *   th9x - http://code.google.com/p/th9x 
 *   er9x - http://code.google.com/p/er9x
 *   gruvin9x - http://code.google.com/p/gruvin9x
 *
 * License GPLv2: http://www.gnu.org/licenses/gpl-2.0.html
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

/* Includes ------------------------------------------------------------------*/
#include "opentx.h"
#if defined(SDCARD)
  #include "FatFs/diskio.h"
#endif

#if defined(__cplusplus) && !defined(SIMU)
extern "C" {
#endif

#include "usb_conf.h"

// The F0 targets drive this back end directly from their own compact mass
// storage class (f0/usb/usb_msd.c) and declare these in usb_storage.h, so the
// ST callback vtable below is not built for them.
#if !defined(STM32F0)
#include "usbd_msc_mem.h"
#else
#define USBD_STD_INQUIRY_LENGTH 36
#endif

enum MassstorageLuns {
  #if defined(SDCARD)
  STORAGE_SDCARD_LUN,
  #endif
  STORAGE_EEPROM_LUN,
  STORAGE_LUN_NBR
};

/* USB Mass storage Standard Inquiry Data */
// "extern" is needed for external linkage: a const object at namespace scope in
// C++ would otherwise be internal, and the F0 USB class reads it directly.
extern const unsigned char STORAGE_Inquirydata[] = { //36
  /* LUN 0 */
  0x00,		
  0x80,		
  0x02,		
  0x02,
  (USBD_STD_INQUIRY_LENGTH - 5),
  0x00,
  0x00,	
  0x00,
  USB_MANUFACTURER,                        /* Manufacturer : 8 bytes */
  USB_PRODUCT,                             /* Product      : 16 Bytes */
  'R', 'a', 'd', 'i', 'o', ' ', ' ', ' ',
  '1', '.', '0', '0'                      /* Version      : 4 Bytes */
#if defined(SDCARD)
  ,
  /* LUN 1 */
  0x00,		
  0x80,		
  0x02,		
  0x02,
  (USBD_STD_INQUIRY_LENGTH - 5),
  0x00,
  0x00,	
  0x00,
  USB_MANUFACTURER,                        /* Manufacturer : 8 bytes */
  USB_PRODUCT,                             /* Product      : 16 Bytes */
  'R', 'a', 'd', 'i', 'o', ' ', ' ', ' ',
  '1', '.', '0' ,'0',                      /* Version      : 4 Bytes */
#endif
};

#if defined(PCBI6X) && defined(EEPROM_RLC) && !defined(BOOT)
  #define USB_MSD_MODEL_FILES
#endif

#define FAT12_SECTORS_PER_CLUSTER 8
#define FAT12_SECTORS_PER_FAT     2
#if defined(USB_MSD_MODEL_FILES)
  #define FAT12_ROOT_ENTRIES      32
#else
  #define FAT12_ROOT_ENTRIES      16
#endif
#define FAT12_ROOT_SECTORS        (FAT12_ROOT_ENTRIES / 16)
#define RESERVED_SECTORS          (1 /*Boot*/ + FAT12_SECTORS_PER_FAT + FAT12_ROOT_SECTORS)
#define FLASH_SECTORS             (FLASHSIZE / BLOCK_SIZE)
#define FLASH_CLUSTERS            (FLASH_SECTORS / FAT12_SECTORS_PER_CLUSTER)
#if defined(EEPROM)
  #define EEPROM_SECTORS          (EEPROM_SIZE / BLOCK_SIZE)
  #define EEPROM_CLUSTERS         (EEPROM_SECTORS / FAT12_SECTORS_PER_CLUSTER)
#else
  #define EEPROM_SECTORS          0
  #define EEPROM_CLUSTERS         0
#endif
#define FIRMWARE_START_CLUSTER    2
#define EEPROM_START_CLUSTER      (FIRMWARE_START_CLUSTER + FLASH_CLUSTERS)

#if defined(USB_MSD_MODEL_FILES)
  #define BACKUP_HEADER_SIZE      8
  #define BACKUP_FOURCC          0x6978746f // "otxi"
  #define MODEL_BACKUP_DATA_MAX  0x0fff // because DirEnt.size is 12 bits
  #define MODEL_BACKUP_FILE_MAX  (BACKUP_HEADER_SIZE + MODEL_BACKUP_DATA_MAX)
  #define MODEL_CLUSTERS         ((MODEL_BACKUP_FILE_MAX + (BLOCK_SIZE * FAT12_SECTORS_PER_CLUSTER) - 1) / (BLOCK_SIZE * FAT12_SECTORS_PER_CLUSTER))
  #define MODEL_SLOT_SECTORS     (MODEL_CLUSTERS * FAT12_SECTORS_PER_CLUSTER)
  #define RADIO_CLUSTERS         1
  #define RADIO_SLOT_SECTORS     (RADIO_CLUSTERS * FAT12_SECTORS_PER_CLUSTER)
  #define RADIO_BACKUP_DATA_MAX  (RADIO_SLOT_SECTORS * BLOCK_SIZE - BACKUP_HEADER_SIZE)
  #define RADIO_START_CLUSTER    (EEPROM_START_CLUSTER + EEPROM_CLUSTERS)
  #define RADIO_START_SECTOR     (RESERVED_SECTORS + FLASH_SECTORS + EEPROM_SECTORS)
  #define MODEL_START_CLUSTER    (RADIO_START_CLUSTER + RADIO_CLUSTERS)
  #define MODEL_START_SECTOR     (RADIO_START_SECTOR + RADIO_SLOT_SECTORS)
  #define MODEL_ROOT_INDEX       4
  #define MODEL_SECTORS_TOTAL    (MAX_MODELS * MODEL_SLOT_SECTORS)
  #define RADIO_SECTORS_TOTAL    RADIO_SLOT_SECTORS
#else
  #define MODEL_SECTORS_TOTAL    0
  #define RADIO_SECTORS_TOTAL    0
#endif

#define TOTALSECTORS              (RESERVED_SECTORS + FLASH_SECTORS + EEPROM_SECTORS + RADIO_SECTORS_TOTAL + MODEL_SECTORS_TOTAL)

int32_t fat12Write(const uint8_t * buffer, uint16_t sector, uint16_t count);
int32_t fat12Read(uint8_t * buffer, uint16_t sector, uint16_t count );

int8_t STORAGE_Init (uint8_t lun);

int8_t STORAGE_GetCapacity (uint8_t lun, 
                           uint32_t *block_num, 
                           uint32_t *block_size);

int8_t  STORAGE_IsReady (uint8_t lun);

int8_t  STORAGE_IsWriteProtected (uint8_t lun);

int8_t STORAGE_Read (uint8_t lun, 
                        uint8_t *buf, 
                        uint32_t blk_addr,
                        uint16_t blk_len);

int8_t STORAGE_Write (uint8_t lun, 
                        uint8_t *buf, 
                        uint32_t blk_addr,
                        uint16_t blk_len);

int8_t STORAGE_GetMaxLun (void);

#if !defined(STM32F0)
const USBD_STORAGE_cb_TypeDef USBD_MICRO_SDIO_fops =    // modified my OpenTX
{
  STORAGE_Init,
  STORAGE_GetCapacity,
  STORAGE_IsReady,
  STORAGE_IsWriteProtected,
  STORAGE_Read,
  STORAGE_Write,
  STORAGE_GetMaxLun,
  (int8_t *)STORAGE_Inquirydata,
};

const USBD_STORAGE_cb_TypeDef  * const USBD_STORAGE_fops = &USBD_MICRO_SDIO_fops;    // modified my OpenTX
#endif // !STM32F0

#if defined(__cplusplus) && !defined(SIMU)
}
#endif

int8_t STORAGE_Init (uint8_t lun)
{
  #if defined(SDCARD)
    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel = SDIO_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority =0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
    return 0;
  #endif
/* TODO if no SD ... if( SD_Init() != 0)
  {
    return (-1); 
  } 
*/
  return (-1);
}

/**
  * @brief  return medium capacity and block size
  * @param  lun : logical unit number
  * @param  block_num :  number of physical block
  * @param  block_size : size of a physical block
  * @retval Status
  */
int8_t STORAGE_GetCapacity (uint8_t lun, uint32_t *block_num, uint32_t *block_size)
{
  if (lun == STORAGE_EEPROM_LUN) {
    *block_size = BLOCK_SIZE;
    *block_num  = TOTALSECTORS;
    return 0;
  }

  if (!SD_CARD_PRESENT())
    return -1;
#if defined(SDCARD)
  *block_size = BLOCK_SIZE;

  static DWORD sector_count = 0;
  if (sector_count == 0) {
    if (disk_ioctl(0, GET_SECTOR_COUNT, &sector_count) != RES_OK) {
      sector_count = 0;
      return -1;
    }
  }

  *block_num  = sector_count;
#endif
  return 0;
}

uint8_t lunReady[STORAGE_LUN_NBR];

void usbPluggedIn()
{
  #if defined(SDCARD)
  lunReady[STORAGE_SDCARD_LUN] = 1;
  #endif
  lunReady[STORAGE_EEPROM_LUN] = 1;
}

/**
  * @brief  check whether the medium is ready
  * @param  lun : logical unit number
  * @retval Status
  */
int8_t  STORAGE_IsReady (uint8_t lun)
{ 
#if defined(EEPROM)
  if (lun == STORAGE_EEPROM_LUN) {
    return (lunReady[STORAGE_EEPROM_LUN] != 0) ? 0 : -1;
  }
#endif
#if defined(SDCARD)
  return (lunReady[STORAGE_SDCARD_LUN] != 0 && SD_CARD_PRESENT()) ? 0 : -1;
#endif
  return 0;
}

/**
  * @brief  check whether the medium is write-protected
  * @param  lun : logical unit number
  * @retval Status
  */
int8_t  STORAGE_IsWriteProtected (uint8_t lun)
{
  return  0;
}

/**
  * @brief  Read data from the medium
  * @param  lun : logical unit number
  * @param  buf : Pointer to the buffer to save data
  * @param  blk_addr :  address of 1st block to be read
  * @param  blk_len : nmber of blocks to be read
  * @retval Status
  */

int8_t STORAGE_Read (uint8_t lun, 
                 uint8_t *buf, 
                 uint32_t blk_addr,                       
                 uint16_t blk_len)
{
  WATCHDOG_SUSPEND(100/*1s*/);
  
  if (lun == STORAGE_EEPROM_LUN) {
    return (fat12Read(buf, blk_addr, blk_len) == 0) ? 0 : -1;
  }
#if defined(SDCARD)
  // read without cache
  return (__disk_read(0, buf, blk_addr, blk_len) == RES_OK) ? 0 : -1;
#endif
  return -1;
}
/**
  * @brief  Write data to the medium
  * @param  lun : logical unit number
  * @param  buf : Pointer to the buffer to write from
  * @param  blk_addr :  address of 1st block to be written
  * @param  blk_len : nmber of blocks to be read
  * @retval Status
  */

int8_t STORAGE_Write (uint8_t lun, 
                  uint8_t *buf, 
                  uint32_t blk_addr,
                  uint16_t blk_len)
{
  WATCHDOG_SUSPEND(100/*1s*/);
  
  if (lun == STORAGE_EEPROM_LUN)	{
    return (fat12Write(buf, blk_addr, blk_len) == 0) ? 0 : -1;
  }
#if defined(SDCARD)
  // write without cache
  return (__disk_write(0, buf, blk_addr, blk_len) == RES_OK) ? 0 : -1;
#endif
  return -1;
}

/**
  * @brief  Return number of supported logical unit
  * @param  None
  * @retval number of logical unit
  */

int8_t STORAGE_GetMaxLun (void)
{
  return STORAGE_LUN_NBR - 1;
}

//------------------------------------------------------------------------------
/**
 * FAT12 boot sector partition.
 */
const char g_FATboot[62] = // [BLOCK_SIZE] - rest is generated on the fly
{
  0xeb, 0x3c, 0x90, // Jump instruction.
  'O', 'p', 'e', 'n', 'T', 'x', 0x00, 0x00, // OEM Name
  0x00, 0x02, // Bytes per sector
  0x08, // Sectors per FS cluster.
  0x01, 0x00, // Reserved sector count

  0x01, // Number of FATs
  FAT12_ROOT_ENTRIES & 0x00ff, (FAT12_ROOT_ENTRIES & 0xff00) >> 8, // Number of root directory entries
  TOTALSECTORS & 0x00ff,  (TOTALSECTORS & 0xff00) >> 8, // Total sectors
  0xf8, // Media descriptor
  FAT12_SECTORS_PER_FAT, 0x00, // Sectors per FAT table
  0x20, 0x00, // Sectors per track
  0x40, 0x00, // Number of heads
  0x00, 0x00, 0x00, 0x00, // Number of hidden sectors

  0x00, 0x00, 0x00, 0x00, // Large number of sectors.
  0x00, // Physical drive number
  0x00, // Reserved
  0x29, // Extended boot signature
  'O', 'T', 'x', 0xD1, // Disk ID (serial number)
  'V', 'I', 'R', 'T', 'F', 'S', ' ', ' ', ' ', ' ', ' ', // Volume label
  'F', 'A', 'T', '1', '2', ' ', ' ', ' ', // FAT file system type
//62
//  0x00, 0x00, // OS boot code
//// 64
//  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//// 128
//  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//// 256
//  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x55, 0xaa
};


//	File Attributes
//	0 	0x01 	Read Only.
//	1 	0x02 	Hidden.
//	2 	0x04 	System.
//	3 	0x08 	Volume Label.
//	4 	0x10 	Subdirectory.
//	5 	0x20 	Archive.
//	6 	0x40 	Device.
//	7 	0x80 	Reserved.

typedef struct
{
    uint8_t name[8];
    uint8_t ext[3];
    uint8_t attribute;
    uint8_t reserved;
    uint8_t create_time_ms;
    uint16_t create_time;
    uint16_t create_date;
    uint16_t access_date;
    uint16_t ea_index;
    uint16_t modify_time;
    uint16_t modify_date;
    uint16_t start_cluster;
    uint32_t file_size;
} FATDirEntry_t;

// First 16 FAT root directory entries (1 sector)
const FATDirEntry_t g_DIRroot[] =
{
    {
        { USB_PRODUCT },
        { ' ', ' ', ' '},
        0x08,		// Volume
        0x00,
        0x00,
        0x0000,
        0x0000,
        0x0000,
        0x0000,
        0x0000,
        0x0000,
        0x0000,
        0x00000000
    },
    {
      { 'F', 'I', 'R', 'M', 'W', 'A', 'R', 'E'},
      { 'B', 'I', 'N'},
      0x21,          // Readonly+Archive
      0x00,
      0x3E,
      0xA301,
      0x3D55,
      0x3D55,
      0x0000,
      0xA302,
      0x3D55,
      0x0002,
      FLASHSIZE
    },
#if defined(EEPROM)
    {
        { 'E', 'E', 'P', 'R', 'O', 'M', ' ', ' '},
        { 'B', 'I', 'N'},
        0x20,		// Archive
        0x00,
        0x3E,
        0xA301,
        0x3D55,
        0x3D55,
        0x0000,
        0xA302,
        0x3D55,
        EEPROM_START_CLUSTER,
        EEPROM_SIZE
    },
#endif
  // Empty entries are 0x00, omitted here. Up to 16 entries can be defined here
};

#if defined(USB_MSD_MODEL_FILES)
static uint16_t usbFileDataSize(uint8_t fileId)
{
  DirEnt & file = eeFs.files[fileId];
  return (eeFs.version == EEFS_VERS && file.startBlk) ? file.size : 0;
}

static void fillBinDirEntry(FATDirEntry_t * entry, uint16_t cluster, uint16_t size)
{
  entry->ext[0] = 'B';
  entry->ext[1] = 'I';
  entry->ext[2] = 'N';
  entry->attribute = 0x20; // Archive
  entry->start_cluster = cluster;
  entry->file_size = size ? (uint16_t)(BACKUP_HEADER_SIZE + size) : 0;
}

static void fillModelDirEntry(FATDirEntry_t * entry, uint8_t id)
{
  uint8_t num = id + 1;
  entry->name[0] = 'M';
  entry->name[1] = 'O';
  entry->name[2] = 'D';
  entry->name[3] = 'E';
  entry->name[4] = 'L';
  entry->name[5] = (num / 10) + '0';
  entry->name[6] = (num % 10) + '0';
  entry->name[7] = ' ';
  fillBinDirEntry(entry, MODEL_START_CLUSTER + id * MODEL_CLUSTERS, usbFileDataSize(FILE_MODEL(id)));
}

static void fillRadioDirEntry(FATDirEntry_t * entry)
{
  entry->name[0] = 'R';
  entry->name[1] = 'A';
  entry->name[2] = 'D';
  entry->name[3] = 'I';
  entry->name[4] = 'O';
  entry->name[5] = ' ';
  entry->name[6] = ' ';
  entry->name[7] = ' ';
  fillBinDirEntry(entry, RADIO_START_CLUSTER, usbFileDataSize(FILE_GENERAL));
}

static void fillExtraRootEntries(uint8_t * buffer, uint8_t rootSector)
{
  if (rootSector == 0) {
    fillRadioDirEntry((FATDirEntry_t *)&buffer[3 * sizeof(FATDirEntry_t)]);
  }
  for (uint8_t id = 0; id < MAX_MODELS; id++) {
    uint8_t entryIndex = MODEL_ROOT_INDEX + id;
    if (entryIndex / 16 == rootSector) {
      fillModelDirEntry((FATDirEntry_t *)&buffer[(entryIndex & 0x0f) * sizeof(FATDirEntry_t)], id);
    }
  }
}

static void readFileData(uint8_t fileId, uint16_t offset, uint8_t * buffer, uint16_t len)
{
  EFile file;
  file.openRd(fileId);

  while (offset) {
    uint8_t count = (offset > 0xff ? 0xff : offset);
    if (file.read(buffer, count) != count) {
      return;
    }
    offset -= count;
  }

  while (len) {
    uint8_t count = (len > 0xff ? 0xff : len);
    count = file.read(buffer, count);
    if (!count) {
      return;
    }
    buffer += count;
    len -= count;
  }
}

static void readBackupFile(uint8_t * buffer, uint8_t fileId, uint8_t type, uint16_t offset)
{
  uint16_t size = usbFileDataSize(fileId);
  uint16_t fileSize = size ? (uint16_t)(BACKUP_HEADER_SIZE + size) : 0;
  if (offset >= fileSize) {
    return;
  }

  uint16_t len = fileSize - offset;
  if (len > BLOCK_SIZE) {
    len = BLOCK_SIZE;
  }

  if (offset == 0) {
    *(uint32_t *)buffer = BACKUP_FOURCC;
    buffer[4] = g_eeGeneral.version;
    buffer[5] = type;
    *(uint16_t *)(buffer + 6) = size;
    buffer += BACKUP_HEADER_SIZE;
    len -= BACKUP_HEADER_SIZE;
    offset = BACKUP_HEADER_SIZE;
  }

  if (len) {
    readFileData(fileId, offset - BACKUP_HEADER_SIZE, buffer, len);
  }
}

static uint16_t writeTotal;
static uint16_t writeNext;
static uint8_t writeFileId;

static int32_t writeBackupFile(const uint8_t * buffer, uint8_t fileId, uint8_t type, uint8_t eeType, uint16_t maxSize, uint16_t offset)
{
  if (offset == 0) {
    if (eeFs.version != EEFS_VERS || *(const uint32_t *)buffer != BACKUP_FOURCC || buffer[4] != EEPROM_VER || buffer[5] != type) {
      return -1;
    }

    uint16_t size = buffer[6] | (buffer[7] << 8);
    if (size == 0 || size > maxSize) {
      return -1;
    }

    theFile.flush();
    theFile.create(fileId, eeType, true);
    writeFileId = fileId;
    writeTotal = BACKUP_HEADER_SIZE + size;
    writeNext = BACKUP_HEADER_SIZE;

    buffer += BACKUP_HEADER_SIZE;
    offset = BACKUP_HEADER_SIZE;
  }
  else if (writeNext == 0 || writeFileId != fileId || offset != writeNext) {
    return -1;
  }

  uint16_t len = BLOCK_SIZE - (offset & (BLOCK_SIZE - 1));
  if (offset + len > writeTotal) {
    len = writeTotal - offset;
  }

  while (len) {
    uint8_t count = (len > 0xff ? 0xff : len);
    theFile.write((uint8_t *)buffer, count);
    if (write_errno()) {
      ENABLE_SYNC_WRITE(false);
      writeNext = 0;
      return -1;
    }
    buffer += count;
    len -= count;
    writeNext += count;
  }

  if (writeNext >= writeTotal) {
    theFile.finishWrite();
    if (eeType == FILE_TYP_MODEL) {
      uint8_t id = fileId - FILE_MODEL(0);
      eeLoadModelHeader(id, &modelHeaders[id]);
      if (id == g_eeGeneral.currModel) {
        eeLoadModelData(id);
      }
    }
    else {
      eeLoadGeneralSettingsData();
    }
    writeNext = 0;
  }

  return 0;
}
#endif

static void writeByte(uint8_t *buffer, uint16_t sector, int byte, uint8_t value)
{
  if (byte >= sector* BLOCK_SIZE && byte < (sector+1)* BLOCK_SIZE)
    buffer[byte - sector*BLOCK_SIZE] = value;
}

static void pushCluster(uint8_t *buffer, uint16_t sector, uint16_t & cluster, int & rest, uint16_t value)
{
  // boot sector is in front of FAT
  sector= sector-1;

  // First byte of the cluster
  int startbyte = cluster *3/2;
  if (cluster % 2 == 0) {
    // First 12 bit half
    rest = value >> 8;
    writeByte(buffer, sector, startbyte, value & 0xff);
  } else {
    // second 12 bit half, write rest and next byte
    writeByte(buffer, sector, startbyte, value << 4 | rest );
    writeByte(buffer, sector, startbyte+1, (value >> 4) & 0xff);
  }
  cluster++;
}

// count is number of 512 byte sectors
int32_t fat12Read(uint8_t * buffer, uint16_t sector, uint16_t count)
{
  while(count) {
    memset(buffer, 0x00, BLOCK_SIZE);
    if (sector == 0) {
      memcpy(buffer, g_FATboot, sizeof(g_FATboot) ) ;
      // skip last 450 - 2 bytes to save flash, it's already zeroed, set only last 2 bytes
      *(uint16_t *)(buffer + BLOCK_SIZE - 2) = 0xaa55; // 2 end bytes
    }
    else if (sector == 1 || sector == 2) {
      // FAT table. Generate on the fly to save the 1024 byte flash space
      uint16_t cluster=0;
      int rest;
      pushCluster (buffer, sector, cluster, rest, (uint16_t) 0xFF8);
      pushCluster (buffer, sector, cluster, rest, (uint16_t) 0xFFF);

      // Entry for firmware.bin
      for (uint32_t i = 0; i < FLASH_CLUSTERS - 1; i++)
        pushCluster(buffer, sector, cluster, rest, cluster+1);
      pushCluster(buffer, sector, cluster, rest, (uint16_t)0xFFF);

#if defined(EEPROM)
      // Entry for eeprom.bin
      for (uint32_t i = 0; i < EEPROM_CLUSTERS - 1; i++)
        pushCluster(buffer, sector, cluster, rest, cluster+1);
      pushCluster(buffer, sector, cluster, rest, (uint16_t)0xFFF);
#endif

#if defined(USB_MSD_MODEL_FILES)
      // Entry for radio.bin
      pushCluster(buffer, sector, cluster, rest, (uint16_t)0xFFF);

      // Entries for fixed MODELxx.BIN slots
      for (uint8_t id = 0; id < MAX_MODELS; id++) {
        for (uint32_t i = 0; i < MODEL_CLUSTERS - 1; i++)
          pushCluster(buffer, sector, cluster, rest, cluster+1);
        pushCluster(buffer, sector, cluster, rest, (uint16_t)0xFFF);
      }
#endif

      // Ensure last cluster is written if it is the first half
      pushCluster (buffer, sector, cluster, rest, (uint16_t)  0x000);

      // Rest is 0x0 as per memset
    }
    else if (sector >= 1 + FAT12_SECTORS_PER_FAT && sector < RESERVED_SECTORS) {
      uint8_t rootSector = sector - (1 + FAT12_SECTORS_PER_FAT);
      if (rootSector == 0) {
        memcpy(buffer, g_DIRroot, sizeof(g_DIRroot));
      }
#if defined(USB_MSD_MODEL_FILES)
      fillExtraRootEntries(buffer, rootSector);
#endif
    }
    else if (sector < RESERVED_SECTORS + FLASH_SECTORS) {
      uint32_t address;
      address = sector - RESERVED_SECTORS;
      address *= BLOCK_SIZE;
      address += FIRMWARE_ADDRESS;
      memcpy(buffer, (uint8_t *)address, BLOCK_SIZE);
    }
#if defined(EEPROM)
    else if (sector < RESERVED_SECTORS + FLASH_SECTORS + EEPROM_SECTORS) {
      eepromReadBlock(buffer, (sector - RESERVED_SECTORS - FLASH_SECTORS)*BLOCK_SIZE, BLOCK_SIZE);
    }
#endif
#if defined(USB_MSD_MODEL_FILES)
    else if (sector < RADIO_START_SECTOR + RADIO_SLOT_SECTORS) {
      readBackupFile(buffer, FILE_GENERAL, 'G', (sector - RADIO_START_SECTOR) * BLOCK_SIZE);
    }
    else if (sector < MODEL_START_SECTOR + MODEL_SECTORS_TOTAL) {
      uint16_t modelSector = sector - MODEL_START_SECTOR;
      readBackupFile(buffer, FILE_MODEL(modelSector / MODEL_SLOT_SECTORS), 'M', (modelSector % MODEL_SLOT_SECTORS) * BLOCK_SIZE);
    }
#endif
    buffer += BLOCK_SIZE ;
    sector++ ;
    count-- ;
  }
  return 0;
}

int32_t fat12Write(const uint8_t * buffer, uint16_t sector, uint16_t count)
{
  TRACE("FAT12 Write(sector=%d, count=%d)", sector, count);

  while (count) {
    if (sector < RESERVED_SECTORS) {
      // FAT / root directory: generated on the fly, ignore host metadata writes
    }
    else if (sector < RESERVED_SECTORS + FLASH_SECTORS) {
      // firmware: read-only (flashing over USB removed), ignore writes
    }
#if defined(EEPROM)
    else if (sector < RESERVED_SECTORS + FLASH_SECTORS + EEPROM_SECTORS) {
      // EEPROM.BIN: write raw sectors through in whatever order the host sends.
      if (sector == RESERVED_SECTORS + FLASH_SECTORS && !isEepromStart(buffer)) {
        TRACE("EEPROM header mismatch in sector %d", sector);
        return -1;
      }
      eepromWriteBlock((uint8_t *)buffer, (sector - RESERVED_SECTORS - FLASH_SECTORS) * BLOCK_SIZE, BLOCK_SIZE);
    }
#endif
#if defined(USB_MSD_MODEL_FILES)
    else if (sector < RADIO_START_SECTOR + RADIO_SLOT_SECTORS) {
      if (writeBackupFile(buffer, FILE_GENERAL, 'G', FILE_TYP_GENERAL, RADIO_BACKUP_DATA_MAX, (sector - RADIO_START_SECTOR) * BLOCK_SIZE) != 0) {
        return -1;
      }
    }
    else if (sector < MODEL_START_SECTOR + MODEL_SECTORS_TOTAL) {
      uint16_t modelSector = sector - MODEL_START_SECTOR;
      if (writeBackupFile(buffer, FILE_MODEL(modelSector / MODEL_SLOT_SECTORS), 'M', FILE_TYP_MODEL, MODEL_BACKUP_DATA_MAX, (modelSector % MODEL_SLOT_SECTORS) * BLOCK_SIZE) != 0) {
        return -1;
      }
    }
#endif
    buffer += BLOCK_SIZE;
    sector++;
    count--;
  }
  return 0;
}
