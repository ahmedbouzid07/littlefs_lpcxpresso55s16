/*
 * McuFlash.c
 *
 *  Created on: Jun 20, 2023
 *      Author: ahmed
 */

#include "McuLib.h"
#include "McuFlash.h"
#include "fsl_iap.h"

static flash_config_t s_flashDriver;


bool McuFlash_IsAccessible(const void *addr, size_t nofBytes) {
	status_t status;
	status = FLASH_VerifyErase(&s_flashDriver, (uint32_t)addr, nofBytes);
	if (status==kStatus_Success) {
		return false; /* if it is an erased FLASH: accessing it will cause a hard fault! */
	}
	return true;
}

bool McuFlash_IsErased(const void *addr, size_t nofBytes) {
	status_t status;
	status = FLASH_VerifyErase(&s_flashDriver, (uint32_t)addr, nofBytes);
	return status==kStatus_Success;  /* true if it is an erased FLASH: accessing it will cause a hard fault! */

}

uint8_t McuFlash_Read(const void *addr, void *data, size_t dataSize) {
	if (!McuFlash_IsAccessible(addr, dataSize)) {
		memset(data, 0xff, dataSize);
		return ERR_FAULT;
	}

	status_t status;
	status = FLASH_Read(&s_flashDriver, (uint32_t)addr, data, (uint32_t)dataSize);
	if(status != kStatus_Success){
		return ERR_FAULT;
	}
	return ERR_OK;
}

static uint8_t McuFlash_ProgramPage(void *addr, const void *data, size_t dataSize) {
	status_t status;
	uint32_t failedAddress, failedData;
	if (((uint32_t)addr%s_flashDriver.PFlashPageSize) != 0) {
		return ERR_FAILED;
	}
	if (dataSize!=s_flashDriver.PFlashPageSize) { /* must match flash page size! */
		return ERR_FAILED;
	}
	/* erase first */
	status = FLASH_Erase(&s_flashDriver, (uint32_t)addr, dataSize, kFLASH_ApiEraseKey);
	if (status!=kStatus_Success ) {
		return ERR_FAILED;
	}
	/* check if it is erased */
	status = FLASH_VerifyErase(&s_flashDriver, (uint32_t)addr, dataSize);
	if (status!=kStatus_Success) {
		return ERR_FAILED;
	}
	status = FLASH_Program(&s_flashDriver, (uint32_t)addr, (uint8_t*)data, dataSize);
	if (status!=kStatus_Success) {
		return ERR_FAILED;
	}
	status = FLASH_VerifyProgram(&s_flashDriver, (uint32_t)addr, dataSize, (const uint8_t *)data, &failedAddress, &failedData);
	if (status!=kStatus_Success) {
		return ERR_FAILED;
	}
	return ERR_OK;
}

uint8_t McuFlash_Program(void *addr, const void *data, size_t dataSize) {
	if (((uint32_t)addr%McuFlash_CONFIG_FLASH_BLOCK_SIZE) != 0 || (dataSize!=McuFlash_CONFIG_FLASH_BLOCK_SIZE)) {
		/* address and size not aligned to page boundaries: make backup into buffer */
		uint8_t buffer[McuFlash_CONFIG_FLASH_BLOCK_SIZE];
		uint8_t res;
		size_t offset, remaining, size;
		uint32_t pageAddr; /* address of page */

		pageAddr = ((uint32_t)addr/McuFlash_CONFIG_FLASH_BLOCK_SIZE)*McuFlash_CONFIG_FLASH_BLOCK_SIZE;
		offset = (uint32_t)addr%McuFlash_CONFIG_FLASH_BLOCK_SIZE; /* offset inside page */
		remaining = dataSize;

		while (remaining>0) {
			res = McuFlash_Read((void*)pageAddr, buffer, sizeof(buffer)); /* read current flash content */
			if (res!=ERR_OK) {
				return ERR_FAILED;
			}
			if (offset+remaining>McuFlash_CONFIG_FLASH_BLOCK_SIZE) {
				size = McuFlash_CONFIG_FLASH_BLOCK_SIZE-offset; /* how much we can copy in this step */
			}
			else {
				size = remaining;
			}
			memcpy(buffer+offset, data, size); /*  merge original page with new data */
			/* program new data/page */
			res = McuFlash_ProgramPage((void*)pageAddr, buffer, sizeof(buffer));
			if (res!=ERR_OK) {
				return ERR_FAILED;
			}
			pageAddr += McuFlash_CONFIG_FLASH_BLOCK_SIZE;
			offset = 0;
			data += size;
			remaining -= size;
		}
		return res;
	}
	else { /* a full page to program */
		return McuFlash_ProgramPage(addr, data, dataSize);
	}
}

uint8_t McuFlash_InitErase(void *addr, size_t nofBytes) {
	/* LPC55Sxx specific: erases the memory, makes it inaccessible */
	status_t status;
	if ((nofBytes%McuFlash_CONFIG_FLASH_BLOCK_SIZE)!=0) { /* check if size is multiple of page size */
		return ERR_FAILED;
	}
	for(int i=0; i<nofBytes/McuFlash_CONFIG_FLASH_BLOCK_SIZE; i++) { /* erase and program each page */
		/* erase each page */
		status = FLASH_Erase(&s_flashDriver, (uint32_t)addr+i*McuFlash_CONFIG_FLASH_BLOCK_SIZE, McuFlash_CONFIG_FLASH_BLOCK_SIZE, kFLASH_ApiEraseKey);
		if (status!=kStatus_Success ) {
			return ERR_FAILED;
		}
	}
	return ERR_OK;
}

uint8_t McuFlash_Erase(void *addr, size_t nofBytes) {
	static const uint8_t zeroBuffer[McuFlash_CONFIG_FLASH_BLOCK_SIZE]; /* initialized with zeros, buffer in FLASH to save RAM */
	uint8_t res;
	if ((nofBytes%McuFlash_CONFIG_FLASH_BLOCK_SIZE)!=0) { /* check if size is multiple of page size */
		return ERR_FAILED;
	}
	for(int i=0; i<nofBytes/McuFlash_CONFIG_FLASH_BLOCK_SIZE; i++) { /* erase and program each page */
		res = McuFlash_Program(addr+i*McuFlash_CONFIG_FLASH_BLOCK_SIZE, zeroBuffer, sizeof(zeroBuffer));
		if (res!=ERR_OK) {
			return res;
		}
	}
	return res;
}

static uint8_t ReadData(void *hndl, uint32_t addr, uint8_t *buf, size_t bufSize) {
  (void)hndl; /* not used */
  if (!McuFlash_IsAccessible((void*)addr, bufSize)) {
    memset(buf, 0xff, bufSize);
    return ERR_FAILED;
  }
  memcpy(buf, (void*)addr, bufSize);
  return ERR_OK;
}

void McuFlash_Deinit(void) {
}

void McuFlash_Init(void) {
	status_t result;    /* Return code from each flash driver function */
	memset(&s_flashDriver, 0, sizeof(flash_config_t));
	result = FLASH_Init(&s_flashDriver);
	if (result!=kStatus_Success) {
		for(;;) { /* error */ }
	}
}
