#ifndef _YOYON_TYPES_H_
#define _YOYON_TYPES_H_

#include <stddef.h>


#define     TRUE      1
#define     FALSE     0

//typedef unsigned int bool;


typedef unsigned char           T_U8;       /* unsigned 8 bit integer */
typedef unsigned short          T_U16;      /* unsigned 16 bit integer */
typedef unsigned int           T_U32;      /* unsigned 32 bit integer */
typedef signed char             T_S8;       /* signed 8 bit integer */
typedef signed short            T_S16;      /* signed 16 bit integer */
typedef signed int             T_S32;      /* signed 32 bit integer */
typedef void                    T_VOID;     /* void */
typedef float                   T_FLOAT;

typedef unsigned char           UINT8;       /* unsigned 8 bit integer */
typedef unsigned short          UINT16;      /* unsigned 16 bit integer */
typedef unsigned int            UINT32;      /* unsigned 32 bit integer */
typedef signed char             INT8;       /* signed 8 bit integer */
typedef signed short            INT16;      /* signed 16 bit integer */
typedef signed int              INT32;      /* signed 32 bit integer */

/* These types must be 8-bit integer */
typedef signed char     CHAR;
typedef unsigned char   UCHAR;
typedef unsigned char   BYTE;

/* These types must be 16-bit integer */
typedef short           SHORT;
typedef unsigned short  USHORT;
typedef unsigned short  WORD;
typedef unsigned short  WCHAR;



//#define _WIN64


//#ifdef _WIN64
//typedef unsigned long long          FULL_UINT;
//typedef signed long long            FULL_INT;
//#else
typedef unsigned int              FULL_UINT;
typedef signed int                FULL_INT;
//#endif


#endif  //_YOYON_TYPES_H_

