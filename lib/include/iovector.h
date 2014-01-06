/* **********************************************************
 * Copyright 1998 VMware, Inc.  All rights reserved. 
 * *********************************************************
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation version 2.1 and no later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA.
 */

/*
 * iovector.h --
 *
 *      iov management code API.
 */

#ifndef _IOVECTOR_H_
#define _IOVECTOR_H_

#define INCLUDE_ALLOW_USERLEVEL
#define INCLUDE_ALLOW_VMCORE
#include "includeCheck.h"

/*
 * Ugly definition of struct iovec.
 */
#if __linux__ || sun || __APPLE__
#include <sys/uio.h>    // for struct iovec
#else // if __linux__ || sun || __APPLE__

#ifndef HAS_IOVEC
struct iovec {
   uint8 *iov_base; /* Starting address.  */
   size_t iov_len;  /* Length in bytes.  */
};
#endif   // HAS_IOVEC

#endif

/*
 * This type should be used for variables that contain sector
 * position/quantity.
 */
typedef uint64 SectorType;

/*
 * An I/O Vector.
 */
typedef struct VMIOVec {
   Bool read;                   /* is it a readv operation? else it's write */
   SectorType startSector;
   SectorType numSectors;
   uint64 numBytes;             /* Total bytes from all of the entries */
   uint32 numEntries;           /* Total number of entries */
   struct iovec *entries;       /* Array of entries (dynamically allocated) */
   struct iovec *allocEntries;  /* The original array that can be passed to free(). 
                                 * NULL if entries is on a stack. */
} VMIOVec;

#define LAZY_ALLOC_MAGIC      ((void*)0xF0F0)

EXTERN VMIOVec* IOV_Split(VMIOVec *regionV,
                          SectorType numSectors,
                          uint32 sectorSize);

EXTERN void IOV_Log(const VMIOVec *iov);
EXTERN void IOV_Zero(VMIOVec *iov);
EXTERN Bool IOV_IsZero(VMIOVec* iov);
EXTERN VMIOVec* IOV_Duplicate(VMIOVec* iovIn);
EXTERN VMIOVec* IOV_Allocate(int numEntries);
EXTERN void IOV_Free(VMIOVec* iov);
EXTERN void IOV_DuplicateStatic(VMIOVec *iovIn,
                                int numStaticEntries,
                                struct iovec *staticEntries,
                                VMIOVec *iovOut);

EXTERN INLINE void IOV_MakeSingleIOV(VMIOVec* v,
                                     struct iovec* iov,
                                     SectorType startSector,
                                     SectorType dataLen,
                                     uint32 sectorSize,
                                     uint8* buffer,
                                     Bool read);

EXTERN void IOV_WriteIovToBuf(struct iovec* entries,
                              int numEntries,
                              uint8* bufOut,
                              size_t bufSize);

EXTERN void IOV_WriteBufToIov(uint8* bufIn,
                              size_t bufSize,
                              struct iovec* entries,
                              int numEntries);

EXTERN size_t
IOV_WriteIovToBufPlus(struct iovec* entries,
                      int numEntries,
                      uint8* bufOut,
                      size_t bufSize,
                      size_t iovOffset);

EXTERN size_t
IOV_WriteBufToIovPlus(uint8* bufIn,
                      size_t bufSize,
                      struct iovec* entries,
                      int numEntries,
                      size_t iovOffset);

EXTERN size_t
IOV_WriteIovToIov(VMIOVec *srcIov,
                  VMIOVec *dstIov,
                  uint32 sectorSizeShift);

/*
 *-----------------------------------------------------------------------------
 *
 * IOV_ASSERT, IOV_Assert --
 *
 *      Checks that the 'numEntries' iovecs in 'iov' are non-null and have
 *      nonzero lengths.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *	Assert-fails if the iovec is invalid.
 *
 *-----------------------------------------------------------------------------
 */


#if VMX86_DEBUG
#define IOV_ASSERT(IOVEC, NUM_ENTRIES) IOV_Assert(IOVEC, NUM_ENTRIES)
EXTERN void IOV_Assert(struct iovec *iov,       // IN: iovector
                       uint32 numEntries);      // IN: # of entries in 'iov'
#else
#define IOV_ASSERT(IOVEC, NUM_ENTRIES) ((void) 0)
#endif

#endif /* #ifndef _IOVECTOR_H_ */
