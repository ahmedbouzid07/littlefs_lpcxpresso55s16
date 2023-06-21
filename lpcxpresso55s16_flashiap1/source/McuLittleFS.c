/*
 * McuLittleFS.c
 *
 *  Created on: Jun 21, 2023
 *      Author: ahmed
 */
#include "McuLittleFS.h"
#include "McuLittleFSconfig.h"
#include "McuLittleFSBlockDevice.h"
#include "lfs.h"
#include "McuFlash.h"
#include "McuLib.h"

#include <stdio.h>
#include <string.h>
#include <stdint.h>


/* variables used by the file system */
static bool McuLFS_isMounted = FALSE;
static lfs_t McuLFS_lfs;

bool McuLFS_IsMounted(void) {
  return McuLFS_isMounted;
}

/* configuration of the file system is provided by this struct */
static const struct lfs_config McuLFS_cfg = {
  .context = NULL,
  /* block device operations */
  .read = McuLittleFS_block_device_read,
  .prog = McuLittleFS_block_device_prog,
  .erase = McuLittleFS_block_device_erase,
  .sync = McuLittleFS_block_device_sync,
  /* block device configuration */
  .read_size = McuLittleFS_CONFIG_FILESYSTEM_READ_BUFFER_SIZE,
  .prog_size = McuLittleFS_CONFIG_FILESYSTEM_PROG_BUFFER_SIZE,
  .block_size = McuLittleFS_CONFIG_BLOCK_SIZE,
  .block_count = McuLittleFS_CONFIG_BLOCK_COUNT,
  .cache_size = McuLittleFS_CONFIG_FILESYSTEM_CACHE_SIZE,
  .lookahead_size = McuLittleFS_CONFIG_FILESYSTEM_LOOKAHEAD_SIZE,
  .block_cycles = 500,
};

/*-----------------------------------------------------------------------
 * Get a string from the file
 * (ported from FatFS function: f_gets())
 *-----------------------------------------------------------------------*/

char* McuLFS_gets (
  char* buff,  /* Pointer to the string buffer to read */
  int len,      /* Size of string buffer (characters) */
  lfs_file_t* fp       /* Pointer to the file object */
)
{
	int n = 0;
	char c, *p = buff;
	unsigned char s[2];
	uint32_t rc;
	while (n < len - 1) { /* Read characters until buffer gets filled */
		rc = lfs_file_read(&McuLFS_lfs,fp,s,1);
		if (rc != 1) break;
	    c = s[0];

	    if (c == '\r') continue; /* Strip '\r' */
	    *p++ = c;
	    n++;
	    if (c == '\n') break;   /* Break on EOL */
	  }
	*p = 0;
	return n ? buff : 0;      /* When no data read (eof or error), return with error. */
}

/*-----------------------------------------------------------------------*
 * Put a character to the file
 * (ported from FatFS)
 *-----------------------------------------------------------------------*/

typedef struct putbuff {
  lfs_file_t* fp ;
  int idx, nchr;
  unsigned char buf[64];
} putbuff;

static void putc_bfd (putbuff* pb, char c) {
  uint32_t bw;
  int32_t i;

  if (c == '\n') {  /* LF -> CRLF conversion */
    putc_bfd(pb, '\r');
  }

  i = pb->idx;  /* Buffer write index (-1:error) */
  if (i < 0) {
    return;
  }
  pb->buf[i++] = (unsigned char)c;

  if (i >= (int)(sizeof pb->buf) - 3) { /* Write buffered characters to the file */
  bw = lfs_file_write(&McuLFS_lfs,pb->fp, pb->buf,(uint32_t)i);
    i = (bw == (uint32_t)i) ? 0 : -1;
  }
  pb->idx = i;
  pb->nchr++;
}

/*-----------------------------------------------------------------------*/
/* Put a string to the file
 * (ported from FatFS function: f_puts())                                            */
/*-----------------------------------------------------------------------*/

int McuLFS_puts (
  const char* str, /* Pointer to the string to be output */
  lfs_file_t* fp       /* Pointer to the file object */
)
{
  putbuff pb;
  uint32_t nw;

  pb.fp = fp;       /* Initialize output buffer */
  pb.nchr = pb.idx = 0;

  while (*str) {     /* Put the string */
    putc_bfd(&pb, *str++);
  }
  nw = lfs_file_write(&McuLFS_lfs,pb.fp, pb.buf, (uint32_t)pb.idx);

  if (   pb.idx >= 0    /* Flush buffered characters to the file */
    && nw>=0
    && (uint32_t)pb.idx == nw)
    {
        return pb.nchr;
    }
  return -1;
}

uint8_t McuLFS_Format() {
	int res;
	if (McuLFS_isMounted) {
		printf("File system is mounted, unmount it first.\r\n");
		return ERR_FAILED;
	}
	res = lfs_format(&McuLFS_lfs, &McuLFS_cfg);
	if (res == LFS_ERR_OK) {
		printf("Formatting ...Done.\r\n");
		return ERR_OK;
	}
	else{
		printf("Formatting ...FAILED!\r\n");
		return ERR_FAILED;

	}
}

uint8_t McuLFS_Mount(){
	int res;
	if (McuLFS_isMounted) {
		printf("File system is already mounted.\r\n");
		return ERR_FAILED;
	}
	res = lfs_mount(&McuLFS_lfs, &McuLFS_cfg);
	if (res == LFS_ERR_OK) {
		printf("Mounting ...Done.\r\n");
		McuLFS_isMounted = TRUE;
		return ERR_OK;
	}
	else {
		printf(" FAILED! Did you format the device already?\r\n");
		return ERR_FAILED;
	}
}

uint8_t McuLFS_Unmount() {
	int res;

	if (!McuLFS_isMounted) {
		printf("File system is already unmounted.\r\n");
		return ERR_FAILED;
	}
	res = lfs_unmount(&McuLFS_lfs);
	if (res == LFS_ERR_OK) {
		printf("Unmounting ....done.\r\n");
		McuLFS_isMounted = FALSE;
		return ERR_OK;
	}
	else {
		printf("Unmounting ...FAILED!\r\n");
		return ERR_FAILED;
	}
}

uint8_t McuLFS_Dir(const char *path) {
  int res;
  lfs_dir_t dir;
  struct lfs_info info;

  if (!McuLFS_isMounted) {
	  printf("File system is not mounted, mount it first.\r\n");
	  return ERR_FAILED;
  }
  if (path == NULL) {
    path = "/"; /* default path */
  }
  res = lfs_dir_open(&McuLFS_lfs, &dir, path);
  if (res != LFS_ERR_OK) {
	  printf("FAILED lfs_dir_open()!\r\n");
	  return ERR_FAILED;
  }
  for(;;) {
	  res = lfs_dir_read(&McuLFS_lfs, &dir, &info);
	  if (res < 0) {
		  printf("FAILED lfs_dir_read()!\r\n");
		  return ERR_FAILED;
	  }
	  if (res == 0) { /* no more files */
		  break;
	  }

	  switch (info.type) {
	  case LFS_TYPE_REG:
		  printf("reg ");
		  break;
	  case LFS_TYPE_DIR:
		  printf("dir ");
		  break;
	  default:
		  printf("?   ");
		  break;
	  }
	  static const char *prefixes[] = { "", "K", "M", "G" }; /* prefixes for kilo, mega, and giga */
	  unsigned char buf[12];

	  for (int i = sizeof(prefixes) / sizeof(prefixes[0]) - 1; i >= 0; i--) {
		  if (info.size >= (1UL << (10 * i)) - 1) {
			  uint32_t size = info.size >> (10 * i);
			  int digits = 4 - (i != 0);

			  // Convert size to a formatted string
			  snprintf((char*)buf, sizeof(buf), "%*u%c ", digits, size, prefixes[i][0]);
			  printf("%s", buf);
			  break;
		  }
	    }
	  printf("%s\r\n", info.name);
  }
  res = lfs_dir_close(&McuLFS_lfs, &dir);
  if (res != LFS_ERR_OK) {
	  printf("FAILED lfs_dir_close()!\r\n");
	  return ERR_FAILED;
  }
  return ERR_OK;
}

/*
 * Prints a list of Files and Directories of a given path
 * If path == NULL, the Files and Direcotries of the root-directory are printed
 * The First two characters of every line determin if its a File (F:) or a Directory (D:)
 */
uint8_t McuLFS_FileList(const char *path) {
  int res;
  lfs_dir_t dir;
  struct lfs_info info;
  if (!McuLFS_isMounted) {
	  printf("File system is not mounted, mount it first.\r\n");
	  return ERR_FAILED;
  }
  if (path == NULL) {
    path = "/"; /* default path */
  }
  res = lfs_dir_open(&McuLFS_lfs, &dir, path);
  if (res != LFS_ERR_OK) {
	  printf("FAILED lfs_dir_open()!\r\n");
	  return ERR_FAILED;
  }
  for(;;) {
	  res = lfs_dir_read(&McuLFS_lfs, &dir, &info);
	  if (res < 0) {
		  printf("FAILED lfs_dir_read()!\r\n");
		  return ERR_FAILED;
	  }
	  if (res == 0) { /* no more files */
		  break;
	  }
	  if (!(strcmp(info.name, ".") == 0 || strcmp(info.name, "..") == 0)) {
		  switch (info.type) {
		  case LFS_TYPE_REG:
			  printf("F:");
	          break;
		  case LFS_TYPE_DIR:
			  printf("D:");
	          break;
		  default:
	          printf("?:");
	          break;
	      }
		  printf("%s\r\n", info.name);
	  }
  }/* for */
  res = lfs_dir_close(&McuLFS_lfs, &dir);
  if (res != LFS_ERR_OK) {
    printf("FAILED lfs_dir_close()!\r\n");
    return ERR_FAILED;
  }
  return ERR_OK;
}

uint8_t McuLFS_CopyFile(const char *srcPath, const char *dstPath) {

	lfs_file_t fsrc, fdst;
	int result, nofBytesRead;
	uint8_t buffer[32]; /* copy buffer */
	uint8_t res = ERR_OK;
	if (!McuLFS_isMounted) {
		printf("File system is not mounted, mount it first.\r\n");
		return ERR_FAILED;
	}
	/* open source file */
	result = lfs_file_open(&McuLFS_lfs, &fsrc, srcPath, LFS_O_RDONLY);
	if (result < 0) {
		printf("*** Failed opening source file!\r\n");
		return ERR_FAILED;
	}
	/* create destination file */
	result = lfs_file_open(&McuLFS_lfs, &fdst, dstPath, LFS_O_WRONLY | LFS_O_CREAT);
	if (result < 0) {
		(void) lfs_file_close(&McuLFS_lfs, &fsrc);
		printf(" *** Failed opening destination file!\r\n");
		return ERR_FAILED;
	}
	/* now copy source to destination */
	for (;;) {
		nofBytesRead = lfs_file_read(&McuLFS_lfs, &fsrc, buffer, sizeof(buffer));
		if (nofBytesRead < 0) {
			printf("*** Failed reading source file!\r\n");
			res = ERR_FAILED;
			break;
		}
		if (nofBytesRead == 0) { /* end of file */
			break;
		}
		result = lfs_file_write(&McuLFS_lfs, &fdst, buffer, nofBytesRead);
		if (result < 0) {
			printf("*** Failed writing destination file!\r\n");
			res = ERR_FAILED;
			break;
		}
	}/* for */
	/* close all files */
	result = lfs_file_close(&McuLFS_lfs, &fsrc);
	if (result < 0) {
		printf("*** Failed closing source file!\r\n");
		res = ERR_FAILED;
	}
	result = lfs_file_close(&McuLFS_lfs, &fdst);
	if (result < 0) {
		printf("*** Failed closing destination file!\r\n");
		res = ERR_FAILED;
	}
	return res;
}

uint8_t McuLFS_MoveFile(const char *srcPath, const char *dstPath) {

	if (!McuLFS_isMounted) {
		printf("File system is not mounted, mount it first.\r\n");
		return ERR_FAILED;
	}
	if (lfs_rename(&McuLFS_lfs, srcPath, dstPath) < 0) {
		printf("ERROR: failed renaming file or directory.\r\n");
		return ERR_FAILED;
	}
	return ERR_OK;
}

/*
 * Used to read out data from Files for SDEP communication
 */
uint8_t McuLFS_ReadFile(lfs_file_t* file, bool readFromBeginning, size_t nofBytes) {
	static int32_t filePos;
	size_t fileSize;
	uint8_t buf[1024];

	if( nofBytes > 1024) {
		nofBytes = 1024;
	}
	if(readFromBeginning) {
		lfs_file_rewind(&McuLFS_lfs,file);
	    filePos = 0;
	} else {
		lfs_file_seek(&McuLFS_lfs,file, filePos,LFS_SEEK_SET);
	}
	fileSize = lfs_file_size(&McuLFS_lfs, file);
	filePos = lfs_file_tell(&McuLFS_lfs, file);
	fileSize = fileSize - filePos;

	if (fileSize < 0) {
	    return ERR_FAILED;
	}

	if(fileSize > nofBytes)  {
		if (lfs_file_read(&McuLFS_lfs, file, buf, nofBytes) < 0) {
			return ERR_FAILED;
	    }
		printf("%u, %zu\n",buf, nofBytes);
	    filePos = filePos + nofBytes;
	    return ERR_OK;

	} else {
	    if (lfs_file_read(&McuLFS_lfs, file, buf, fileSize) < 0) {
	    	return ERR_FAILED;
	    }
	    printf("%u, %zu\n",buf, fileSize);
	    filePos = filePos + fileSize;
	    return ERR_PARAM_SIZE; //EOF
	  }
}

uint8_t McuLFS_openFile(lfs_file_t* file, uint8_t* filename) {

	if (lfs_file_open(&McuLFS_lfs, file, (const char*)filename, LFS_O_RDWR | LFS_O_CREAT| LFS_O_APPEND) < 0)
	{
		return ERR_FAILED;
	}
	return ERR_OK;
}

uint8_t McuLFS_closeFile(lfs_file_t* file) {

	if(lfs_file_close(&McuLFS_lfs, file) == 0) {
		return ERR_OK;
	} else {
		return ERR_FAILED;
	}
}

uint8_t McuLFS_writeLine(lfs_file_t* file, uint8_t* line) {

	uint8_t lineBuf[200];
	strcpy((char*)lineBuf, (char*)line);
	strcat((char*)lineBuf, "\r\n");

	if (lfs_file_write(&McuLFS_lfs, file, lineBuf, strlen((char*)lineBuf)) < 0) {
		lfs_file_close(&McuLFS_lfs, file);
		return ERR_FAILED;
	}
	return ERR_OK;
}

uint8_t McuLFS_readLine(lfs_file_t* file, uint8_t* lineBuf, size_t bufSize, uint8_t* nofReadChars) {

	lineBuf[0] = '\0';
	uint8_t ch;
	*nofReadChars = 0;

	while (lfs_file_read(&McuLFS_lfs, file, &ch, 1) != 0 && ch != '\n') {
		(*nofReadChars)++;
		if (*nofReadChars < bufSize - 1) {
			lineBuf[*nofReadChars - 1] = ch;
		}
	}
	lineBuf[*nofReadChars] = '\0';

	return ERR_OK;
}

/* Function for the Shell PrintHex command */
static uint8_t readFromFile(void *hndl, uint32_t addr, uint8_t *buf, size_t bufSize) {

	lfs_file_t *fp;

	fp = (lfs_file_t*)hndl;
	if (lfs_file_read(&McuLFS_lfs, fp, buf, bufSize) < 0) {
		return ERR_FAILED;
	}
	return ERR_OK;
}

uint8_t McuLFS_RemoveFile(const char *filePath) {

	int result;

	if (!McuLFS_isMounted) {
		printf("ERROR: File system is not mounted.\r\n");
		return ERR_FAILED;
	}

	result = lfs_remove(&McuLFS_lfs, filePath);
	if (result < 0) {
    	printf("ERROR: Failed removing file.\r\n");
    	return ERR_FAILED;
    }

    return ERR_OK;
}

lfs_t* McuLFS_GetFileSystem(void) {
	return &McuLFS_lfs;
}


