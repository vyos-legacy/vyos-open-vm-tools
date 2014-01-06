/* ************************************************************************
 * Copyright 2007 VMware, Inc.  All rights reserved. 
 * ************************************************************************
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
 * embed_version.h --
 *
 * Embeds a version string in an ELF binary that is readable by modinfo.
 */

#ifndef _EMBED_VERSION_H_
#define _EMBED_VERSION_H_

/* 
 * Using section attributes, embed the specified version in the "modinfo"
 * section of the ELF binary. We don't do this on Windows, where the PE format
 * already has version information stuffed inside it.
 *
 * We can't declare vm_version as static, otherwise it may get optimized out.
 * I've seen this when building with gcc 4.1, but not with 3.3.
 */
#ifndef _WIN32
#define VM_EMBED_VERSION(ver)                                                 \
const char vm_version[]                                                       \
   __attribute__((section(".modinfo"), unused)) = "version=" ver
#else
#define VM_EMBED_VERSION(ver)
#endif

#endif /* _EMBED_VERSION_H_ */
