/*
 * McuLittleFSBlockDevice.h
 *
 *  Created on: Jun 21, 2023
 *      Author: ahmed
 */

#ifndef MCULITTLEFSBLOCKDEVICE_H_
#define MCULITTLEFSBLOCKDEVICE_H_


#include <stdint.h>
#include "lfs.h"



int McuLittleFS_block_device_read(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size);

int McuLittleFS_block_device_prog(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size);

int McuLittleFS_block_device_erase(const struct lfs_config *c, lfs_block_t block);

int McuLittleFS_block_device_sync(const struct lfs_config *c);

int McuLittleFS_block_device_deinit(void);

int McuLittleFS_block_device_init(void);

#endif /* MCULITTLEFSBLOCKDEVICE_H_ */
