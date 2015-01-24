#ifndef _TYPES_H_
#define _TYPES_H_

typedef signed char           int8_t;
typedef short                int16_t;
typedef int                  int32_t;
typedef long long            int64_t;
typedef unsigned char         uint8_t;
typedef unsigned short       uint16_t;
typedef unsigned int         uint32_t;
typedef unsigned long long   uint64_t;

typedef __SIZE_TYPE__         size_t;

#define CMDLINE	1024


typedef short				CHAR16;
typedef void				VOID;
typedef unsigned long		UINTN;
typedef uint8_t				UINT8;
typedef UINT8				CHAR8;
typedef int16_t				INT16;
typedef uint16_t			UINT16;
typedef int32_t				INTN;
typedef unsigned int		UINT32;
typedef int64_t				INT64;
typedef unsigned long long	UINT64;

typedef unsigned char		u8;
typedef unsigned short		u16;
typedef unsigned int		u32;
typedef unsigned long long	u64;
typedef int bool;
typedef unsigned int    boolean_t;

#define false 0
#define true  1
#define NULL  0
//
typedef unsigned long RGBA; 

typedef struct {
	u32 width; // everything else filled by BootVgaInitializationKernel() on return
	u32 height;
	u32 xmargin;
} CURRENT_VIDEO_MODE_DETAILS;

volatile CURRENT_VIDEO_MODE_DETAILS vmode;


volatile uint32_t VIDEO_CURSOR_POSY;
volatile uint32_t VIDEO_CURSOR_POSX;
volatile uint32_t VIDEO_ATTR;



#endif
