/* **********************************************************
 * Copyright 2007 VMware, Inc.  All rights reserved. 
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

/*
 * This header file is given out as part of the open source
 * tools. Things in this file are public, but they may not have 
 * been tested or documented, and that may change in future releases.
 * The public Vix API is defined in vix.h
 *
 * These definitions are used by the implementation of the vix
 * client, the VMX process, and the tols.
 *
 */

#ifndef _VIXOpenSource_h_
#define _VIXOpenSource_h_

#ifdef __cplusplus
extern "C"{
#endif 

/*
 * VIX_HIDE_BORA_DEPENDENCIES:
 *
 * This flag allows some mini-applications to use parts of Vix that will
 * someday make it into the public API, without including all of the include
 * directories in bora. Specifically, some VMware apps will want to use a
 * separate, more public, build tree, but still make use API that have not been
 * publicly released yet.
 */

/*
 * VIX_HIDE_FROM_JAVA
 *
 * Setting this flag minizes the functions exposed here.  The java
 * binding is generated by processing this header, resulting in a
 * large number of currently unused functions.  To keep the number
 * of functions that need to be exposed via vixWrapper, we hide all
 * but the needed functions to java.
 */

/*
 * If we're hiding functions from java, also hide the bora dependencies.
 */
#ifdef VIX_HIDE_FROM_JAVA
#define VIX_HIDE_BORA_DEPENDENCIES
#endif

#include "vm_basic_types.h"

#ifndef VIX_HIDE_BORA_DEPENDENCIES
#include "cryptoError.h"
#endif // VIX_HIDE_BORA_DEPENDENCIES

#include "vix.h"    // Vix Public API



/*
 *-----------------------------------------------------------------------------
 *
 * Vix Errors --
 *
 *-----------------------------------------------------------------------------
 */


#ifndef VIX_HIDE_BORA_DEPENDENCIES
VixError Vix_TranslateSystemError(int systemError);
VixError Vix_TranslateCryptoError(CryptoError cryptoError);
#endif // VIX_HIDE_BORA_DEPENDENCIES



/*
 *-----------------------------------------------------------------------------
 *
 * VIX Handles --
 *
 * These are common functions that apply to handles of several types. 
 *-----------------------------------------------------------------------------
 */

/*
 * VIX Property ID's
 *
 * These are used in the tools, but they are not (yet) part of the public
 * API. They may be promoted, but for now they are not tested or documented.
 */

enum {
   VIX_PROPERTY_VM_GUEST_TEMP_DIR_PROPERTY            = 203,

   /* VMX properties. */
   VIX_PROPERTY_VMX_VERSION                           = 4400,
   VIX_PROPERTY_VMX_PRODUCT_NAME                      = 4401,
   VIX_PROPERTY_VMX_VIX_FEATURES                      = 4402,

   /* GuestOS and Tools properties. */
   VIX_PROPERTY_GUEST_TOOLS_VERSION                   = 4500,
   VIX_PROPERTY_GUEST_TOOLS_API_OPTIONS               = 4501,
   VIX_PROPERTY_GUEST_OS_FAMILY                       = 4502,
   VIX_PROPERTY_GUEST_OS_VERSION                      = 4503,
   VIX_PROPERTY_GUEST_OS_PACKAGE_LIST                 = 4504,
   VIX_PROPERTY_GUEST_NAME                            = 4505,
   VIX_PROPERTY_GUEST_POWER_OFF_SCRIPT                = 4506,
   VIX_PROPERTY_GUEST_POWER_ON_SCRIPT                 = 4507,
   VIX_PROPERTY_GUEST_RESUME_SCRIPT                   = 4508,
   VIX_PROPERTY_GUEST_SUSPEND_SCRIPT                  = 4509,
   VIX_PROPERTY_GUEST_TOOLS_PRODUCT_NAM               = 4511,
   VIX_PROPERTY_FOREIGN_VM_TOOLS_VERSION              = 4512,
   VIX_PROPERTY_VM_DHCP_ENABLED                       = 4513,
   VIX_PROPERTY_VM_IP_ADDRESS                         = 4514,
   VIX_PROPERTY_VM_SUBNET_MASK                        = 4515,
   VIX_PROPERTY_VM_DEFAULT_GATEWAY                    = 4516,
   VIX_PROPERTY_VM_DNS_SERVER_DHCP_ENABLED            = 4517,
   VIX_PROPERTY_VM_DNS_SERVER                         = 4518,
};



/*
 *-----------------------------------------------------------------------------
 *
 * PropertyList --
 *
 *-----------------------------------------------------------------------------
 */

/* 
 * VIX Property Type
 */

enum {
   //VIX_PROPERTYTYPE_ANY             = 0,
   //VIX_PROPERTYTYPE_INTEGER         = 1,
   //VIX_PROPERTYTYPE_STRING          = 2,
   //VIX_PROPERTYTYPE_BOOL            = 3,
   //VIX_PROPERTYTYPE_HANDLE          = 4,
   //VIX_PROPERTYTYPE_INT64           = 5,
   //VIX_PROPERTYTYPE_BLOB            = 6,
   VIX_PROPERTYTYPE_POINTER           = 7
};


#ifndef VIX_HIDE_FROM_JAVA
/*
 * This is a single name/value pair.
 */
typedef struct VixPropertyValue
{
   int                        propertyID;
   VixPropertyType            type;

   union {
      Bool                    boolValue;
      char                    *strValue;
      int                     intValue;
      int64                   int64Value;
      VixHandle               handleValue;
      struct {
         unsigned char        *blobContents;
         int                  blobSize;
      } blobValue;
      void                    *ptrValue;
   } value;

   Bool                       isDirty;
   struct VixPropertyValue    *next;
} VixPropertyValue;


/*
 * This is the entire list.
 */
typedef struct VixPropertyListImpl
{
   VixPropertyValue    *properties;
} VixPropertyListImpl;


void VixPropertyList_Initialize(VixPropertyListImpl *propList);

void VixPropertyList_RemoveAllWithoutHandles(VixPropertyListImpl *propList);

VixError VixPropertyList_Serialize(VixPropertyListImpl *propListImpl,
                                   Bool dirtyOnly,
                                   size_t *resultSize,
                                   char **resultBuffer);

VixError VixPropertyList_Deserialize(VixPropertyListImpl *propListImpl,
                                     const char *buffer,
                                     size_t bufferSize);
 
VixError VixPropertyList_GetString(struct VixPropertyListImpl *propList,
                                   int propertyID,
                                   int index,
                                   char **resultValue);
                                                 
VixError VixPropertyList_SetString(struct VixPropertyListImpl *propList,
                                   int propertyID,
                                   const char *value);

VixError VixPropertyList_GetInteger(struct VixPropertyListImpl *propList,
                                    int propertyID,
                                    int index,
                                    int *resultValue);

VixError VixPropertyList_SetInteger(struct VixPropertyListImpl *propList,
                                    int propertyID,
                                    int value);

VixError VixPropertyList_GetBool(struct VixPropertyListImpl *propList,
                                 int propertyID,
                                 int index,
                                 Bool *resultValue);

VixError VixPropertyList_SetBool(struct VixPropertyListImpl *propList,
                                 int propertyID,
                                 Bool value);

VixError VixPropertyList_GetHandle(struct VixPropertyListImpl *propList,
                                   int propertyID,
                                   int index,
                                   VixHandle *resultValue);

VixError VixPropertyList_SetHandle(struct VixPropertyListImpl *propList,
                                   int propertyID,
                                   VixHandle value);

VixError VixPropertyList_GetInt64(struct VixPropertyListImpl *propList,
                                  int propertyID,
                                  int index,
                                  int64 *resultValue);

VixError VixPropertyList_SetInt64(struct VixPropertyListImpl *propList,
                                  int propertyID,
                                  int64 value);

VixError VixPropertyList_GetBlob(struct VixPropertyListImpl *propList,
                                 int propertyID,
                                 int index,
                                 int *resultSize,
                                 unsigned char **resultValue);

VixError VixPropertyList_SetBlob(struct VixPropertyListImpl *propList,
                                 int propertyID,
                                 int blobSize,
                                 const unsigned char *value);

VixError VixPropertyList_RemoveAll(VixHandle propertyListHandle);

VixError VixPropertyList_Remove(VixHandle propertyListHandle,
                                int propertyID);

VixError VixPropertyList_AppendProperties(VixHandle handle, 
                                          int firstPropertyID,
                                          ...);

VixError VixPropertyList_AllocPropertyList(VixHandle *resultHandle, 
                                           int firstPropertyID,
                                           ...);

VixError VixPropertyList_FindProperty(VixPropertyListImpl *propList,
                                      int propertyID,
                                      VixPropertyType type,
                                      int index,
                                      Bool createIfMissing,
                                      VixPropertyValue **resultEntry);

Bool VixPropertyList_PropertyExists(VixPropertyListImpl *propList,
                                    int propertyID,
                                    VixPropertyType type);

VixError VixPropertyListAppendProperty(VixPropertyListImpl *propList,
                                       int propertyID,
                                       VixPropertyType type,
                                       VixPropertyValue **resultEntry);

int VixPropertyList_GetNumProperties(VixHandle propertyListHandle,
                                     int propertyID);

VixError VixPropertyList_GetOptionalProperties(VixHandle propertyListHandle, 
                                               int firstPropertyID,
                                               ...);

VixError VixPropertyList_GetIndexedProperties(VixHandle propertyListHandle, 
                                              Bool ignoreMissingProperties,
                                              int firstPropertyID,
                                              int firstPropertyIndex,
                                              ...);

VixError VixPropertyList_GetPtr(VixPropertyListImpl *propList,
                                int propertyID,
                                int index,
                                void **resultValue);

VixError VixPropertyList_SetPtr(VixPropertyListImpl *propList,
                                int propertyID,
                                void *value);



#endif   // VIX_HIDE_FROM_JAVA


/*
 *-----------------------------------------------------------------------------
 *
 * VixVM --
 *
 * This describes the persistent configuration state of a single VM. The 
 * VM may or may not be running.
 *
 *-----------------------------------------------------------------------------
 */

#define VIX_FOREIGN_VM_TOOLS_VMX_VERSION_STRING    "Foreign VM Tools"

/*
 * These are the types of variable strings we can read in the VM.
 */
enum {
   //VIX_VM_GUEST_VARIABLE            = 1,
   //VIX_VM_CONFIG_RUNTIME_ONLY       = 2,
   //VIX_GUEST_ENVIRONMENT_VARIABLE   = 3,
   VIX_GUEST_CONFIG                 = 4,
   VIX_VMDB_VARIABLE                = 5,
};



#ifdef __cplusplus
} // extern "C" {
#endif 


#endif // _VIXOpenSource_h_



