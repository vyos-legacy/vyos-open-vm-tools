/* **********************************************************
 * Copyright (C) 1998-2004 VMware, Inc. All Rights Reserved 
 * **********************************************************
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


#ifndef VM_VERSION_H
#define VM_VERSION_H

#define INCLUDE_ALLOW_USERLEVEL
#define INCLUDE_ALLOW_VMMEXT
#define INCLUDE_ALLOW_MODULE
#define INCLUDE_ALLOW_VMMON
#define INCLUDE_ALLOW_VMNIXMOD
#define INCLUDE_ALLOW_VMKERNEL
#define INCLUDE_ALLOW_VMKDRIVERS
#define INCLUDE_ALLOW_VMCORE
#include "includeCheck.h"
#include "vm_product.h"
#include "vm_tools_version.h"


/*
 * buildNumber.h is generated by make/mk/buildNumber.mk to match
 * the build number defined by the BUILD_NUMBER variable at the
 * beginning of every build.
 * --Jeremy Bar
 */
#include "buildNumber.h"

#ifdef VMX86_DEVEL
#   ifdef VMX86_DEBUG
#      define COMPILATION_OPTION "DEBUG"
#   else
#      define COMPILATION_OPTION "OPT"
#   endif
#else
#   ifdef VMX86_ALPHA
#      define COMPILATION_OPTION "ALPHA"
#   elif defined(VMX86_BETA)
#      ifdef VMX86_EXPERIMENTAL
#         define COMPILATION_OPTION "BETA-EXPERIMENTAL"
#      else
#         define COMPILATION_OPTION "BETA"
#      endif
#   elif defined(VMX86_RELEASE)
#      define COMPILATION_OPTION "Release"
#   elif defined(VMX86_OPT)
#      define COMPILATION_OPTION "OPT"
#   elif defined(VMX86_DEBUG)
#      define COMPILATION_OPTION "DEBUG"
#   elif defined(VMX86_STATS)
#      define COMPILATION_OPTION "STATS"
#   endif
#endif


/*
 * This is used so we can identify the build and release type
 * in any generated core files.
 */

#define BUILD_VERSION COMPILATION_OPTION BUILD_NUMBER


/* Hard-coded expiration date */
/* Please don't put 0 in the front if the month or date is single digital number,
 * otherwise ENCODE_DATE will treat it as an octal number.
 */
#define DATE_DAY_MAX 31
#define DATE_MONTH_MAX 12
#define ENCODE_DATE(year, month, day) ((year) * ((DATE_MONTH_MAX + 1) * (DATE_DAY_MAX + 1)) + (month) * (DATE_DAY_MAX + 1) + (day))
#if !defined(VMX86_DEVEL) && defined(BUILD_EXPIRE)
#   if defined(VMX86_SERVER)
#      define HARD_EXPIRE ENCODE_DATE(2008, 01, 31)
#   elif defined(VMX86_WGS)
#      define HARD_EXPIRE ENCODE_DATE(2008, 01, 31)
#   elif defined(VMX86_DESKTOP)
#      define HARD_EXPIRE ENCODE_DATE(2008, 01, 31)
#   elif defined(VMX86_P2V)
#      define HARD_EXPIRE ENCODE_DATE(2008, 01, 31)
#   elif defined(VMX86_V2V)
#      define HARD_EXPIRE ENCODE_DATE(2008, 01, 31)
#   elif defined(VMX86_SYSIMAGE)
#      define HARD_EXPIRE ENCODE_DATE(2008, 01, 31)
#   elif defined(VMX86_VCB)
#      define HARD_EXPIRE ENCODE_DATE(2008, 01, 31)
#   elif defined(VMX86_VPX)
#      define HARD_EXPIRE ENCODE_DATE(2008, 01, 31)
#   elif defined(VMX86_WBC)
#      define HARD_EXPIRE ENCODE_DATE(2008, 01, 31)
#   elif defined(VMX86_SDK)
#      define HARD_EXPIRE ENCODE_DATE(2008, 01, 31)
#   endif
#endif


/* 
 * Used in .rc files on the Win32 platform.
 * 
 * When building the Tools, we make an effort to follow the "internal" Tools 
 * version. Otherwise we use a hard-coded value for Workstation and a different
 * hard-coded value for every other product.
 */
#if defined(VMX86_DESKTOP)
   #define PRODUCT_VERSION    6,0,0,BUILD_NUMBER_NUMERIC
#elif defined(VMX86_TOOLS)
   #define PRODUCT_VERSION    TOOLS_VERSION_CURRENT_CSV,BUILD_NUMBER_NUMERIC
#elif defined(VMX86_VCB)
   #define PRODUCT_VERSION    1,0,0,BUILD_NUMBER_NUMERIC
#else
   #define PRODUCT_VERSION    3,1,0,BUILD_NUMBER_NUMERIC
#endif

/*
 * The VIE components are shared by different products and may be updated by newer
 * installers. Since the installer replaces the component files based on the version
 * resource, it's important that the file version be monotonically increasing. As
 * a result, these components need their own file version number that is
 * independent of the VMX86_XXX macros. This goes into the FILEVERSION property of
 * the version resources. The first release of this stuff was with VPX which had a
 * FILEVERSION of 1,0,0,BUILD_NUMBER_NUMERIC
 *
 * P2VA 2.0     : 2,1,2
 * VPX 1.2      : 2,1,3
 * V2V 1.0      : 2.2.0
 * SYSIMAGE 1.0 : 2.2.1 or later (TBD)
 * Symantec     : 2.2.2 7/2005
 * VC 2.0       : 2.2.3
 * P2V 2.1      : 2.2.4 (also used for WS55 betas and RC)
 * V2V 1.5      : 2.2.5 V2V 1.5 released with WS55
 * WS 5.1       : 2.2.5 to be set when WS55 branches
 * VCB 1.0      : e.x.p esx-dali: first release with vmacore + vstor3Bus
 * VCB 1.0.1    : 3.0.1 includes security fix for registry alteration vulnerability
 * VCB 1.1      : 3.1
 * VMI 2.0      : 3.1.0
 * P2VA 3.0     : 3.?.?
 */
#define VIE_FILEVERSION 3,1,0,BUILD_NUMBER_NUMERIC

/*
 * This string can be a little more "free form".  The license
 * manager doesn't depend on it.  This is the version that will
 * be used by the build process, the UI, etc.  Things people see.
 *
 * If platforms are on different version numbers, manage it here.
 *
 * Manage version numbers for each product here.
 *
 *  NOTE:  BE AWARE that Makefiles and build scripts depend
 *         on these defines.
 *
 */

/*
 * This is the Scripting API (VmCOM, VmPerl, VmXXX) version number.
 * It is independent of the main VMX product version number.
 * The first released Scripting API from branch server02 has
 * version 2.0.0 to distinguish it from the legacy Perl API.
 *
 * Rules for updating the version:
 * - New features bump either major or minor version, depending on
 *   the magnitude of the change.
 * - A change that deprecates or obsoletes any existing interfaces
 *   requires a major version bump.
 */
#define API_SCRIPTING_VERSION "e.x.p"

#define API_VMDB_VERSION "e.x.p"
#define ESX_VERSION "e.x.p"
#define GSX_VERSION "e.x.p"
#define VMSERVER_VERSION "e.x.p"
#define WORKSTATION_VERSION "e.x.p"
#define WORKSTATION_ENTERPRISE_VERSION "e.x.p"
#define ACE_MANAGEMENT_SERVER_VERSION "e.x.p"
#define MUI_VERSION "e.x.p"
#define CONSOLE_VERSION "e.x.p"
#define P2V_VERSION "e.x.p"
#define P2V_FILE_VERSION 3,0,0,0
#define PLAYER_VERSION "e.x.p"
#define V2V_VERSION "e.x.p"
#define V2V_FILE_VERSION 1,0,0,0

// These must match VIE_FILEVERSION above
#define SYSIMAGE_VERSION "4.0.0"
#define SYSIMAGE_FILE_VERSION VIE_FILEVERSION

#define VCB_VERSION "e.x.p"
#define VCB_FILE_VERSION 1,0,0,0
#define VPX_VERSION "e.x.p"
#define WBC_VERSION "e.x.p"
#define SDK_VERSION "e.x.p"
#define FOUNDRY_VERSION "e.x.p"
#define VMLS_VERSION "e.x.p"
#define DTX_VERSION "e.x.p"
#define DDK_VERSION "e.x.p"
#define VIM_API_VERSION "2.0.0"
#define TOOLS_VERSION "2007.09.04"

#ifdef VMX86_VPX
#define VIM_API_TYPE "VirtualCenter"
#else
#define VIM_API_TYPE "HostAgent"
#endif

#define VIM_EESX_PRODUCT_LINE_ID "embeddedEsx"
#define VIM_ESX_PRODUCT_LINE_ID "esx"
#define VIM_GSX_PRODUCT_LINE_ID "gsx"

#define PRODUCT_API_SCRIPTING_VERSION API_SCRIPTING_VERSION " " BUILD_NUMBER

#if defined(VMX86_SERVER)
#  define PRODUCT_VERSION_NUMBER ESX_VERSION
#elif defined(VMX86_WGS_MIGRATION)
#  define PRODUCT_VERSION_NUMBER GSX_MIGRATION_VERSION
#elif defined(VMX86_WGS)
#  define PRODUCT_VERSION_NUMBER VMSERVER_VERSION
#elif defined(VMX86_MUI)
#  define PRODUCT_VERSION_NUMBER MUI_VERSION
#elif defined(VMX86_ENTERPRISE_DESKTOP)
#  define PRODUCT_VERSION_NUMBER WORKSTATION_ENTERPRISE_VERSION
#elif defined(VMX86_DESKTOP)
#  define PRODUCT_VERSION_NUMBER WORKSTATION_VERSION
#elif defined(VMX86_API)
#  define PRODUCT_VERSION_NUMBER API_SCRIPTING_VERSION
#elif defined(VMX86_VPX)
#  define PRODUCT_VERSION_NUMBER VPX_VERSION
#elif defined(VMX86_WBC)
#  define PRODUCT_VERSION_NUMBER WBC_VERSION
#elif defined(VMX86_SDK)
#  define PRODUCT_VERSION_NUMBER SDK_VERSION
#elif defined(VMX86_P2V)
#  define PRODUCT_VERSION_NUMBER P2V_VERSION
#elif defined(VMX86_V2V)
#  define PRODUCT_VERSION_NUMBER V2V_VERSION
#elif defined(VMX86_SYSIMAGE)
#  define PRODUCT_VERSION_NUMBER SYSIMAGE_VERSION
#elif defined(VMX86_VCB)
#  define PRODUCT_VERSION_NUMBER VCB_VERSION
#elif defined(VMX86_FOUNDRY)
#  define PRODUCT_VERSION_NUMBER FOUNDRY_VERSION
#elif defined(VMX86_VMLS)
#  define PRODUCT_VERSION_NUMBER VMLS_VERSION
#elif defined(VMX86_DTX)
#  define PRODUCT_VERSION_NUMBER DTX_VERSION
#elif defined(VMX86_DDK)
#  define PRODUCT_VERSION_NUMBER DDK_VERSION
#elif defined(VMX86_TOOLS)
#  define PRODUCT_VERSION_NUMBER TOOLS_VERSION
#endif

#define PRODUCT_VERSION_STRING PRODUCT_VERSION_NUMBER " " BUILD_NUMBER

/*
 * The license manager requires that PRODUCT_VERSION_STRING matches the
 * following pattern: <x>[.<y>][.<z>].
 *
 * If platforms are on different version numbers, manage it here.
 */

#if defined(VMX86_TOOLS)
/* This product doesn't use a license */
#  define PRODUCT_VERSION_STRING_FOR_LICENSE ""
#  define PRODUCT_LICENSE_VERSION "0.0"
#else
#  if defined(VMX86_SERVER)
#    define PRODUCT_LICENSE_VERSION "2.0"
#  elif defined(VMX86_WGS_MIGRATION)
#    define PRODUCT_LICENSE_VERSION "1.0"
#  elif defined(VMX86_WGS)
#    define PRODUCT_LICENSE_VERSION "3.0"
#  elif defined(VMX86_ENTERPRISE_DESKTOP)
#    define PRODUCT_LICENSE_VERSION "1.0"
#  elif defined(VMX86_DESKTOP)
#    define PRODUCT_LICENSE_VERSION "6.0"
#  elif defined(VMX86_VPX)
#    define PRODUCT_LICENSE_VERSION "1.0"
#  elif defined(VMX86_WBC)
#    define PRODUCT_LICENSE_VERSION "1.0"
#  elif defined(VMX86_SDK)
#    define PRODUCT_LICENSE_VERSION "1.0"
#  elif defined(VMX86_P2V)
#    define PRODUCT_LICENSE_VERSION "1.0"
#  elif defined(VMX86_V2V)
#    define PRODUCT_LICENSE_VERSION "1.0"
#  else
#    define PRODUCT_LICENSE_VERSION "0.0"
#  endif
#  define PRODUCT_VERSION_STRING_FOR_LICENSE PRODUCT_LICENSE_VERSION " " BUILD_NUMBER
#endif

/*
 * This is for ACE Management Server
 * Since there is no separate product defined for Ace Mgmt Server
 * (i.e. PRODUCT=xxx when running makefile), we can not used the
 * generic PRODUCT_LICENSE_VERSION and PRODUCT_VERSION_STRING_FOR_LICENSE
 * definition.
 * As a result, the specific ACE_MGMT_SERVER_VERSION_STRING_FOR_LICENSE 
 * is used instead.
 * A similar reason is used also for the PRODUCT_NAME_FOR_LICENSE definition
 * in the vm_product.h
 */

#define ACE_MGMT_SERVER_VERSION_STRING_FOR_LICENSE "2.0"


/*
 * The configuration file version string should be changed
 * whenever we make incompatible changes to the config file
 * format or to the meaning of settings.  When we do this,
 * we must also add code that detects the change and can
 * convert an old config file to a new one.
 */

#define CONFIG_VERSION_VARIABLE		"config.version"

/*
 * PREF_VERSION_VARIABLE somehow cannot be written through Dictionary_Write
 * (there is a bug after the first reload). So it's not used.
 */
/* #define PREF_VERSION_VARIABLE		"preferences.version"*/

#define CONFIG_VERSION_DEFAULT		"1"	/* if no version in file*/
#define CONFIG_VERSION                  "8"

#define CONFIG_VERSION_UNIFIEDSVGAME	"3"	/* Merged (S)VGA for WinME*/
#define CONFIG_VERSION_UNIFIEDSVGA	"4"	/* Merged (S)VGA enabled.  -J.*/
#define CONFIG_VERSION_440BX		"5"	/* 440bx becomes default */
#define CONFIG_VERSION_NEWMACSTYLE	"3"	/* ethernet?.oldMACStyle */
#define CONFIG_VERSION_WS2              "2"     /* config version of WS2.0.x */
#define CONFIG_VERSION_MIGRATION        "6"     /* migration work for WS3 */
#define CONFIG_VERSION_ESX2             "6"     /* config version of ESX 2.x */
#define CONFIG_VERSION_UNDOPOINT        "7"     /* Undopoint paradigm (WS40) */
#define CONFIG_VERSION_WS4              "7"     /* config version of WS4.0.x */
#define CONFIG_VERSION_MSNAP            "8"     /* Multiple Snapshots */

#define VMVISOR_VERSION "99.99.99"


/*
 * Product version strings allows UIs to refer to a single place for specific
 * versions of product names.  These do not include a "VMware" prefix.
 */

#define PRODUCT_VERSION_SCALABLE_SERVER_1 PRODUCT_SCALABLE_SERVER_BRIEF_NAME " 1.x"
#define PRODUCT_VERSION_SCALABLE_SERVER_2 PRODUCT_SCALABLE_SERVER_BRIEF_NAME " 2.x"
#define PRODUCT_VERSION_SCALABLE_SERVER_3 PRODUCT_SCALABLE_SERVER_BRIEF_NAME " 3.x"
#define PRODUCT_VERSION_SCALABLE_SERVER_30 PRODUCT_SCALABLE_SERVER_BRIEF_NAME " 3.0"
#define PRODUCT_VERSION_SCALABLE_SERVER_31 PRODUCT_SCALABLE_SERVER_BRIEF_NAME " 3.1"
#define PRODUCT_VERSION_WGS_1 PRODUCT_WGS_BRIEF_NAME " 1.x"
#define PRODUCT_VERSION_GSX_2 PRODUCT_GSX_BRIEF_NAME " 2.x"
#define PRODUCT_VERSION_GSX_3 PRODUCT_GSX_BRIEF_NAME " 3.x"
#define PRODUCT_VERSION_WORKSTATION_4 PRODUCT_WORKSTATION_BRIEF_NAME " 4.x"
#define PRODUCT_VERSION_WORKSTATION_5 PRODUCT_WORKSTATION_BRIEF_NAME " 5.x"
#define PRODUCT_VERSION_WORKSTATION_6 PRODUCT_WORKSTATION_BRIEF_NAME " 6.x"
#define PRODUCT_VERSION_WORKSTATION_ENTERPRISE_1 "ACE 1.x"
#define PRODUCT_VERSION_WORKSTATION_ENTERPRISE_2 "ACE 2.x"
#define PRODUCT_VERSION_PLAYER_1 PRODUCT_PLAYER_BRIEF_NAME " 1.x"
#define PRODUCT_VERSION_MAC_DESKTOP_1 PRODUCT_MAC_DESKTOP_BRIEF_NAME " 1.x"


/*
 * This allows UIs and guest binaries to know what kind of VMX they are dealing
 * with. Don't change those values (only add new ones if needed) because they
 * rely on them --hpreg
 */

typedef enum {
   VMX_TYPE_UNSET,
   VMX_TYPE_EXPRESS, /* This deprecated type was used for VMware Express */
   VMX_TYPE_SCALABLE_SERVER,
   VMX_TYPE_WGS,
   VMX_TYPE_WORKSTATION,
   VMX_TYPE_WORKSTATION_ENTERPRISE /* This deprecated type was used for ACE 1.x */
} VMX_Type;


/*
 * This allows UIs and guest binaries to know what platform the VMX is running.
 */

typedef enum {
   VMX_PLATFORM_UNSET,
   VMX_PLATFORM_LINUX,
   VMX_PLATFORM_WIN32,
   VMX_PLATFORM_MACOS,
} VMX_Platform;


/*
 * UI versions
 *
 * Note that these only make sense in the context of the server type
 */

#define  UI_VERSION_OLD            1  // pre-versioned UIs
#define  UI_VERSION_ESX15          2
#define  UI_VERSION_GSX20          2
#define  UI_VERSION_ESX20          3
#define  UI_VERSION_GSX25          3
// Skip one just in case we want to insert ESX21 in between here for neatness
#define  UI_VERSION_GSX30          5
#define  UI_VERSION_VMSERVER10     6

#define  UI_VERSION                6  // Current UI version

#endif /* VM_VERSION_H */
