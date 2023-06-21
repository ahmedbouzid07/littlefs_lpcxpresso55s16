/*
 * McuLittleFSBlockDevice.c
 *
 *  Created on: Jun 21, 2023
 *      Author: ahmed
 */
#include "lfs.h"
#include "McuLib.h"
#include "McuFlash.h"
#include "McuLittleFSconfig.h"
#include "McuLittleFSBlockDevice.h"

int McuLittleFS_block_device_read(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size) {
  uint8_t res;
  res = McuFlash_Read((void*)((block+McuLittleFS_CONFIG_BLOCK_OFFSET) * c->block_size + off), buffer, size);
  if (res != ERR_OK) {
	  return LFS_ERR_IO;
  }
  return LFS_ERR_OK;
}

int McuLittleFS_block_device_prog(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size) {
  uint8_t res;
  res = McuFlash_Program((void*)((block+McuLittleFS_CONFIG_BLOCK_OFFSET) * c->block_size + off), buffer, size);
  if (res != ERR_OK) {
    return LFS_ERR_IO;
  }
  return LFS_ERR_OK;
}

int McuLittleFS_block_device_erase(const struct lfs_config *c, lfs_block_t block) {
  uint8_t res;
  res = McuFlash_Erase((void*)((block+McuLittleFS_CONFIG_BLOCK_OFFSET) * c->block_size), c->block_size);
  if (res != ERR_OK) {
    return LFS_ERR_IO;
  }
  return LFS_ERR_OK;
}

int McuLittleFS_block_device_sync(const struct lfs_config *c) {

	return LFS_ERR_OK;
}

int McuLittleFS_block_device_deinit(void) {
	return LFS_ERR_OK;
}

int McuLittleFS_block_device_init(void) {

	McuFlash_Init();
	return LFS_ERR_OK;
}
