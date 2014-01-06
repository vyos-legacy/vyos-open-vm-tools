/*********************************************************
 * Copyright (C) 2006 VMware, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation version 2.1 and no later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the Lesser GNU General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA.
 *
 *********************************************************/

/*
 * All the legalese that we display in About boxes and similar places.
 */


#ifndef VM_LEGAL_H
#define VM_LEGAL_H

/*
 * COMPANY_NAME comes from vm_version.h
 */
#include "vm_version.h"

#ifndef WSTR
#define WSTR_(x) L ## x
#define WSTR(x) WSTR_(x)
#endif


#define COPYRIGHT_YEARS    "1998-2013"
#define COPYRIGHT_STRING   "Copyright (C) " COPYRIGHT_YEARS " " COMPANY_NAME
#define RIGHT_RESERVED     "All rights reserved."

/*
 * Use UTF8_COPYRIGHT_STRING_BASE when the COMPANY_NAME must be separated out
 * to create a hyperlink.
 */
#define UTF8_COPYRIGHT_STRING_BASE   "Copyright \302\251 " COPYRIGHT_YEARS
#define UTF8_COPYRIGHT_STRING   UTF8_COPYRIGHT_STRING_BASE " " COMPANY_NAME

/*
 * A UTF-16 version of the copyright string.  wchar_t is an
 * implementation-defined type, but we can expect it to be UTF-16 on
 * Windows. (Only Windows cares about UTF-16 anyway.)
 */
#ifdef _WIN32
#define UTF16_COPYRIGHT_STRING L"Copyright \x00A9 " WSTR(COPYRIGHT_YEARS) L" " WSTR(COMPANY_NAME)
#endif


/*
 * Use PATENTS_STRING for showing the patents string in plaintext form.
 * PATENTS_FMT_STRING can be used with PATENTS_URL for creating hyperlinks.
 */
#define PATENTS_STRING_BASE "This product is protected by U.S. and international copyright and\nintellectual property laws. VMware products are covered by one or\nmore patents listed at "
#define PATENTS_STRING PATENTS_STRING_BASE "<" PATENTS_URL ">."
#define PATENTS_FMT_STRING PATENTS_STRING_BASE "%s."
#define PATENTS_URL "http://www.vmware.com/go/patents"

#define TRADEMARK_STRING "VMware is a registered trademark or trademark of VMware, Inc. in the\nUnited States and/or other jurisdictions."
#define GENERIC_TRADEMARK_STRING "All other marks and names mentioned herein may be trademarks of their\nrespective companies."

#endif /* VM_LEGAL_H */
