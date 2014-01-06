/* **********************************************************
 * Copyright 2006 VMware, Inc.  All rights reserved. 
 * **********************************************************
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation version 2 and no later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

/*
 * file.c --
 *
 * File operations for the filesystem portion of the vmhgfs driver.
 */

/* Must come before any kernel header file. */
#include "driver-config.h"

#include <linux/errno.h>
#include <linux/module.h>
#include <linux/signal.h>
#include "compat_fs.h"
#include "compat_kernel.h"
#include "compat_slab.h"

#include "cpName.h"
#include "hgfsProto.h"
#include "module.h"
#include "request.h"
#include "fsutil.h"
#include "vm_assert.h"
#include "vm_basic_types.h"

/* Private functions. */
static int HgfsPackOpenRequest(struct inode *inode,
                               struct file *file,
                               HgfsReq *req);
static int HgfsUnpackOpenReply(HgfsReq *req,
                               HgfsOp opUsed,
                               HgfsHandle *file,
                               HgfsServerLock *lock);
static int HgfsGetOpenFlags(uint32 flags);

/* HGFS file operations for files. */
static int HgfsOpen(struct inode *inode,
                    struct file *file);
#if defined(VMW_USE_AIO)
static ssize_t HgfsAioRead(struct kiocb *iocb,
                           const struct iovec *iov,
                           unsigned long numSegs,
                           loff_t offset);
static ssize_t HgfsAioWrite(struct kiocb *iocb,
                            const struct iovec *iov,
                            unsigned long numSegs,
                            loff_t offset);
#else
static ssize_t HgfsRead(struct file *file,
                        char __user *buf,
                        size_t count,
                        loff_t *offset);
static ssize_t HgfsWrite(struct file *file,
                         const char __user *buf,
                         size_t count,
                         loff_t *offset);
#endif
static int HgfsFsync(struct file *file,
                     struct dentry *dentry,
                     int datasync);
static int HgfsMmap(struct file *file,
                    struct vm_area_struct *vma);
static int HgfsRelease(struct inode *inode,
                       struct file *file);

#ifndef VMW_SENDFILE_NONE
#if defined(VMW_SENDFILE_OLD)
static ssize_t HgfsSendfile(struct file *file,
                            loff_t *offset,
                            size_t count,
                            read_actor_t actor,
                            void __user *target);
#else /* defined(VMW_SENDFILE_NEW) */
static ssize_t HgfsSendfile(struct file *file,
                            loff_t *offset,
                            size_t count,
                            read_actor_t actor,
                            void *target);
#endif
#endif
#ifdef VMW_SPLICE_READ
static ssize_t HgfsSpliceRead(struct file *file,
                              loff_t *offset,
                              struct pipe_inode_info *pipe,
                              size_t len,
                              unsigned int flags);
#endif

/* HGFS file operations structure for files. */
struct file_operations HgfsFileFileOperations = {
   .owner      = THIS_MODULE,
   .open       = HgfsOpen,
#if defined(VMW_USE_AIO)
   .aio_read   = HgfsAioRead,
   .aio_write  = HgfsAioWrite,
#else
   .read       = HgfsRead,
   .write      = HgfsWrite,
#endif
   .fsync      = HgfsFsync,
   .mmap       = HgfsMmap,
   .release    = HgfsRelease,
#ifndef VMW_SENDFILE_NONE
   .sendfile   = HgfsSendfile,
#endif
#ifdef VMW_SPLICE_READ
   .splice_read = HgfsSpliceRead,
#endif
};


/* 
 * Private functions. 
 */

/*
 *----------------------------------------------------------------------
 *
 * HgfsPackOpenRequest --
 *
 *    Setup the Open request, depending on the op version.
 *
 * Results:
 *    Returns zero on success, or negative error on failure. 
 *
 * Side effects:
 *    None
 *
 *----------------------------------------------------------------------
 */

static int 
HgfsPackOpenRequest(struct inode *inode, // IN: Inode of the file to open
                    struct file *file,   // IN: File pointer for this open
                    HgfsReq *req)        // IN/OUT: Packet to write into
{
   HgfsRequest *requestHeader;
   HgfsRequestOpenV2 *requestV2;
   HgfsRequestOpen *request;
   HgfsFileName *fileNameP;
   size_t requestSize;
   int result;
   
   ASSERT(inode);
   ASSERT(file);
   ASSERT(req);

   requestHeader = (HgfsRequest *)(HGFS_REQ_PAYLOAD(req));

   switch (requestHeader->op) {
   case HGFS_OP_OPEN_V2:
      requestV2 = (HgfsRequestOpenV2 *)(HGFS_REQ_PAYLOAD(req));
      
      /* We'll use these later. */
      fileNameP = &requestV2->fileName;
      requestSize = sizeof *requestV2;
      
      requestV2->mask = HGFS_OPEN_VALID_MODE | HGFS_OPEN_VALID_FLAGS |
         HGFS_OPEN_VALID_SPECIAL_PERMS |  HGFS_OPEN_VALID_OWNER_PERMS | 
         HGFS_OPEN_VALID_GROUP_PERMS | HGFS_OPEN_VALID_OTHER_PERMS | 
         HGFS_OPEN_VALID_FILE_NAME | HGFS_OPEN_VALID_SERVER_LOCK;

      /* Set mode. */
      result = HgfsGetOpenMode(file->f_flags);
      if (result < 0) {
         LOG(4, (KERN_DEBUG "VMware hgfs: HgfsPackOpenRequest: failed to get "
                 "open mode\n"));
         return -EINVAL;
      }
      requestV2->mode = result;

      /* Set flags. */
      result = HgfsGetOpenFlags(file->f_flags);
      if (result < 0) {
         LOG(4, (KERN_DEBUG "VMware hgfs: HgfsPackOpenRequest: failed to get "
                 "open flags\n"));
         return -EINVAL;
      }
      requestV2->flags = result;

      /* Set permissions. */
      requestV2->specialPerms = (inode->i_mode & (S_ISUID | S_ISGID | S_ISVTX))
                                >> 9;
      requestV2->ownerPerms = (inode->i_mode & S_IRWXU) >> 6;
      requestV2->groupPerms = (inode->i_mode & S_IRWXG) >> 3;
      requestV2->otherPerms = (inode->i_mode & S_IRWXO);

      /* XXX: Request no lock for now. */
      requestV2->desiredLock = HGFS_LOCK_NONE;
      break;
   case HGFS_OP_OPEN:
      request = (HgfsRequestOpen *)(HGFS_REQ_PAYLOAD(req));
     
      /* We'll use these later. */
      fileNameP = &request->fileName;
      requestSize = sizeof *request;

      /* Set mode. */
      result = HgfsGetOpenMode(file->f_flags);
      if (result < 0) {
         LOG(4, (KERN_DEBUG "VMware hgfs: HgfsPackOpenRequest: failed to get "
                 "open mode\n"));
         return -EINVAL;
      }
      request->mode = result;

      /* Set flags. */
      result = HgfsGetOpenFlags(file->f_flags);
      if (result < 0) {
         LOG(4, (KERN_DEBUG "VMware hgfs: HgfsPackOpenRequest: failed to get "
                 "open flags\n"));
         return -EINVAL;
      }
      request->flags = result;

      /* Set permissions. */
      request->permissions = (inode->i_mode & S_IRWXU) >> 6;
      break;
   default:
      LOG(4, (KERN_DEBUG "VMware hgfs: HgfsPackOpenRequest: unexpected "
              "OP type encountered\n"));
      return -EPROTO;
   }

   /* Build full name to send to server. */
   if (HgfsBuildPath(fileNameP->name,
                     HGFS_PACKET_MAX - (requestSize - 1),
                     file->f_dentry) < 0) {
      LOG(4, (KERN_DEBUG "VMware hgfs: HgfsPackOpenRequest: build path "
              "failed\n"));
      return -EINVAL;
   }
   LOG(6, (KERN_DEBUG "VMware hgfs: HgfsPackOpenRequest: opening \"%s\", "
           "flags %o, create perms %o\n", fileNameP->name,
           file->f_flags, file->f_mode));

   /* Convert to CP name. */
   result = CPName_ConvertTo(fileNameP->name, 
                             HGFS_PACKET_MAX - (requestSize - 1),
                             fileNameP->name);
   if (result < 0) {
      LOG(4, (KERN_DEBUG "VMware hgfs: HgfsPackOpenRequest: CP conversion "
              "failed\n"));
      return -EINVAL;
   }

   /* Unescape the CP name. */
   result = HgfsUnescapeBuffer(fileNameP->name, result);
   fileNameP->length = result;
   req->payloadSize = requestSize + result;

   return 0;
}


/*
 *----------------------------------------------------------------------
 *
 * HgfsUnpackOpenReply --
 *
 *    Get interesting fields out of the Open reply, depending on the op 
 *    version.
 *
 * Results:
 *    Returns zero on success, or negative error on failure.
 *
 * Side effects:
 *    None
 *
 *----------------------------------------------------------------------
 */

static int
HgfsUnpackOpenReply(HgfsReq *req,          // IN: Packet with reply inside
                    HgfsOp opUsed,         // IN: What request op did we send
                    HgfsHandle *file,      // OUT: Handle in reply packet
                    HgfsServerLock *lock)  // OUT: The server lock we got
{
   HgfsReplyOpenV2 *replyV2;
   HgfsReplyOpen *replyV1;
   size_t replySize;

   ASSERT(req);
   ASSERT(file);
   ASSERT(lock);

   switch (opUsed) {
   case HGFS_OP_OPEN_V2:
      replyV2 = (HgfsReplyOpenV2 *)(HGFS_REQ_PAYLOAD(req));            
      replySize = sizeof *replyV2;
      *file = replyV2->file;
      *lock = replyV2->acquiredLock;
      break;
   case HGFS_OP_OPEN:
      replyV1 = (HgfsReplyOpen *)(HGFS_REQ_PAYLOAD(req));
      replySize = sizeof *replyV1;
      *file = replyV1->file;
      *lock = HGFS_LOCK_NONE;
      break;
   default:
      
      /* This really shouldn't happen since we set opUsed ourselves. */
      LOG(4, (KERN_DEBUG "VMware hgfs: HgfsUnpackOpenReply: unexpected "
              "OP type encountered\n"));
      ASSERT(FALSE);
      return -EPROTO;
   }

   if (req->payloadSize != replySize) {
      /*
       * The reply to Open is a fixed size. So the size of the payload
       * really ought to match the expected size of an HgfsReplyOpen[V2].
       */
      LOG(4, (KERN_DEBUG "VMware hgfs: HgfsUnpackOpenReply: wrong packet "
              "size\n"));
      return -EPROTO;
   }
   return 0;
}


/*
 *----------------------------------------------------------------------
 *
 * HgfsGetOpenFlags --
 *
 *    Based on the flags requested by the process making the open()
 *    syscall, determine which flags to send to the server to open the
 *    file.
 *
 * Results:
 *    Returns the correct HgfsOpenFlags enumeration to send to the
 *    server, or -1 on failure.
 *
 * Side effects:
 *    None
 *
 *----------------------------------------------------------------------
 */

static int
HgfsGetOpenFlags(uint32 flags) // IN: Open flags
{
   uint32 mask = O_CREAT | O_TRUNC | O_EXCL;
   int result = -1;

   LOG(6, (KERN_DEBUG "VMware hgfs: HgfsGetOpenFlags: entered\n"));

   /*
    * Mask the flags to only look at O_CREAT, O_EXCL, and O_TRUNC.
    */

   flags &= mask;

   /* O_EXCL has no meaning if O_CREAT is not set. */
   if (!(flags & O_CREAT)) {
      flags &= ~O_EXCL;
   }

   /* Pick the right HgfsOpenFlags. */
   switch (flags) {

   case 0:
      /* Regular open; fails if file nonexistant. */
      result = HGFS_OPEN;
      break;

   case O_CREAT:
      /* Create file; if it exists already just open it. */
      result = HGFS_OPEN_CREATE;
      break;

   case O_TRUNC:
      /* Truncate existing file; fails if nonexistant. */
      result = HGFS_OPEN_EMPTY;
      break;

   case (O_CREAT | O_EXCL):
      /* Create file; fail if it exists already. */
      result = HGFS_OPEN_CREATE_SAFE;
      break;

   case (O_CREAT | O_TRUNC):
      /* Create file; if it exists already, truncate it. */
      result = HGFS_OPEN_CREATE_EMPTY;
      break;

   default:
      /*
       * This can only happen if all three flags are set, which
       * conceptually makes no sense because O_EXCL and O_TRUNC are
       * mutually exclusive if O_CREAT is set.
       *
       * However, the open(2) man page doesn't say you can't set all
       * three flags, and certain apps (*cough* Nautilus *cough*) do
       * so. To be friendly to those apps, we just silenty drop the
       * O_TRUNC flag on the assumption that it's safer to honor
       * O_EXCL.
       */
      LOG(4, (KERN_DEBUG "VMware hgfs: HgfsGetOpenFlags: invalid open "
              "flags %o. Ignoring the O_TRUNC flag.\n", flags));
      result = HGFS_OPEN_CREATE_SAFE;
      break;
   }

   return result;
}


/*
 * HGFS file operations for files.
 */

/*
 *----------------------------------------------------------------------
 *
 * HgfsOpen --
 *
 *    Called whenever a process opens a file in our filesystem.
 *
 *    We send an "Open" request to the server with the name stored in
 *    this file's inode. If the Open succeeds, we store the filehandle
 *    sent by the server in the file struct so it can be accessed by
 *    read/write/close.
 *
 * Results:
 *    Returns zero if on success, error on failure.
 *
 * Side effects:
 *    None
 *
 *----------------------------------------------------------------------
 */

static int
HgfsOpen(struct inode *inode,  // IN: Inode of the file to open
         struct file *file)    // IN: File pointer for this open
{
   HgfsSuperInfo *si;
   HgfsReq *req;
   HgfsOp opUsed;
   HgfsRequest *requestHeader;
   HgfsReply *replyHeader;
   HgfsHandle replyFile;
   HgfsServerLock replyLock;
   HgfsInodeInfo *iinfo;
   int result = 0;

   ASSERT(inode);
   ASSERT(inode->i_sb);
   ASSERT(file);
   ASSERT(file->f_dentry);
   ASSERT(file->f_dentry->d_inode);

   si = HGFS_SB_TO_COMMON(inode->i_sb);
   iinfo = INODE_GET_II_P(inode);

   req = HgfsGetNewRequest();
   if (!req) {
      LOG(4, (KERN_DEBUG "VMware hgfs: HgfsOpen: out of memory while "
              "getting new request\n"));
      result = -ENOMEM;
      goto out;
   }

   requestHeader = (HgfsRequest *)(HGFS_REQ_PAYLOAD(req));

  retry:
   /* 
    * Set up pointers using the proper struct This lets us check the 
    * version exactly once and use the pointers later. 
    */
   requestHeader->op = opUsed = atomic_read(&hgfsVersionOpen);
   requestHeader->id = req->id;

   result = HgfsPackOpenRequest(inode, file, req);
   if (result != 0) {
      LOG(4, (KERN_DEBUG "VMware hgfs: HgfsOpen: error packing request\n"));  
      goto out;
   }

   /* Send the request and process the reply. */
   result = HgfsSendRequest(req);
   if (result == 0) {
      /* Get the reply and check return status. */
      replyHeader = (HgfsReply *)(HGFS_REQ_PAYLOAD(req));
      result = HgfsStatusConvertToLinux(replyHeader->status);

      switch (result) {
      case 0:
         iinfo->createdAndUnopened = FALSE;
         result = HgfsUnpackOpenReply(req, opUsed, &replyFile, &replyLock);
         if (result != 0) {
            break;
         }
         result = HgfsCreateFileInfo(file, replyFile);
         if (result != 0) {
            break;
         }
         LOG(6, (KERN_DEBUG "VMware hgfs: HgfsOpen: set handle to %u\n",
                 replyFile));

         /*
          * HgfsCreate faked all of the inode's attributes, so by the time 
          * we're done in HgfsOpen, we need to make sure that the attributes 
          * in the inode are real. The following is only necessary when 
          * O_CREAT is set, otherwise we got here after HgfsLookup (which sent 
          * a getattr to the server and got the real attributes).
          *
          * In particular, we'd like to at least try and set the inode's 
          * uid/gid to match the caller's. We don't expect this to work, 
          * because Windows servers will ignore it, and Linux servers running 
          * as non-root won't be able to change it, but we're forward thinking 
          * people.
          * 
          * Either way, we force a revalidate following the setattr so that 
          * we'll get the actual uid/gid from the server.
          */
         if (file->f_flags & O_CREAT) {
            struct iattr setUidGid;
            
            setUidGid.ia_valid = ATTR_UID | ATTR_GID;
            setUidGid.ia_uid = current->fsuid;
            /* 
             * XXX: How can we handle SGID from here? We would need access to 
             * this dentry's parent inode's mode and gid.
             *
             * After the setattr, we desperately want a revalidate so we can
             * get the true attributes from the server. However, the setattr
             * may have done that for us. To prevent a spurious revalidate,
             * reset the dentry's time before the setattr. That way, if setattr
             * ends up revalidating the dentry, the subsequent call to 
             * revalidate will do nothing.
             */
            setUidGid.ia_gid = current->fsgid;
            HgfsDentryAgeForce(file->f_dentry);
            HgfsSetattr(file->f_dentry, &setUidGid);
            HgfsRevalidate(file->f_dentry);
         }
         break;

      case -EPROTO:
         /* Retry with Version 1 of Open. Set globally. */
         if (opUsed == HGFS_OP_OPEN_V2) {
            LOG(4, (KERN_DEBUG "VMware hgfs: HgfsOpen: Version 2 not "
                    "supported. Falling back to version 1.\n"));
            atomic_set(&hgfsVersionOpen, HGFS_OP_OPEN);
            goto retry;
         }
         
         /* Fallthrough. */
      default:
         break;
      }
   } else if (result == -EIO) {
      LOG(4, (KERN_DEBUG "VMware hgfs: HgfsOpen: timed out\n"));
   } else if (result == -EPROTO) {
      LOG(4, (KERN_DEBUG "VMware hgfs: HgfsOpen: server "
              "returned error: %d\n", result));
   } else {
      LOG(4, (KERN_DEBUG "VMware hgfs: HgfsOpen: unknown error: "
              "%d\n", result));
   }
out:
   HgfsFreeRequest(req);

   /* 
    * If the open failed (for any reason) and we tried to open a newly created 
    * file, we must ensure that the next operation on this inode triggers a 
    * revalidate to the server. This is because the file wasn't created on the 
    * server, yet we currently believe that it was, because we created a fake
    * inode with a hashed dentry for it in HgfsCreate. We will continue to
    * believe this until the dentry's ttl expires, which will cause a 
    * revalidate to the server that will reveal the truth. So in order to find
    * the truth as soon as possible, we'll reset the dentry's last revalidate 
    * time now to force a revalidate the next time someone uses the dentry.
    *
    * We're using our own flag to track this case because using O_CREAT isn't 
    * good enough: HgfsOpen will be called with O_CREAT even if the file exists
    * on the server, and if that's the case, there's no need to revalidate.
    *
    * XXX: Note that this will need to be reworked if/when we support hard
    * links, because multiple dentries will point to the same inode, and
    * forcing a revalidate on one will not force it on any others.
    */
   if (result != 0 && iinfo->createdAndUnopened == TRUE) {
      HgfsDentryAgeForce(file->f_dentry);
   }
   return result;
}


#if defined(VMW_USE_AIO)
/*
 *----------------------------------------------------------------------
 *
 * HgfsAioRead --
 *
 *    Called when the kernel initiates an asynchronous read to a file in
 *    our filesystem. Our function is just a thin wrapper around 
 *    generic_file_aio_read() that tries to validate the dentry first.
 *
 * Results:
 *    Returns the number of bytes read on success, or an error on
 *    failure.
 *
 * Side effects:
 *    None
 *
 *----------------------------------------------------------------------
 */

static ssize_t 
HgfsAioRead(struct kiocb *iocb,      // IN:  I/O control block
            const struct iovec *iov, // OUT: Array of I/O buffers
            unsigned long numSegs,   // IN:  Number of buffers
            loff_t offset)           // IN:  Offset at which to read
{
   int result;

   ASSERT(iocb);
   ASSERT(iocb->ki_filp);
   ASSERT(iocb->ki_filp->f_dentry);
   ASSERT(iov);

   LOG(6, (KERN_DEBUG "VMware hgfs: HgfsAioRead: was called\n"));

   result = HgfsRevalidate(iocb->ki_filp->f_dentry);
   if (result) {
      LOG(4, (KERN_DEBUG "VMware hgfs: HgfsAioRead: invalid dentry\n"));
      goto out;
   }

   result = generic_file_aio_read(iocb, iov, numSegs, offset);
  out:
   return result;
}


/*
 *----------------------------------------------------------------------
 *
 * HgfsAioWrite --
 *
 *    Called when the kernel initiates an asynchronous write to a file in
 *    our filesystem. Our function is just a thin wrapper around 
 *    generic_file_aio_write() that tries to validate the dentry first.
 *
 *    Note that files opened with O_SYNC (or superblocks mounted with
 *    "sync") are synchronously written to by the VFS.
 *
 * Results:
 *    Returns the number of bytes written on success, or an error on
 *    failure.
 *
 * Side effects:
 *    None
 *
 *----------------------------------------------------------------------
 */

static ssize_t 
HgfsAioWrite(struct kiocb *iocb,      // IN:  I/O control block
             const struct iovec *iov, // IN:  Array of I/O buffers
             unsigned long numSegs,   // IN:  Number of buffers
             loff_t offset)           // IN:  Offset at which to read
{
   int result;

   ASSERT(iocb);
   ASSERT(iocb->ki_filp);
   ASSERT(iocb->ki_filp->f_dentry);
   ASSERT(iov);

   LOG(6, (KERN_DEBUG "VMware hgfs: HgfsAioWrite: was called\n"));
   
   result = HgfsRevalidate(iocb->ki_filp->f_dentry);
   if (result) {
      LOG(4, (KERN_DEBUG "VMware hgfs: HgfsAioWrite: invalid dentry\n"));
      goto out;
   }

   result = generic_file_aio_write(iocb, iov, numSegs, offset);   
  out:
   return result;
}


#else
/*
 *----------------------------------------------------------------------
 *
 * HgfsRead --
 *
 *    Called whenever a process reads from a file in our filesystem. Our
 *    function is just a thin wrapper around generic_read_file() that
 *    tries to validate the dentry first.
 *
 * Results:
 *    Returns the number of bytes read on success, or an error on
 *    failure.
 *
 * Side effects:
 *    None
 *
 *----------------------------------------------------------------------
 */

static ssize_t
HgfsRead(struct file *file,  // IN:  File to read from
         char __user *buf,   // OUT: User buffer to copy data into
         size_t count,       // IN:  Number of bytes to read
         loff_t *offset)     // IN:  Offset at which to read
{
   int result;

   ASSERT(file);
   ASSERT(file->f_dentry);
   ASSERT(buf);
   ASSERT(offset);

   LOG(6, (KERN_DEBUG "VMware hgfs: HgfsRead: read %Zu bytes from fh %u "
           "at offset %Lu\n", count, FILE_GET_FI_P(file)->handle, *offset));

   result = HgfsRevalidate(file->f_dentry);
   if (result) {
      LOG(4, (KERN_DEBUG "VMware hgfs: HgfsRead: invalid dentry\n"));
      goto out;
   }

   result = generic_file_read(file, buf, count, offset);
  out:
   return result;
}


/*
 *----------------------------------------------------------------------
 *
 * HgfsWrite --
 *
 *    Called whenever a process writes to a file in our filesystem. Our
 *    function is just a thin wrapper around generic_write_file() that
 *    tries to validate the dentry first.
 *
 *    Note that files opened with O_SYNC (or superblocks mounted with
 *    "sync") are synchronously written to by the VFS.
 *
 * Results:
 *    Returns the number of bytes written on success, or an error on
 *    failure.
 *
 * Side effects:
 *    None
 *
 *----------------------------------------------------------------------
 */

static ssize_t
HgfsWrite(struct file *file,      // IN: File to write to
          const char __user *buf, // IN: User buffer where the data is
          size_t count,           // IN: Number of bytes to write
          loff_t *offset)         // IN: Offset to begin writing at
{
   int result;

   ASSERT(file);
   ASSERT(file->f_dentry);
   ASSERT(file->f_dentry->d_inode);
   ASSERT(buf);
   ASSERT(offset);

   LOG(6, (KERN_DEBUG "VMware hgfs: HgfsWrite: write %Zu bytes to fh %u "
           "at offset %Lu\n", count, FILE_GET_FI_P(file)->handle, *offset));

   result = HgfsRevalidate(file->f_dentry);
   if (result) {
      LOG(4, (KERN_DEBUG "VMware hgfs: HgfsWrite: invalid dentry\n"));
      goto out;
   }

   result = generic_file_write(file, buf, count, offset);
  out:
   return result;
}
#endif


/*
 *----------------------------------------------------------------------
 *
 * HgfsFsync --
 *
 *    Called when user process calls fsync() on hgfs file.
 *
 *    The hgfs protocol doesn't support fsync yet, so for now, we punt
 *    and just return success. This is a little less sketchy than it
 *    might sound, because hgfs skips the buffer cache in the guest
 *    anyway (we always write to the host immediately).
 *
 *    In the future we might want to try harder though, since
 *    presumably the intent of an app calling fsync() is to get the
 *    data onto persistent storage, and as things stand now we're at
 *    the whim of the hgfs server code running on the host to fsync or
 *    not if and when it pleases.
 *
 *    Note that do_fsync will call filemap_fdatawrite() before us and
 *    filemap_fdatawait() after us, so there's no need to do anything
 *    here w.r.t. writing out dirty pages.
 *
 * Results:
 *    Returns zero on success. (Currently always succeeds).
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */

static int
HgfsFsync(struct file *file,		// IN: File we operate on
          struct dentry *dentry,        // IN: Dentry for this file
          int datasync)	                // IN: fdatasync or fsync
{
   LOG(6, (KERN_DEBUG "VMware hgfs: HgfsFsync: was called\n"));

   return 0;
}


/*
 *----------------------------------------------------------------------
 *
 * HgfsMmap --
 *
 *    Called when user process calls mmap() on hgfs file. This is a very
 *    thin wrapper function- we simply attempt to revalidate the
 *    dentry prior to calling generic_file_mmap().
 *
 * Results:
 *    Returns zero on success.
 *    Returns negative error value on failure
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */

static int
HgfsMmap(struct file *file,		// IN: File we operate on
         struct vm_area_struct *vma)	// IN/OUT: VM area information
{
   int result;

   ASSERT(file);
   ASSERT(vma);
   ASSERT(file->f_dentry);

   LOG(6, (KERN_DEBUG "VMware hgfs: HgfsMmap: was called\n"));

   result = HgfsRevalidate(file->f_dentry);
   if (result) {
      LOG(4, (KERN_DEBUG "VMware hgfs: HgfsMmap: invalid dentry\n"));
      goto out;
   }

   result = generic_file_mmap(file, vma);
  out:
   return result;
}


/*
 *----------------------------------------------------------------------
 *
 * HgfsRelease --
 *
 *    Called when the last user of a file closes it, i.e. when the
 *    file's f_count becomes zero.
 *
 * Results:
 *    Returns zero on success, or an error on failure.
 *
 * Side effects:
 *    None
 *
 *----------------------------------------------------------------------
 */

static int
HgfsRelease(struct inode *inode,  // IN: Inode that this file points to
            struct file *file)    // IN: File that is getting released
{
   HgfsSuperInfo *si;
   HgfsReq *req;
   HgfsRequestClose *request;
   HgfsReplyClose *reply;
   HgfsHandle handle;
   int result = 0;

   ASSERT(inode);
   ASSERT(file);
   ASSERT(file->f_dentry);
   ASSERT(file->f_dentry->d_sb);

   handle = FILE_GET_FI_P(file)->handle;
   LOG(6, (KERN_DEBUG "VMware hgfs: HgfsRelease: close fh %u\n", handle));

   /* 
    * This may be our last open handle to an inode, so we should flush our
    * dirty pages before closing it.
    */
   compat_filemap_write_and_wait(inode->i_mapping);

   HgfsReleaseFileInfo(file);
   si = HGFS_SB_TO_COMMON(file->f_dentry->d_sb);

   req = HgfsGetNewRequest();
   if (!req) {
      LOG(4, (KERN_DEBUG "VMware hgfs: HgfsRelease: out of memory while "
              "getting new request\n"));
      result = -ENOMEM;
      goto out;
   }

   /* Fill in the request's fields. */
   request = (HgfsRequestClose *)(HGFS_REQ_PAYLOAD(req));
   request->header.id = req->id;
   request->header.op = HGFS_OP_CLOSE;
   request->file = handle;
   req->payloadSize = sizeof *request;

   /* Send the request and process the reply. */
   result = HgfsSendRequest(req);
   if (result == 0) {
      /* Get the reply. */
      reply = (HgfsReplyClose *)(HGFS_REQ_PAYLOAD(req));
      result = HgfsStatusConvertToLinux(reply->header.status);
      if (result == 0) {
         LOG(4, (KERN_DEBUG "VMware hgfs: HgfsRelease: released handle %u\n",
                 handle));
      } else {
         LOG(4, (KERN_DEBUG "VMware hgfs: HgfsRelease: failed handle %u\n",
                 handle));
      }
   } else if (result == -EIO) {
      LOG(4, (KERN_DEBUG "VMware hgfs: HgfsRelease: timed out\n"));
   } else if (result == -EPROTO) {
      LOG(4, (KERN_DEBUG "VMware hgfs: HgfsRelease: server "
              "returned error: %d\n", result));
   } else {
      LOG(4, (KERN_DEBUG "VMware hgfs: HgfsRelease: unknown error: "
              "%d\n", result));
   }

out:
   HgfsFreeRequest(req);
   return result;
}


#ifndef VMW_SENDFILE_NONE
/*
 *-----------------------------------------------------------------------------
 *
 * HgfsSendfile --
 *
 *    sendfile() wrapper for HGFS. Note that this is for sending a file
 *    from HGFS to another filesystem (or socket). To use HGFS as the
 *    destination file in a call to sendfile(), we must implement sendpage()
 *    as well.
 *
 *    Like mmap(), we're just interested in validating the dentry and then
 *    calling into generic_file_sendfile().
 *
 * Results:
 *    Returns number of bytes written on success, or an error on failure.
 *
 * Side effects:
 *    None.
 *
 *-----------------------------------------------------------------------------
 */

#if defined(VMW_SENDFILE_OLD)
static ssize_t
HgfsSendfile(struct file *file,    // IN: File to read from
             loff_t *offset,       // IN/OUT: Where to start reading
             size_t count,         // IN: How much to read
             read_actor_t actor,   // IN: Routine to send a page of data
             void __user *target)  // IN: Destination file/socket
#elif defined(VMW_SENDFILE_NEW)
static ssize_t
HgfsSendfile(struct file *file,    // IN: File to read from
             loff_t *offset,       // IN/OUT: Where to start reading
             size_t count,         // IN: How much to read
             read_actor_t actor,   // IN: Routine to send a page of data
             void *target)         // IN: Destination file/socket
#endif
{
   ssize_t result;

   ASSERT(file);
   ASSERT(file->f_dentry);
   ASSERT(target);
   ASSERT(offset);
   ASSERT(actor);

   LOG(6, (KERN_DEBUG "VMware hgfs: HgfsSendfile: was called\n"));

   result = HgfsRevalidate(file->f_dentry);
   if (result) {
      LOG(4, (KERN_DEBUG "VMware hgfs: HgfsSendfile: invalid dentry\n"));
      goto out;
   }

   result = generic_file_sendfile (file, offset, count, actor, target);
  out:
   return result;

}
#endif


#ifdef VMW_SPLICE_READ
/*
 *-----------------------------------------------------------------------------
 *
 * HgfsSpliceRead --
 *
 *    splice_read() wrapper for HGFS. Note that this is for sending a file
 *    from HGFS to another filesystem (or socket). To use HGFS as the
 *    destination file in a call to splice, we must implement splice_write()
 *    as well.
 *
 *    Like mmap(), we're just interested in validating the dentry and then
 *    calling into generic_file_splice_read().
 *
 * Results:
 *    Returns number of bytes written on success, or an error on failure.
 *
 * Side effects:
 *    None.
 *
 *-----------------------------------------------------------------------------
 */

static ssize_t
HgfsSpliceRead(struct file *file,            // IN: File to read from
               loff_t *offset,               // IN/OUT: Where to start reading
               struct pipe_inode_info *pipe, // IN: Pipe where to write data
               size_t len,                   // IN: How much to read
               unsigned int flags)           // IN: Various flags
{
   ssize_t result;

   ASSERT(file);
   ASSERT(file->f_dentry);

   LOG(6, (KERN_DEBUG "VMware hgfs: HgfsSpliceRead: was called\n"));

   result = HgfsRevalidate(file->f_dentry);
   if (result) {
      LOG(4, (KERN_DEBUG "VMware hgfs: HgfsSpliceRead: invalid dentry\n"));
      goto out;
   }

   result = generic_file_splice_read(file, offset, pipe, len, flags);
  out:
   return result;

}
#endif


