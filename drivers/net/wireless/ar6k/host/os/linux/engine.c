/*------------------------------------------------------------------------------ */
/* <copyright file="engine.c" company="Atheros"> */
/*    Copyright (c) 2004-2009 Atheros Corporation.  All rights reserved. */
/*  */
/* This program is free software; you can redistribute it and/or modify */
/* it under the terms of the GNU General Public License version 2 as */
/* published by the Free Software Foundation; */
/* */
/* Software distributed under the License is distributed on an "AS */
/* IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or */
/* implied. See the License for the specific language governing */
/* rights and limitations under the License. */
/* */
/* */
/*------------------------------------------------------------------------------ */
/*============================================================================== */
/* Author(s): ="Atheros" */
/*============================================================================== */

#include "engine.h"
/* ATHENV */
#ifdef FW_AUTOLOAD
/* ATHENV */

#define ACC regs[0]

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
static unsigned int   regs[16];
static int            offset;
static unsigned char *cp;
int ar6k_reg_preload( int reg, unsigned int value )
{
    if( (reg < 0) || (reg > 16) )
        return(-1);              /* Illegal register */

    regs[ reg ] = value;
    return(0);
}

static int getoffset( void )
{
    int i = 0;

    cp++;
    offset++;

    i |=  (*cp & 0xFF);       cp++; offset++;
    i |=  (*cp & 0xFF) <<  8; cp++; offset++;
    i |=  (*cp & 0xFF) << 16; cp++; offset++;
    i |=  (*cp & 0xFF) << 24; cp++; offset++;

    return(i);
}

static int getarg( void )
{
    int i;

    if( *cp & 0x0F )
    {
        i = regs[ *cp & 0xF ];
        cp++; offset++;
        return(i);
    }
    return( getoffset() );
}

static void dumpregs( void )
{
    int i;

    sysprint("Acc: 0x%X\n", ACC);

    for(i=1; i<16; i++)
        sysprint("r%x : 0x%X\n", i, regs[ i ]);

}

int fwengine(const unsigned char *_img, int size, void *ar)
{
    unsigned char *img = (unsigned char*)_img;
    unsigned int   crc;
    int            i;

    /* 14 is the minimal size of an empty image */
    if( ! (*img) || (*img != 0xFF) || (size <14) )
    {
        syserr("Wrong image or no image\n");
        return -1;
    }

    /* Check crc32 */
    cp = img + size - 4;

    *cp = ~(*cp) & 0xFF ; cp++;
    *cp = ~(*cp) & 0xFF ; cp++;
    *cp = ~(*cp) & 0xFF ; cp++;
    *cp = ~(*cp) & 0xFF ;

    crc = crc32( ~0, img, size );

    if( crc )
    {
        syserr("Image CRC error\n");
        return -1;
    }

    if( *(img+1) > VERSION )
    {
        syserr("Image version is newer than the driver\n");
        return -1;
    }

    /* Basic checks complete, we can print greeting and FW_ID here if desired */

    /* Get to the first opcode */

    cp     = img + 2;                            /* Skip magic and version */
    offset = 2;

    while( (offset < size) && (*cp) )
    {
        cp++;                                    /* Skip build host name */
        offset++;
    }

    cp     += 6;                                 /* Skip build date */
    offset += 6;

    while( (cp > img) && (cp < (img+size-4)) )   /* just an additional bounds check */
    {
        /* First, find out opcode class */
        switch( (*cp & 0xF0) )
        {
          case RLoad :
              ACC = getarg( );
              break;
          case Ror   :
              ACC |= getarg( );
              break;
          case Rand  :
              ACC &= getarg( );
              break;
          case Add   :
              ACC += getarg( );
              break;
          case Rstor :
              regs[ *cp & 0xF ] = ACC;
              cp++;
              offset++;
              break;
          case Shift :
              i = getarg( );
              if(i < 0)
                  ACC >>= -i;
              else
                  ACC <<= i;
              break;
          case Nneg  :
              if( *cp & 0xF )
                  regs[ *cp & 0xF ] = ~regs[ *cp & 0xF ];
              else
                  ACC = -ACC;
              cp++;
              offset++;
              break;
          case Trr   :
              ACC = get_target_reg( getarg(), ar );
              break;
          case Trw   :
              write_target_reg( getarg(), ACC , ar );
              break;
          case Trx   :
              ACC = execute_on_target( getarg(), ACC, ar );
              break;
          case Exit  :
              if( *cp & 0xF ) /* abort with code */
                  return( regs[ *cp & 0xF ] );
              else{            /* clean exit */
                  bmidone( ar );
                  return(0);
              }
              break;
          case Cmp   :
              ACC -= getarg();
              break;
          case Ldprn :
              if( ! (*cp & 0xF) ) /* register dump */
                  dumpregs();
              else
              {
                  int ret;

                  ret = load_binary(ACC, cp, ar);
                  if( ret < 0 )
                      return( -1 ); /* Error */
                  cp += ret;
                  offset += ret;
              }
              break;
          case Jump  :
              if( !(*cp & 0xF) ||
                  ((*cp == 0xE1) && (ACC)) ||
                  ((*cp == 0xE2) && !(ACC)) )

                  offset = getoffset();
                  cp     = img + offset;

                  break;
          default:
              syserr("Image format error\n");
              break;

        }
    }
    return(0);
}
#ifdef UNIT_TEST
#include <stdio.h>

unsigned char fwbuf[ 4 * 1024 * 1024 ];

void bmidone( void *ar )
{
    printf("BMI done.\n");
}
int load_binary( unsigned int addr, unsigned char *cp, void *ar )
{
    int size = 0;
    int adv  = 5;

    cp++;
    size |= ( *cp & 0xFF );       cp++;
    size |= ( *cp & 0xFF ) <<  8; cp++;
    size |= ( *cp & 0xFF ) << 16; cp++;
    size |= ( *cp & 0xFF ) << 24; cp++;

    printf("Loading binary to address 0x%X, size %d\n", addr, size);
    adv += size;
    return(adv);
}

int execute_on_target( unsigned address, unsigned arg, void *ar )
{
    printf("Start target execution at address 0x%X with argument 0x%X\n", address, arg);
    return(0);
}

int write_target_reg( unsigned address, unsigned value, void *ar )
{
    printf("Writing target register 0x%X with 0x%X\n", address, value);
    return(0);
}

unsigned int get_target_reg( unsigned address, void *ar )
{
    unsigned int ret = 1;

    printf("Reading target register 0x%X, returning 0x%X\n", address, ret );
    return (ret);
}
int main()
{
    int ch, ret;
    unsigned char *bufptr = fwbuf;

    init_crc32();

    printf("Reading firmware image\n");
    while( (ch = getchar()) != EOF)
        *bufptr++ = ch;

    printf("Calling firmware engine for image size: %d\n", bufptr - fwbuf + 1);
    ret = engine( fwbuf, bufptr - fwbuf, NULL );
    printf("Engine returned with code %d\n", ret);
}
#endif /* UNIT_TEST */
#endif /* KERNEL_VERSION 2.6 only */
/* ATHENV */
#endif /* FW_AUTOLOAD */
/* ATHENV */
