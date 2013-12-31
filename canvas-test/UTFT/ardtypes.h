#ifndef __ARDTYPES_H__
#define __ARDTYPES_H__

#include<math.h>
#include<string.h>

typedef unsigned char      uint8_t;
typedef unsigned short int uint16_t;
typedef unsigned int       uint32_t;
typedef uint8_t            boolean;
typedef uint8_t            byte;
typedef uint16_t           word;

#define bitmapdatatype uint16_t *
#define regsize        uint32_t
#define regtype        volatile uint32_t

#define swap(t,a,b)                                 \
		{                                   \
			register t __swap__temp__c; \
			__swap__temp__c = b;        \
			b = a;                      \
			a = __swap__temp__c;        \
		}
#define cbi(x,y)
#define sbi(x,y)
#define pgm_read_byte(x)	(*((char *) (x)))
#define pgm_read_word(x)	(*((uint16_t *) (x)))

#define fontbyte(x) cfont.font[x]  

#endif
