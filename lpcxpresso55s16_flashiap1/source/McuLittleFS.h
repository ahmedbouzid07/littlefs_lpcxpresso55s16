/*
 * McuLittleFS.h
 *
 *  Created on: Jun 21, 2023
 *      Author: ahmed
 */

#ifndef MCULITTLEFS_H_
#define MCULITTLEFS_H_

#include "lfs.h"


bool McuLFS_IsMounted(void);
lfs_t* McuLFS_GetFileSystem(void);

uint8_t McuLFS_ReadFile(lfs_file_t* file, bool readFromBeginning, size_t nofBytes);
uint8_t McuLFS_FileList(const char *path);
uint8_t McuLFS_RemoveFile(const char *filePath);
uint8_t McuLFS_MoveFile(const char *srcPath, const char *dstPath);

uint8_t McuLFS_Mount();
uint8_t McuLFS_Unmount();

uint8_t McuLFS_openFile(lfs_file_t* file,uint8_t* filename);
uint8_t McuLFS_closeFile(lfs_file_t* file);
uint8_t McuLFS_writeLine(lfs_file_t* file,uint8_t* line);
uint8_t McuLFS_readLine(lfs_file_t* file,uint8_t* lineBuf,size_t bufSize,uint8_t* nofReadChars);

/* Functions ported from FatFS (Used by MiniIni) */
char* McuLFS_gets (char* buff,int len, lfs_file_t* fp);
int McuLFS_puts (const char* str, lfs_file_t* fp);

#endif /* MCULITTLEFS_H_ */
