/*
 * McuFlash.h
 *
 *  Created on: Jun 20, 2023
 *      Author: ahmed
 */

#ifndef MCUFLASH_H_
#define MCUFLASH_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define McuFlash_CONFIG_FLASH_BLOCK_SIZE         (0x200)

/*!
 * \brief Decides if memory is accessible. On some architectures it needs to be prepared first.
 * \param addr Memory area to check
 * \param nofBytes Number of bytes to check
 * \return true if memory can be accessed, false otherwise
 */
bool McuFlash_IsAccessible(const void *addr, size_t nofBytes);

/*!
 * \brief Erases a memory area
 * \param addr Memory area to erase
 * \param nofBytes Number of bytes to erase
 * \return Error code, ERR_OK if everything is fine
 */
uint8_t McuFlash_Erase(void *addr, size_t nofBytes);


/*!
 * \brief For LPC55Sxx only: initializes memory with an erase, making it inaccessible
 * \param addr Start address of memory, must be 0x200 aligned
 * \param nofBytes Number of bytes, must be multiple if 0x200
 * \return Error code, ERR_OK if everything is fine
 */
uint8_t McuFlash_InitErase(void *addr, size_t nofBytes);

/*!
 * \brief Program the flash memory with data
 * \param addr Address where to store the data
 * \param data Pointer to the data
 * \param dataSize Number of data bytes
 * \return Error code, ERR_OK if everything is fine
 */
uint8_t McuFlash_Program(void *addr, const void *data, size_t dataSize);

/*!
 * \brief Read the flash memory
 * \param addr Address where to store the data
 * \param data Pointer where to store the data
 * \param dataSize Number of data bytes
 * \return Error code, ERR_OK if everything is fine
 */
uint8_t McuFlash_Read(const void *addr, void *data, size_t dataSize);


/*!
 * \brief Module de-initialization
 */
void McuFlash_Deinit(void);

/*!
 * \brief Module initialization
 */
void McuFlash_Init(void);

#ifdef __cplusplus
}  /* extern "C" */
#endif


#endif /* MCUFLASH_H_ */
