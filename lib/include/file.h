/* **********************************************************
 * Copyright 1998 VMware, Inc.  All rights reserved. 
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
 * file.h --
 *
 *	Interface to host file system and related utility functions.
 */

#ifndef _FILE_H_
#define _FILE_H_

#define INCLUDE_ALLOW_USERLEVEL
#define INCLUDE_ALLOW_VMCORE
#include "includeCheck.h"

#include "fileIO.h"
#include <stdio.h>

#ifdef N_PLAT_NLM
#define FILE_MAXPATH	256
#elif defined(_WIN32)
#define FILE_MAXPATH	MAX_PATH
#else
# if defined(__FreeBSD__) && BSD_VERSION >= 53
#  include <syslimits.h>  // PATH_MAX
# else 
#  include <limits.h>  // PATH_MAX
# endif 
#define FILE_MAXPATH	PATH_MAX
#endif

#define FILE_SEARCHPATHTOKEN ";"

#if __APPLE__
EXTERN Bool File_UnmountDev(const char *devName, Bool isFullPath,
                            Bool wholeDev, Bool eject);
#endif

EXTERN Bool File_Exists(const char *name);
EXTERN int File_Unlink(const char *name);
EXTERN int File_UnlinkIfExists(const char *name);
EXTERN int File_UnlinkDelayed(const char *name);
EXTERN void File_SplitName(const char *path,
			   char **volume, 
                           char **dir, 
                           char **base);
EXTERN void File_GetPathName(const char *fullpath, 
                             char **pathname, 
                             char **base);
EXTERN Bool File_CreateDirectory(char const *pathName);
EXTERN Bool File_CreateDirectoryHierarchy(char const *pathName);
EXTERN Bool File_DeleteEmptyDirectory(char const *pathName);
EXTERN Bool File_DeleteDirectoryTree(char const *pathName);
EXTERN int File_ListDirectory(char const *pathName, char ***ids);


EXTERN Bool File_IsWritableDir(const char *dirName);
EXTERN Bool File_IsRemote(const char *fileName);
EXTERN Bool File_IsDirectory(const char *fileName);
EXTERN Bool File_IsEmptyDirectory(const char *fileName);
EXTERN Bool File_IsFile(const char *fileName);
EXTERN Bool File_IsSymLink(const char *fileName);
EXTERN char *File_FindLastSlash(const char *path);

EXTERN char *File_Cwd(const char *drive); // XXX belongs to `process' module
EXTERN char *File_FullPath(const char *fileName);
EXTERN Bool File_IsFullPath(const char *fileName);
EXTERN uint64 File_GetFreeSpace(const char *fileName);
EXTERN uint64 File_GetCapacity(const char *fileName);

/* Deprecated; use Util_GetSafeTmpDir if you can */
EXTERN char *File_GetTmpDir(Bool useConf);

/* Deprecated; use Util_MakeSafeTemp if you can */
EXTERN int File_MakeTemp(const char *tag, char **presult);
EXTERN int File_MakeTempEx(const char *dir,
                           const char *fileName,
                           char **presult);

EXTERN int64 File_GetModTime(const char *fileName);
EXTERN char *File_GetModTimeString(const char *fileName);
EXTERN char *File_GetUniqueFileSystemID(const char *fileName);
EXTERN Bool File_SetTimes(const char *fileName,
                          VmTimeType createTime,      // IN: Windows NT time format
                          VmTimeType accessTime,      // IN: Windows NT time format
                          VmTimeType writeTime,       // IN: Windows NT time format
                          VmTimeType attrChangeTime); // IN: ignored
EXTERN Bool File_SupportsFileSize(const char *pathname, uint64 fileSize);
EXTERN Bool File_SupportsLargeFiles(const char *pathname);


EXTERN Bool File_CopyFromFdToFd(FileIODescriptor src, 
                                FileIODescriptor dst);
EXTERN FileIOResult File_CreatePrompt(FileIODescriptor *file, 
                                      const char *name, 
                                      int access, 
                                      int prompt);
EXTERN Bool File_CopyFromFd(FileIODescriptor src, 
                            const char *dstName, 
                            Bool overwriteExisting);
EXTERN Bool File_Copy(const char *srcName, 
                      const char *dstName, 
                      Bool overwriteExisting);
EXTERN Bool File_CopyFromFdToName(FileIODescriptor src, 
                                  const char *dstName, 
                                  int dstDispose);
EXTERN Bool File_CopyFromNameToName(const char *srcName, 
                                    const char *dstName, 
                                    int dstDispose);
EXTERN Bool File_Replace(const char *oldFile, 
                         const char *newFile);

EXTERN Bool File_Rename(const char *oldFile, 
                        const char *newFile);
EXTERN int64 File_GetSize(const char *name);
EXTERN int64 File_GetSizeByPath(const char *name);
EXTERN int64 File_GetSizeAlternate(const char *fileName);

/* file change notification module */
typedef void (*CbFunction)(void *clientData);
typedef void (*NotifyCallback)(const char *filename, 
                               int err, 
                               void *data);
typedef void (*PollTimeout) (CbFunction f, void *clientData, int delay);
typedef void (*PollRemoveTimeout) (CbFunction f, void *clientData);

EXTERN void File_PollInit(PollTimeout pt, 
                          PollRemoveTimeout prt);
EXTERN void File_PollExit(void);
EXTERN void File_PollImpersonateOnCheck(Bool check);
EXTERN Bool File_PollAddFile(const char *filename, 
                             uint32 pollPeriod, 
                             NotifyCallback callback, 
                             void *data, 
                             Bool fPeriodic);
EXTERN Bool File_PollAddDirFile(const char *filename,
                                uint32 pollPeriod, 
                                NotifyCallback callback,
                                void *data, 
                                Bool fPeriodic);
EXTERN Bool File_PollRemoveFile(const char *filename, 
                                uint32 pollPeriod,
                                NotifyCallback callback);
EXTERN Bool File_IsSameFile(const char *path1, const char *path2);

EXTERN Bool File_FindFileInSearchPath(const char *file, const char *searchPath,
                                      const char *cwd, char **result);

EXTERN char *File_ReplaceExtension(const char *input, const char *newExtension,
                                   int numExtensions, ...);
EXTERN Bool File_OnVMFS(const char *fileName);

EXTERN Bool File_MakeCfgFileExecutable(const char *path);

EXTERN Bool File_IsCharDevice(const char *filename);
EXTERN char *File_ExpandAndCheckDir(const char *dirName);

#endif // ifndef _FILE_H_
