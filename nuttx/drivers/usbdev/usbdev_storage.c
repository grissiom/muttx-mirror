/****************************************************************************
 * drivers/usbdev/usbdev_storage.c
 *
 *   Copyright (C) 2008 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <spudmonkey@racsa.co.cr>
 *
 * Mass storage class device.  Bulk-only with SCSI subclass.
 *
 * References:
 *   "Universal Serial Bus Mass Storage Class, Specification Overview,"
 *   Revision 1.2,  USB Implementer's Forum, June 23, 2003.
 *
 *   "Universal Serial Bus Mass Storage Class, Bulk-Only Transport,"
 *   Revision 1.0, USB Implementer's Forum, September 31, 1999.
 *
 *   "SCSI Primary Commands - 3 (SPC-3),"  American National Standard
 *   for Information Technology, May 4, 2005
 *
 *   "SCSI Primary Commands - 4 (SPC-4),"  American National Standard
 *   for Information Technology, July 19, 2008
 *
 *   "SCSI Block Commands -2 (SBC-2)," American National Standard
 *   for Information Technology, November 13, 2004
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <sys/types.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <queue.h>
#include <debug.h>

#include <nuttx/arch.h>
#include <nuttx/fs.h>
#include <nuttx/usb.h>
#include <nuttx/usb_bulk.h>
#include <nuttx/usbdev.h>
#include <nuttx/usbdev_trace.h>

#include "usbdev_storage.h"

/****************************************************************************
 * Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Types
 ****************************************************************************/

/* The internal version of the class driver */

struct usbstrg_driver_s
{
  struct usbdevclass_driver_s drvr;
  FAR struct usbstrg_dev_s    *dev;
};

/* This is what is allocated */

struct usbstrg_alloc_s
{
  struct usbstrg_dev_s    dev;
  struct usbstrg_driver_s drvr;
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

/* Class Driver Support *****************************************************/

static void   usbstrg_ep0incomplete(FAR struct usbdev_ep_s *ep,
                FAR struct usbdev_req_s *req);
static struct usbdev_req_s *usbstrg_allocreq(FAR struct usbdev_ep_s *ep,
                uint16 len);
static void   usbstrg_freereq(FAR struct usbdev_ep_s *ep,
                FAR struct usbdev_req_s *req);
static int    usbstrg_mkstrdesc(ubyte id, struct usb_strdesc_s *strdesc);
#ifdef CONFIG_USBDEV_DUALSPEED
static sint16 usbstrg_mkcfgdesc(ubyte *buf, ubyte speed);
#else
static sint16 usbstrg_mkcfgdesc(ubyte *buf);
#endif

/* Class Driver Operations (most at interrupt level) ************************/

static int    usbstrg_bind(FAR struct usbdev_s *dev,
                FAR struct usbdevclass_driver_s *driver);
static void   usbstrg_unbind(FAR struct usbdev_s *dev);
static int    usbstrg_setup(FAR struct usbdev_s *dev,
                FAR const struct usb_ctrlreq_s *ctrl);
static void   usbstrg_disconnect(FAR struct usbdev_s *dev);

/* Initialization/Uninitialization ******************************************/

static void   usbstrg_lununinitialize(struct usbstrg_lun_s *lun);

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* Driver operations ********************************************************/

static struct usbdevclass_driverops_s g_driverops =
{
  usbstrg_bind,       /* bind */
  usbstrg_unbind,     /* unbind */
  usbstrg_setup,      /* setup */
  usbstrg_disconnect, /* disconnect */
  NULL,               /* suspend */
  NULL                /* resume */
};

/* Descriptors **************************************************************/

/* Device descriptor */

static const struct usb_devdesc_s g_devdesc =
{
  USB_SIZEOF_DEVDESC,                           /* len */
  USB_DESC_TYPE_DEVICE,                         /* type */
  {LSBYTE(0x0200), MSBYTE(0x0200)},             /* usb */
  USB_CLASS_PER_INTERFACE,                      /* class */
  0,                                            /* subclass */
  0,                                            /* protocol */
  CONFIG_USBSTRG_EP0MAXPACKET,                  /* maxpacketsize */
  { LSBYTE(CONFIG_USBSTRG_VENDORID),            /* vendor */
    MSBYTE(CONFIG_USBSTRG_VENDORID) },
  { LSBYTE(CONFIG_USBSTRG_PRODUCTID),           /* product */
    MSBYTE(CONFIG_USBSTRG_PRODUCTID) },
  { LSBYTE(CONFIG_USBSTRG_VERSIONNO),           /* device */
    MSBYTE(CONFIG_USBSTRG_VERSIONNO) },
  USBSTRG_MANUFACTURERSTRID,                    /* imfgr */
  USBSTRG_PRODUCTSTRID,                         /* iproduct */
  USBSTRG_SERIALSTRID,                          /* serno */
  USBSTRG_NCONFIGS                              /* nconfigs */
};

/* Configuration descriptor */

static const struct usb_cfgdesc_s g_cfgdesc =
{
  USB_SIZEOF_CFGDESC,                           /* len */
  USB_DESC_TYPE_CONFIG,                         /* type */
  {0, 0},                                       /* totallen -- to be provided */
  USBSTRG_NINTERFACES,                          /* ninterfaces */
  USBSTRG_CONFIGID,                             /* cfgvalue */
  USBSTRG_CONFIGSTRID,                          /* icfg */
  USB_CONFIG_ATTR_ONE|SELFPOWERED|REMOTEWAKEUP, /* attr */
  (CONFIG_USBDEV_MAXPOWER + 1) / 2              /* mxpower */
};

/* Single interface descriptor */

static const struct usb_ifdesc_s g_ifdesc =
{
  USB_SIZEOF_IFDESC,                            /* len */
  USB_DESC_TYPE_INTERFACE,                      /* type */
  0,                                            /* ifno */
  0,                                            /* alt */
  USBSTRG_NENDPOINTS,                           /* neps */
  USB_CLASS_MASS_STORAGE,                       /* class */
  SUBSTRG_SUBCLASS_SCSI,                        /* subclass */
  USBSTRG_PROTO_BULKONLY,                       /* protocol */
  USBSTRG_CONFIGSTRID                           /* iif */
};

/* Endpoint descriptors */

static const struct usb_epdesc_s g_fsepbulkoutdesc =
{
  USB_SIZEOF_EPDESC,                            /* len */
  USB_DESC_TYPE_ENDPOINT,                       /* type */
  USBSTRG_EPOUTBULK_ADDR,                       /* addr */
  USBSTRG_EPOUTBULK_ATTR,                       /* attr */
  { LSBYTE(USBSTRG_FSBULKMAXPACKET),            /* maxpacket */
    MSBYTE(USBSTRG_FSBULKMAXPACKET) },
  0                                             /* interval */
};

static const struct usb_epdesc_s g_fsepbulkindesc =
{
  USB_SIZEOF_EPDESC,                            /* len */
  USB_DESC_TYPE_ENDPOINT,                       /* type */
  USBSTRG_EPINBULK_ADDR,                        /* addr */
  USBSTRG_EPINBULK_ATTR,                        /* attr */
  { LSBYTE(USBSTRG_FSBULKMAXPACKET),            /* maxpacket */
    MSBYTE(USBSTRG_FSBULKMAXPACKET) },
  0                                             /* interval */
};

#ifdef  CONFIG_USBDEV_DUALSPEED
static const struct usb_qualdesc_s g_qualdesc =
{
  USB_SIZEOF_QUALDESC,                          /* len */
  USB_DESC_TYPE_DEVICEQUALIFIER,                /* type */
  {LSBYTE(0x0200), MSBYTE(0x0200) },            /* USB */
  USB_CLASS_PER_INTERFACE,                      /* class */
  0,                                            /* subclass */
  0,                                            /* protocol */
  CONFIG_USBSTRG_EP0MAXPACKET,                  /* mxpacketsize */
  USBSTRG_NCONFIGS,                             /* nconfigs */
  0,                                            /* reserved */
};

static const struct usb_epdesc_s g_hsepbulkoutdesc =
{
  USB_SIZEOF_EPDESC,                            /* len */
  USB_DESC_TYPE_ENDPOINT,                       /* type */
  USBSTRG_EPOUTBULK_ADDR,                       /* addr */
  USBSTRG_EPOUTBULK_ATTR,                       /* attr */
  { LSBYTE(USBSTRG_HSBULKMAXPACKET),            /* maxpacket */
    MSBYTE(USBSTRG_HSBULKMAXPACKET) },
  0                                             /* interval */
};

static const struct usb_epdesc_s g_hsepbulkindesc =
{
  USB_SIZEOF_EPDESC,                            /* len */
  USB_DESC_TYPE_ENDPOINT,                       /* type */
  USBSTRG_EPINBULK_ADDR,                        /* addr */
  USBSTRG_EPINBULK_ATTR,                        /* attr */
  { LSBYTE(USBSTRG_HSBULKMAXPACKET),            /* maxpacket */
    MSBYTE(USBSTRG_HSBULKMAXPACKET) },
  0                                             /* interval */
};
#endif

/****************************************************************************
 * Public Data
 ****************************************************************************/

/* String *******************************************************************/

const char g_vendorstr[]  = CONFIG_USBSTRG_VENDORSTR;
const char g_productstr[] = CONFIG_USBSTRG_PRODUCTSTR;
const char g_serialstr[]  = CONFIG_USBSTRG_SERIALSTR;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Class Driver Support
 ****************************************************************************/
/****************************************************************************
 * Name: usbstrg_ep0incomplete
 *
 * Description:
 *   Handle completion of EP0 control operations
 *
 ****************************************************************************/

static void usbstrg_ep0incomplete(FAR struct usbdev_ep_s *ep,
                                  FAR struct usbdev_req_s *req)
{
  if (req->result || req->xfrd != req->len)
    {
      usbtrace(TRACE_CLSERROR(USBSTRG_TRACEERR_REQRESULT), (uint16)-req->result);
    }
}

/****************************************************************************
 * Name: usbstrg_allocreq
 *
 * Description:
 *   Allocate a request instance along with its buffer
 *
 ****************************************************************************/

static struct usbdev_req_s *usbstrg_allocreq(FAR struct usbdev_ep_s *ep, uint16 len)
{
  FAR struct usbdev_req_s *req;

  req = EP_ALLOCREQ(ep);
  if (req != NULL)
    {
      req->len = len;
      req->buf = EP_ALLOCBUFFER(ep, len);
      if (!req->buf)
        {
          EP_FREEREQ(ep, req);
          req = NULL;
        }
    }
  return req;
}

/****************************************************************************
 * Name: usbstrg_freereq
 *
 * Description:
 *   Free a request instance along with its buffer
 *
 ****************************************************************************/

static void usbstrg_freereq(FAR struct usbdev_ep_s *ep, struct usbdev_req_s *req)
{
  if (ep != NULL && req != NULL)
    {
      if (req->buf != NULL)
        {
          EP_FREEBUFFER(ep, req->buf);
        }
      EP_FREEREQ(ep, req);
    }
}

/****************************************************************************
 * Name: usbstrg_mkstrdesc
 *
 * Description:
 *   Construct a string descriptor
 *
 ****************************************************************************/

static int usbstrg_mkstrdesc(ubyte id, struct usb_strdesc_s *strdesc)
{
  const char *str;
  int len;
  int ndata;
  int i;

  switch (id)
    {
    case 0:
      {
        /* Descriptor 0 is the language id */

        strdesc->len     = 4;
        strdesc->type    = USB_DESC_TYPE_STRING;
        strdesc->data[0] = LSBYTE(USBSTRG_STR_LANGUAGE);
        strdesc->data[1] = MSBYTE(USBSTRG_STR_LANGUAGE);
        return 4;
      }

    case USBSTRG_MANUFACTURERSTRID:
      str = g_vendorstr;
      break;

    case USBSTRG_PRODUCTSTRID:
      str = g_productstr;
      break;

    case USBSTRG_SERIALSTRID:
      str = g_serialstr;
      break;

    case USBSTRG_CONFIGSTRID:
      str = CONFIG_USBSTRG_CONFIGSTR;
      break;

    default:
      return -EINVAL;
    }

   /* The string is utf16-le.  The poor man's utf-8 to utf16-le
    * conversion below will only handle 7-bit en-us ascii
    */

   len = strlen(str);
   for (i = 0, ndata = 0; i < len; i++, ndata += 2)
     {
       strdesc->data[ndata]   = str[i];
       strdesc->data[ndata+1] = 0;
     }

   strdesc->len  = ndata+2;
   strdesc->type = USB_DESC_TYPE_STRING;
   return strdesc->len;
}

/****************************************************************************
 * Name: usbstrg_mkcfgdesc
 *
 * Description:
 *   Construct the configuration descriptor
 *
 ****************************************************************************/

#ifdef CONFIG_USBDEV_DUALSPEED
static sint16 usbstrg_mkcfgdesc(ubyte *buf, ubyte speed)
#else
static sint16 usbstrg_mkcfgdesc(ubyte *buf)
#endif
{
  FAR struct usb_cfgdesc_s *cfgdesc = (struct usb_cfgdesc_s*)buf;
#ifdef CONFIG_USBDEV_DUALSPEED
  FAR struct usb_epdesc_s *epdesc;
  boolean hispeed = (speed == USB_SPEED_HIGH);
  uint16 bulkmxpacket;
#endif
  uint16 totallen;

  /* This is the total length of the configuration (not necessarily the
   * size that we will be sending now.
   */

  totallen = USB_SIZEOF_CFGDESC + USB_SIZEOF_IFDESC + USBSTRG_NENDPOINTS * USB_SIZEOF_EPDESC;

  /* Configuration descriptor -- Copy the canned descriptor and fill in the
   * type (we'll also need to update the size below
   */

  memcpy(cfgdesc, &g_cfgdesc, USB_SIZEOF_CFGDESC);
  buf += USB_SIZEOF_CFGDESC;

  /*  Copy the canned interface descriptor */

  memcpy(buf, &g_ifdesc, USB_SIZEOF_IFDESC);
  buf += USB_SIZEOF_IFDESC;

  /* Make the two endpoint configurations */

#ifdef CONFIG_USBDEV_DUALSPEED
  /* Check for switches between high and full speed */

  hispeed = (speed == USB_SPEED_HIGH);
  if (type == USB_DESC_TYPE_OTHERSPEEDCONFIG)
    {
      hispeed = !hispeed;
    }

  bulkmxpacket = USBSTRG_BULKMAXPACKET(hispeed);
  epdesc       = USBSTRG_EPBULKINDESC(hispeed);
  memcpy(buf, epdesc, USB_SIZEOF_EPDESC);
  buf += USB_SIZEOF_EPDESC;

  epdesc       = USBSTRG_EPBULKOUTDESC(hispeed);
  memcpy(buf, epdesc, USB_SIZEOF_EPDESC);
#else
  memcpy(buf, &g_fsepbulkoutdesc, USB_SIZEOF_EPDESC);
  buf += USB_SIZEOF_EPDESC;
  memcpy(buf, &g_fsepbulkindesc, USB_SIZEOF_EPDESC);
#endif

  /* Finally, fill in the total size of the configuration descriptor */

  cfgdesc->totallen[0] = LSBYTE(totallen);
  cfgdesc->totallen[1] = MSBYTE(totallen);
  return totallen;
}

/****************************************************************************
 * Class Driver Interfaces
 ****************************************************************************/
/****************************************************************************
 * Name: usbstrg_bind
 *
 * Description:
 *   Invoked when the driver is bound to a USB device driver
 *
 ****************************************************************************/

static int usbstrg_bind(FAR struct usbdev_s *dev, FAR struct usbdevclass_driver_s *driver)
{
  FAR struct usbstrg_dev_s *priv = ((struct usbstrg_driver_s*)driver)->dev;
  FAR struct usbstrg_req_s *reqcontainer;
  irqstate_t flags;
  int ret;
  int i;

  usbtrace(TRACE_CLASSBIND, 0);

  /* Bind the structures */

  priv->usbdev      = dev;
  dev->ep0->private = priv;

  /* The configured EP0 size should match the reported EP0 size.  We could
   * easily adapt to the reported EP0 size, but then we could not use the
   * const, canned descriptors.
   */

  DEBUGASSERT(CONFIG_USBSTRG_EP0MAXPACKET == dev->ep0->maxpacket);

  /* Preallocate control request */

  priv->ctrlreq = usbstrg_allocreq(dev->ep0, USBSTRG_MXDESCLEN);
  if (priv->ctrlreq == NULL)
    {
      usbtrace(TRACE_CLSERROR(USBSTRG_TRACEERR_ALLOCCTRLREQ), 0);
      ret = -ENOMEM;
      goto errout;
    }
  priv->ctrlreq->callback = usbstrg_ep0incomplete;

  /* Pre-allocate all endpoints... the endpoints will not be functional
   * until the SET CONFIGURATION request is processed in usbstrg_setconfig.
   * This is done here because there may be calls to malloc and the SET
   * CONFIGURATION processing probably occurrs within interrupt handling
   * logic where malloc calls will fail.
   */

  /* Pre-allocate the IN bulk endpoint */

  priv->epbulkin = DEV_ALLOCEP(dev, USBSTRG_EPINBULK_ADDR, TRUE, USB_EP_ATTR_XFER_BULK);
  if (!priv->epbulkin)
    {
      usbtrace(TRACE_CLSERROR(USBSTRG_TRACEERR_EPBULKINALLOCFAIL), 0);
      ret = -ENODEV;
      goto errout;
    }
  priv->epbulkin->private = priv;

  /* Pre-allocate the OUT bulk endpoint */

  priv->epbulkout = DEV_ALLOCEP(dev, USBSTRG_EPOUTBULK_ADDR, FALSE, USB_EP_ATTR_XFER_BULK);
  if (!priv->epbulkout)
    {
      usbtrace(TRACE_CLSERROR(USBSTRG_TRACEERR_EPBULKOUTALLOCFAIL), 0);
      ret = -ENODEV;
      goto errout;
    }
  priv->epbulkout->private = priv;

  /* Pre-allocate read requests */

  for (i = 0; i < CONFIG_USBSTRG_NRDREQS; i++)
    {
      reqcontainer      = &priv->rdreqs[i];
      reqcontainer->req = usbstrg_allocreq(priv->epbulkout, CONFIG_USBSTRG_BULKOUTREQLEN);
      if (reqcontainer->req == NULL)
        {
          usbtrace(TRACE_CLSERROR(USBSTRG_TRACEERR_RDALLOCREQ), (uint16)-ret);
          ret = -ENOMEM;
          goto errout;
        }
      reqcontainer->req->private  = reqcontainer;
      reqcontainer->req->callback = usbstrg_rdcomplete;
    }

  /* Pre-allocate write request containers and put in a free list */

  for (i = 0; i < CONFIG_USBSTRG_NWRREQS; i++)
    {
      reqcontainer      = &priv->wrreqs[i];
      reqcontainer->req = usbstrg_allocreq(priv->epbulkin, CONFIG_USBSTRG_BULKINREQLEN);
      if (reqcontainer->req == NULL)
        {
          usbtrace(TRACE_CLSERROR(USBSTRG_TRACEERR_WRALLOCREQ), (uint16)-ret);
          ret = -ENOMEM;
          goto errout;
        }
      reqcontainer->req->private  = reqcontainer;
      reqcontainer->req->callback = usbstrg_wrcomplete;

      flags = irqsave();
      sq_addlast((sq_entry_t*)reqcontainer, &priv->wrreqlist);
      irqrestore(flags);
    }

  /* Report if we are selfpowered */

#ifdef CONFIG_USBDEV_SELFPOWERED
  DEV_SETSELFPOWERED(dev);
#endif
  return OK;

errout:
  usbstrg_unbind(dev);
  return ret;
}

/****************************************************************************
 * Name: usbstrg_unbind
 *
 * Description:
 *    Invoked when the driver is unbound from a USB device driver
 *
 ****************************************************************************/

static void usbstrg_unbind(FAR struct usbdev_s *dev)
{
  FAR struct usbstrg_dev_s *priv;
  FAR struct usbstrg_req_s *reqcontainer;
  irqstate_t flags;
  int i;

  usbtrace(TRACE_CLASSUNBIND, 0);

#ifdef CONFIG_DEBUG
  if (!dev || !dev->ep0)
    {
      usbtrace(TRACE_CLSERROR(USBSTRG_TRACEERR_UNBINDINVALIDARGS), 0);
      return;
     }
#endif

  /* Extract reference to private data */

  priv = (FAR struct usbstrg_dev_s *)dev->ep0->private;

#ifdef CONFIG_DEBUG
  if (!priv)
    {
      usbtrace(TRACE_CLSERROR(USBSTRG_TRACEERR_EP0NOTBOUND1), 0);
      return;
    }
#endif

  /* The worker thread should have already been stopped by the
   * driver un-initialize logic.
   */

  DEBUGASSERT(priv->thstate == USBSTRG_STATE_TERMINATED);

  /* Make sure that we are not already unbound */

  if (priv != NULL)
    {
      /* Make sure that the endpoints have been unconfigured.  If
       * we were terminated gracefully, then the configuration should
       * already have been reset.  If not, then calling usbstrg_resetconfig
       * should cause the endpoints to immediately terminate all
       * transfers and return the requests to us (with result == -ESHUTDOWN)
       */

      usbstrg_resetconfig(priv);
      up_mdelay(50);

      /* Free the bulk IN endpoint */

      if (priv->epbulkin)
        {
          DEV_FREEEP(dev, priv->epbulkin);
          priv->epbulkin = NULL;
        }

      /* Free the pre-allocated control request */

      if (priv->ctrlreq != NULL)
        {
          usbstrg_freereq(dev->ep0, priv->ctrlreq);
          priv->ctrlreq = NULL;
        }

      /* Free pre-allocated read requests (which should all have
       * been returned to the free list at this time -- we don't check)
       */

      for (i = 0; i < CONFIG_USBSTRG_NRDREQS; i++)
        {
          reqcontainer = &priv->rdreqs[i];
          if (reqcontainer->req)
            {
              usbstrg_freereq(priv->epbulkout, reqcontainer->req);
              reqcontainer->req = NULL;
            }
        }

      /* Free the bulk OUT endpoint */

      if (priv->epbulkout)
        {
          DEV_FREEEP(dev, priv->epbulkout);
          priv->epbulkout = NULL;
        }

      /* Free write requests that are not in use (which should be all
       * of them
       */

      flags = irqsave();
      while (!sq_empty(&priv->wrreqlist))
        {
          reqcontainer = (struct usbstrg_req_s *)sq_remfirst(&priv->wrreqlist);
          if (reqcontainer->req != NULL)
            {
              usbstrg_freereq(priv->epbulkin, reqcontainer->req);
            }
        }
      irqrestore(flags);
    }
}

/****************************************************************************
 * Name: usbstrg_setup
 *
 * Description:
 *   Invoked for ep0 control requests.  This function probably executes
 *   in the context of an interrupt handler.
 *
 ****************************************************************************/

static int usbstrg_setup(FAR struct usbdev_s *dev, const struct usb_ctrlreq_s *ctrl)
{
  FAR struct usbstrg_dev_s *priv;
  FAR struct usbdev_req_s *ctrlreq;
  uint16 value;
  uint16 index;
  uint16 len;
  int ret = -EOPNOTSUPP;

#ifdef CONFIG_DEBUG
  if (!dev || !dev->ep0 || !ctrl)
    {
      usbtrace(TRACE_CLSERROR(USBSTRG_TRACEERR_SETUPINVALIDARGS), 0);
      return -EIO;
     }
#endif

  /* Extract reference to private data */

  usbtrace(TRACE_CLASSSETUP, ctrl->req);
  priv = (FAR struct usbstrg_dev_s *)dev->ep0->private;

#ifdef CONFIG_DEBUG
  if (!priv || !priv->ctrlreq)
    {
      usbtrace(TRACE_CLSERROR(USBSTRG_TRACEERR_EP0NOTBOUND2), 0);
      return -ENODEV;
    }
#endif
  ctrlreq = priv->ctrlreq;

  /* Extract the little-endian 16-bit values to host order */

  value = GETUINT16(ctrl->value);
  index = GETUINT16(ctrl->index);
  len   = GETUINT16(ctrl->len);

  uvdbg("type=%02x req=%02x value=%04x index=%04x len=%04x\n",
        ctrl->type, ctrl->req, value, index, len);

  if ((ctrl->type & USB_REQ_TYPE_MASK) == USB_REQ_TYPE_STANDARD)
    {
      /**********************************************************************
       * Standard Requests
       **********************************************************************/

      switch (ctrl->req)
        {
        case USB_REQ_GETDESCRIPTOR:
          {
            /* The value field specifies the descriptor type in the MS byte and the
             * descriptor index in the LS byte (order is little endian)
             */

            switch (ctrl->value[1])
              {
              case USB_DESC_TYPE_DEVICE:
                {
                  ret = USB_SIZEOF_DEVDESC;
                  memcpy(ctrlreq->buf, &g_devdesc, ret);
                }
                break;

#ifdef CONFIG_USBDEV_DUALSPEED
              case USB_DESC_TYPE_DEVICEQUALIFIER:
                {
                  ret = USB_SIZEOF_QUALDESC;
                  memcpy(ctrlreq->buf, &g_qualdesc, ret);
                }
                break;

              case USB_DESC_TYPE_OTHERSPEEDCONFIG:
#endif /* CONFIG_USBDEV_DUALSPEED */

              case USB_DESC_TYPE_CONFIG:
                {
#ifdef CONFIG_USBDEV_DUALSPEED
                  ret = usbstrg_mkcfgdesc(ctrlreq->buf, dev->speed, len);
#else
                  ret = usbstrg_mkcfgdesc(ctrlreq->buf);
#endif
                }
                break;

              case USB_DESC_TYPE_STRING:
                {
                  /* index == language code. */

                  ret = usbstrg_mkstrdesc(ctrl->value[0], (struct usb_strdesc_s *)ctrlreq->buf);
                }
                break;

              default:
                {
                  usbtrace(TRACE_CLSERROR(USBSTRG_TRACEERR_GETUNKNOWNDESC), value);
                }
                break;
              }
          }
          break;

        case USB_REQ_SETCONFIGURATION:
          {
            if (ctrl->type == 0)
              {
                /* Signal the worker thread to instantiate the new configuration */

                priv->theventset |= USBSTRG_EVENT_CFGCHANGE;
                priv->thvalue     = value;
                pthread_cond_signal(&priv->cond);

                /* Return here... the response will be provided later by the
                 * worker thread.
                 */

                return OK;
              }
          }
          break;

        case USB_REQ_GETCONFIGURATION:
          {
            if (ctrl->type == USB_DIR_IN)
              {
                ctrlreq->buf[0] = priv->config;
                ret = 1;
              }
          }
          break;

        case USB_REQ_SETINTERFACE:
          {
            if (ctrl->type == USB_REQ_RECIPIENT_INTERFACE)
              {
                if (priv->config == USBSTRG_CONFIGID &&
                    index == USBSTRG_INTERFACEID &&
                    value == USBSTRG_ALTINTERFACEID)
                  {
                    /* Signal to instantiate the interface change */

                    priv->theventset |= USBSTRG_EVENT_IFCHANGE;
                    pthread_cond_signal(&priv->cond);

                    /* Return here... the response will be provided later by the
                     * worker thread.
                     */

                    return OK;
                  }
              }
          }
          break;

        case USB_REQ_GETINTERFACE:
          {
            if (ctrl->type == (USB_DIR_IN|USB_REQ_RECIPIENT_INTERFACE) &&
                priv->config == USBSTRG_CONFIGIDNONE)
              {
                if (index != USBSTRG_INTERFACEID)
                  {
                    ret = -EDOM;
                  }
                else
                  {
                    ctrlreq->buf[0] = USBSTRG_ALTINTERFACEID;
                    ret = 1;
                  }
              }
           }
           break;

        default:
          usbtrace(TRACE_CLSERROR(USBSTRG_TRACEERR_UNSUPPORTEDSTDREQ), ctrl->req);
          break;
        }
    }
  else
    {
      /**********************************************************************
       * Bulk-Only Mass Storage Class Requests
       **********************************************************************/

      /* Verify that we are configured */

      if (!priv->config)
        {
           usbtrace(TRACE_CLSERROR(USBSTRG_TRACEERR_NOTCONFIGURED), 0);
           return ret;
        }

      switch (ctrl->req)
        {
        case USBSTRG_REQ_MSRESET: /* Reset mass storage device and interface */
          {
            if (ctrl->type == USBSTRG_TYPE_SETUPOUT && value == 0 && len == 0)
              {
                /* Only one interface is supported */

                if (index != USBSTRG_CONFIGID)
                  {
                    usbtrace(TRACE_CLSERROR(USBSTRG_TRACEERR_MSRESETNDX), index);
                    ret = -EDOM;
                  }
                else
                  {
                    /* Signal to stop the current operation and reinitialize state */

                     priv->theventset |= USBSTRG_EVENT_RESET;
                     pthread_cond_signal(&priv->cond);

                    /* Return here... the response will be provided later by the
                     * worker thread.
                     */

                    return OK;
                  }
              }
          }
          break;

        case USBSTRG_REQ_GETMAXLUN: /* Return number LUNs supported */
          {
            if (ctrl->type != USBSTRG_TYPE_SETUPIN && value == 0)
              {
                /* Only one interface is supported */

                if (index != USBSTRG_CONFIGID)
                  {
                    usbtrace(TRACE_CLSERROR(USBSTRG_TRACEERR_GETMAXLUNNDX), index);
                    ret = -EDOM;
                  }
                else
                  {
                    ctrlreq->buf[0] = priv->nluns - 1;
                    ret = 1;
                  }
              }
          }
          break;

        default:
          usbtrace(TRACE_CLSERROR(USBSTRG_TRACEERR_BADREQUEST), index);
          break;
        }
    }

  /* Respond to the setup command if data was returned.  On an error return
   * value (ret < 0), the USB driver will stall EP0.
   */

  if (ret >= 0)
    {
      ctrlreq->len   = min(len, ret);
      ctrlreq->flags = USBDEV_REQFLAGS_NULLPKT;
      ret            = EP_SUBMIT(dev->ep0, ctrlreq);
      if (ret < 0)
        {
          usbtrace(TRACE_CLSERROR(USBSTRG_TRACEERR_EPRESPQ), (uint16)-ret);
#if 0 /* Not necessary */
          ctrlreq->result = OK;
          usbstrg_ep0incomplete(dev->ep0, ctrlreq);
#endif
        }
    }

  return ret;
}

/****************************************************************************
 * Name: usbstrg_disconnect
 *
 * Description:
 *   Invoked after all transfers have been stopped, when the host is
 *   disconnected.  This function is probably called from the context of an
 *   interrupt handler.
 *
 ****************************************************************************/

static void usbstrg_disconnect(FAR struct usbdev_s *dev)
{
  struct usbstrg_dev_s *priv;
  irqstate_t flags;

  usbtrace(TRACE_CLASSDISCONNECT, 0);

#ifdef CONFIG_DEBUG
  if (!dev || !dev->ep0)
    {
      usbtrace(TRACE_CLSERROR(USBSTRG_TRACEERR_DISCONNECTINVALIDARGS), 0);
      return;
     }
#endif

  /* Extract reference to private data */

  priv = (FAR struct usbstrg_dev_s *)dev->ep0->private;

#ifdef CONFIG_DEBUG
  if (!priv)
    {
      usbtrace(TRACE_CLSERROR(USBSTRG_TRACEERR_EP0NOTBOUND3), 0);
      return;
    }
#endif

  /* Reset the configuration */

  flags = irqsave();
  usbstrg_resetconfig(priv);

  /* Signal the worker thread */

  priv->theventset |= USBSTRG_EVENT_DISCONNECT;
  pthread_cond_signal(&priv->cond);
  irqrestore(flags);
}

/****************************************************************************
 * Initialization/Un-Initialization
 ****************************************************************************/
/****************************************************************************
 * Name: usbstrg_lununinitialize
 ****************************************************************************/

static void usbstrg_lununinitialize(struct usbstrg_lun_s *lun)
{
  /* Has a block driver has been bound to the LUN? */

  if (lun->inode)
    {
      /* Close the block driver */

      (void)close_blockdriver(lun->inode);
    }

  memset(lun, 0, sizeof(struct usbstrg_lun_s *));
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/
/****************************************************************************
 * Internal Interfaces
 ****************************************************************************/

/****************************************************************************
 * Name: usbstrg_setconfig
 *
 * Description:
 *   Set the device configuration by allocating and configuring endpoints and
 *   by allocating and queue read and write requests.
 *
 ****************************************************************************/

int usbstrg_setconfig(FAR struct usbstrg_dev_s *priv, ubyte config)
{
  FAR struct usbstrg_req_s *privreq;
  FAR struct usbdev_req_s *req;
#ifdef CONFIG_USBDEV_DUALSPEED
  struct usb_epdesc_s *epdesc;
  uint16 bulkmxpacket;
#endif
  int i;
  int ret = 0;

#if CONFIG_DEBUG
  if (priv == NULL)
    {
      usbtrace(TRACE_CLSERROR(USBSTRG_TRACEERR_SETCONFIGINVALIDARGS), 0);
      return -EIO;
    }
#endif

  if (config == priv->config)
    {
      /* Already configured -- Do nothing */

      usbtrace(TRACE_CLSERROR(USBSTRG_TRACEERR_ALREADYCONFIGURED), 0);
      return 0;
    }

  /* Discard the previous configuration data */

  usbstrg_resetconfig(priv);

  /* Was this a request to simply discard the current configuration? */

  if (config == USBSTRG_CONFIGIDNONE)
    {
      usbtrace(TRACE_CLSERROR(USBSTRG_TRACEERR_CONFIGNONE), 0);
      return 0;
    }

  /* We only accept one configuration */

  if (config != USBSTRG_CONFIGID)
    {
      usbtrace(TRACE_CLSERROR(USBSTRG_TRACEERR_CONFIGIDBAD), 0);
      return -EINVAL;
    }

  /* Configure the IN bulk endpoint */

#ifdef CONFIG_USBDEV_DUALSPEED
  bulkmxpacket = USBSTRG_BULKMAXPACKET(hispeed);
  epdesc       = USBSTRG_EPBULKINDESC(hispeed);
  ret          = EP_CONFIGURE(priv->epbulkin, epdesc, FALSE);
#else
  ret          = EP_CONFIGURE(priv->epbulkin, &g_fsepbulkindesc, FALSE);
#endif
  if (ret < 0)
    {
      usbtrace(TRACE_CLSERROR(USBSTRG_TRACEERR_EPBULKINCONFIGFAIL), 0);
      goto errout;
    }

  priv->epbulkin->private = priv;

  /* Configure the OUT bulk endpoint */

#ifdef CONFIG_USBDEV_DUALSPEED
  epdesc       = USBSTRG_EPBULKINDESC(hispeed);
  ret          = EP_CONFIGURE(priv->epbulkout, epdesc, TRUE);
#else
  ret          = EP_CONFIGURE(priv->epbulkout, &g_fsepbulkoutdesc, TRUE);
#endif
  if (ret < 0)
    {
      usbtrace(TRACE_CLSERROR(USBSTRG_TRACEERR_EPBULKOUTCONFIGFAIL), 0);
      goto errout;
    }

  priv->epbulkout->private = priv;

  /* Queue read requests in the bulk OUT endpoint */

  for (i = 0; i < CONFIG_USBSTRG_NRDREQS; i++)
    {
      privreq       = &priv->rdreqs[i];
      req           = privreq->req;
      req->len      = CONFIG_USBSTRG_BULKOUTREQLEN;
      req->private  = privreq;
      req->callback = usbstrg_rdcomplete;
      ret           = EP_SUBMIT(priv->epbulkout, req);
      if (ret < 0)
        {
          usbtrace(TRACE_CLSERROR(USBSTRG_TRACEERR_RDSUBMIT), (uint16)-ret);
          goto errout;
        }
    }

  priv->config = config;
  return OK;

errout:
  usbstrg_resetconfig(priv);
  return ret;
}

/****************************************************************************
 * Name: usbstrg_resetconfig
 *
 * Description:
 *   Mark the device as not configured and disable all endpoints.
 *
 ****************************************************************************/

void usbstrg_resetconfig(FAR struct usbstrg_dev_s *priv)
{
  /* Are we configured? */

  if (priv->config != USBSTRG_CONFIGIDNONE)
    {
      /* Yes.. but not anymore */

      priv->config = USBSTRG_CONFIGIDNONE;

      /* Disable endpoints.  This should force completion of all pending
       * transfers.
       */

      EP_DISABLE(priv->epbulkin);
      EP_DISABLE(priv->epbulkout);
    }
}

/****************************************************************************
 * Name: usbstrg_wrcomplete
 *
 * Description:
 *   Handle completion of write request.  This function probably executes
 *   in the context of an interrupt handler.
 *
 ****************************************************************************/

void usbstrg_wrcomplete(FAR struct usbdev_ep_s *ep, FAR struct usbdev_req_s *req)
{
  FAR struct usbstrg_dev_s *priv;
  FAR struct usbstrg_req_s *privreq;
  irqstate_t flags;

  /* Sanity check */

#ifdef CONFIG_DEBUG
  if (!ep || !ep->private || !req || !req->private)
    {
      usbtrace(TRACE_CLSERROR(USBSTRG_TRACEERR_WRCOMPLETEINVALIDARGS), 0);
      return;
     }
#endif

  /* Extract references to private data */

  priv = (FAR struct usbstrg_dev_s*)ep->private;
  privreq = (FAR struct usbstrg_req_s *)req->private;

  /* Return the write request to the free list */

  flags = irqsave();
  sq_addlast((sq_entry_t*)privreq, &priv->wrreqlist);
  irqrestore(flags);

  /* Process the received data unless this is some unusual condition */

  switch (req->result)
    {
    case OK: /* Normal completion */
      usbtrace(TRACE_CLASSWRCOMPLETE, req->xfrd);
      break;

    case -ESHUTDOWN: /* Disconnection */
      usbtrace(TRACE_CLSERROR(USBSTRG_TRACEERR_WRSHUTDOWN), 0);
      break;

    default: /* Some other error occurred */
      usbtrace(TRACE_CLSERROR(USBSTRG_TRACEERR_WRUNEXPECTED), (uint16)-req->result);
      break;
    };

  /* Inform the worker thread that a write request has been returned */

  priv->theventset |= USBSTRG_EVENT_WRCOMPLETE;
  pthread_cond_signal(&priv->cond);
}

/****************************************************************************
 * Name: usbstrg_rdcomplete
 *
 * Description:
 *   Handle completion of read request on the bulk OUT endpoint.  This
 *   is handled like the receipt of serial data on the "UART"
 *
 ****************************************************************************/

void usbstrg_rdcomplete(FAR struct usbdev_ep_s *ep, FAR struct usbdev_req_s *req)
{
  FAR struct usbstrg_dev_s *priv;
  FAR struct usbstrg_req_s *privreq;
  irqstate_t flags;
  int ret;

  /* Sanity check */

#ifdef CONFIG_DEBUG
  if (!ep || !ep->private || !req || !req->private)
    {
      usbtrace(TRACE_CLSERROR(USBSTRG_TRACEERR_RDCOMPLETEINVALIDARGS), 0);
      return;
     }
#endif

  /* Extract references to private data */

  priv    = (FAR struct usbstrg_dev_s*)ep->private;
  privreq = (FAR struct usbstrg_req_s *)req->private;

  /* Process the received data unless this is some unusual condition */

  switch (req->result)
    {
    case 0: /* Normal completion */
      {
        usbtrace(TRACE_CLASSRDCOMPLETE, req->xfrd);

        /* Add the filled read request from the rdreqlist */

        flags = irqsave();
        sq_addlast((sq_entry_t*)privreq, &priv->rdreqlist);
        irqrestore(flags);

        /* Signal the worker thread that there is received data to be processed */

        priv->theventset |= USBSTRG_EVENT_RDCOMPLETE;
        pthread_cond_signal(&priv->cond);
      }
      break;

    case -ESHUTDOWN: /* Disconnection */
      {
        usbtrace(TRACE_CLSERROR(USBSTRG_TRACEERR_RDSHUTDOWN), 0);

        /* Drop the read request... it will be cleaned up later */
      }
      break;

    default: /* Some other error occurred */
      {
        usbtrace(TRACE_CLSERROR(USBSTRG_TRACEERR_RDUNEXPECTED), (uint16)-req->result);

        /* Return the read request to the bulk out endpoint for re-filling */

        req           = privreq->req;
        req->private  = privreq;
        req->callback = usbstrg_rdcomplete;

        ret = EP_SUBMIT(priv->epbulkout, req);
        if (ret != OK)
          {
            usbtrace(TRACE_CLSERROR(USBSTRG_TRACEERR_RDCOMPLETERDSUBMIT), (uint16)-ret);
          }
      }
      break;
    }
}

/****************************************************************************
 * Name: usbstrg_deferredresponse
 *
 * Description:
 *   Some EP0 setup request cannot be responded to immediately becuase they
 *   require some asynchronous action from the SCSI worker thread.  This
 *   function is provided for the SCSI thread to make that deferred response.
 *   The specific requests that require this deferred response are:
 *
 *   1. USB_REQ_SETCONFIGURATION,
 *   2. USB_REQ_SETINTERFACE, or
 *   3. USBSTRG_REQ_MSRESET
 *
 *   In all cases, the success reponse is a zero-length packet; the failure
 *   response is an EP0 stall.
 *
 * Input parameters:
 *   priv  - Private state structure for this USB storage instance
 *   stall - TRUE is the action failed and a stall is required
 *
 ****************************************************************************/

void usbstrg_deferredresponse(FAR struct usbstrg_dev_s *priv, boolean failed)
{
  FAR struct usbdev_s *dev;
  FAR struct usbdev_req_s *ctrlreq;
  int ret;

#ifdef CONFIG_DEBUG
  if (!priv || !priv->usbdev || !priv->ctrlreq)
    {
      usbtrace(TRACE_CLSERROR(USBSTRG_TRACEERR_DEFERREDRESPINVALIDARGS), 0);
      return;
    }
#endif

  dev     = priv->usbdev;
  ctrlreq = priv->ctrlreq;

  /* If no error occurs, respond to the deferred setup command with a null
   * packet.
   */

  if (!failed)
    {
      ctrlreq->len   = 0;
      ctrlreq->flags = USBDEV_REQFLAGS_NULLPKT;
      ret            = EP_SUBMIT(dev->ep0, ctrlreq);
      if (ret < 0)
        {
          usbtrace(TRACE_CLSERROR(USBSTRG_TRACEERR_DEFERREDRESPSUBMIT), (uint16)-ret);
#if 0 /* Not necessary */
          ctrlreq->result = OK;
          usbstrg_ep0incomplete(dev->ep0, ctrlreq);
#endif
        }
    }
  else
    {
      /* On a failure, the USB driver will stall. */

      usbtrace(TRACE_DEVERROR(USBSTRG_TRACEERR_DEFERREDRESPSTALLED), 0);
      EP_STALL(dev->ep0);
    }
}

/****************************************************************************
 * User Interfaces
 ****************************************************************************/
/****************************************************************************
 * Name: usbstrg_configure
 *
 * Description:
 *   One-time initialization of the USB storage driver.  The initialization
 *   sequence is as follows:
 *
 *   1. Call usbstrg_configure to perform one-time initialization specifying
 *      the number of luns.
 *   2. Call usbstrg_bindlun to configure each supported LUN
 *   3. Call usbstrg_exportluns when all LUNs are configured
 *
 * Input Parameters:
 *   nluns  - the number of LUNs that will be registered
 *   handle - Location to return a handle that is used in other API calls.
 *
 * Returned Value:
 *   0 on success; a negated errno on failure
 *
 ****************************************************************************/

int usbstrg_configure(unsigned int nluns, void **handle)
{
  FAR struct usbstrg_alloc_s  *alloc;
  FAR struct usbstrg_dev_s    *priv;
  FAR struct usbstrg_driver_s *drvr;
  int ret;

#ifdef CONFIG_DEBUG
  if (nluns > 15)
    {
      usbtrace(TRACE_CLSERROR(USBSTRG_TRACEERR_TOOMANYLUNS), 0);
      return -EDOM;
    }
#endif

  /* Allocate the structures needed */

  alloc = (FAR struct usbstrg_alloc_s*)malloc(sizeof(struct usbstrg_alloc_s));
  if (!alloc)
    {
      usbtrace(TRACE_CLSERROR(USBSTRG_TRACEERR_ALLOCDEVSTRUCT), 0);
      return -ENOMEM;
    }

  /* Initialize the USB storage driver structure */

  priv = &alloc->dev;
  memset(priv, 0, sizeof(struct usbstrg_dev_s));

  pthread_mutex_init(&priv->mutex, NULL);
  pthread_cond_init(&priv->cond, NULL);
  sq_init(&priv->wrreqlist);

  priv->nluns = nluns;

  /* Allocate the LUN table */

  priv->luntab = (struct usbstrg_lun_s*)malloc(priv->nluns*sizeof(struct usbstrg_lun_s));
  if (!priv->luntab)
    {
      ret = -ENOMEM;
      goto errout;
    }
  memset(priv->luntab, 0, priv->nluns * sizeof(struct usbstrg_lun_s));

  /* Initialize the USB class driver structure */

  drvr             = &alloc->drvr;
#ifdef CONFIG_USBDEV_DUALSPEED
  drvr->drvr.speed = USB_SPEED_HIGH;
#else
  drvr->drvr.speed = USB_SPEED_FULL;
#endif
  drvr->drvr.ops   = &g_driverops;
  drvr->dev        = priv;

  /* Return the handle and success */

  *handle = (FAR void*)alloc;
  return OK;

errout:
  usbstrg_uninitialize(alloc);
  return ret;
}

/****************************************************************************
 * Name: usbstrg_bindlun
 *
 * Description:
 *   Bind the block driver specified by drvrpath to a USB storage LUN.
 *
 * Input Parameters:
 *   handle      - The handle returned by a previous call to usbstrg_configure().
 *   drvrpath    - the full path to the block driver
 *   startsector - A sector offset into the block driver to the start of the
 *                 partition on drvrpath (0 if no partitions)
 *   nsectors    - The number of sectors in the partition (if 0, all sectors
 *                 to the end of the media will be exported).
 *   lunno       - the LUN to bind to
 *
 * Returned Value:
 *  0 on success; a negated errno on failure.
 *
 ****************************************************************************/

int usbstrg_bindlun(FAR void *handle, FAR const char *drvrpath,
                    unsigned int lunno, off_t startsector, size_t nsectors,
                    boolean readonly)
{
  FAR struct usbstrg_alloc_s *alloc = (FAR struct usbstrg_alloc_s *)handle;
  FAR struct usbstrg_dev_s *priv;
  FAR struct usbstrg_lun_s *lun;
  FAR struct inode *inode;
  struct geometry geo;
  int ret;

#ifdef CONFIG_DEBUG
  if (!alloc || !drvrpath || startsector < 0 || nsectors < 0)
    {
      usbtrace(TRACE_CLSERROR(USBSTRG_TRACEERR_BINLUNINVALIDARGS1), 0);
      return -EINVAL;
    }
#endif

  priv = &alloc->dev;

#ifdef CONFIG_DEBUG
  if (!priv->luntab)
    {
      usbtrace(TRACE_CLSERROR(USBSTRG_TRACEERR_INTERNALCONFUSION1), 0);
      return -EIO;
    }

  if (lunno > priv->nluns)
    {
      usbtrace(TRACE_CLSERROR(USBSTRG_TRACEERR_BINDLUNINVALIDARGS2), 0);
      return -EINVAL;
    }
#endif

  lun = &priv->luntab[lunno];

#ifdef CONFIG_DEBUG
  if (lun->inode != NULL)
    {
      usbtrace(TRACE_CLSERROR(USBSTRG_TRACEERR_LUNALREADYBOUND), 0);
      return -EBUSY;
    }
#endif

  /* Open the block driver */

  ret = open_blockdriver(drvrpath, 0, &inode);
  if (ret < 0)
    {
      usbtrace(TRACE_CLSERROR(USBSTRG_TRACEERR_BLKDRVEOPEN), 0);
      return ret;
    }

  /* Get the drive geometry */

  if (!inode || !inode->u.i_bops || !inode->u.i_bops->geometry ||
      inode->u.i_bops->geometry(inode, &geo) != OK || !geo.geo_available)
    {
      usbtrace(TRACE_CLSERROR(USBSTRG_TRACEERR_NOGEOMETRY), 0);
      return -ENODEV;
    }

  /* Verify that the partition parameters are valid */

  if (startsector >= geo.geo_nsectors)
    {
      usbtrace(TRACE_CLSERROR(USBSTRG_TRACEERR_BINDLUNINVALIDARGS3), 0);
      return -EDOM;
    }
  else if (nsectors == 0)
    {
      nsectors = geo.geo_nsectors - startsector;
    }
  else if (startsector + nsectors >= geo.geo_nsectors)
    {
      usbtrace(TRACE_CLSERROR(USBSTRG_TRACEERR_BINDLUNINVALIDARGS4), 0);
      return -EDOM;
    }

  /* Initialize the LUN structure */

  memset(lun, 0, sizeof(struct usbstrg_lun_s *));

  /* Allocate an I/O buffer big enough to hold one hardware sector.  SCSI commands
   * are processed one at a time so all LUNs may share a single I/O buffer.  The
   * I/O buffer will be allocated so that is it as large as the largest block
   * device sector size
   */

  if (!priv->iobuffer)
    {
      priv->iobuffer = (ubyte*)malloc(geo.geo_sectorsize);
      if (!priv->iobuffer)
        {
          usbtrace(TRACE_CLSERROR(USBSTRG_TRACEERR_ALLOCIOBUFFER), geo.geo_sectorsize);
          return -ENOMEM;
        }
      priv->iosize = geo.geo_sectorsize;
    }
  else if (priv->iosize < geo.geo_sectorsize)
    {
      void *tmp;
      tmp = (ubyte*)realloc(priv->iobuffer, geo.geo_sectorsize);
      if (!tmp)
        {
          usbtrace(TRACE_CLSERROR(USBSTRG_TRACEERR_REALLOCIOBUFFER), geo.geo_sectorsize);
          return -ENOMEM;
        }

      priv->iobuffer = (ubyte*)tmp;
      priv->iosize   = geo.geo_sectorsize;
    }

  lun->inode       = inode;
  lun->startsector = startsector;
  lun->nsectors    = nsectors;
  lun->sectorsize  = geo.geo_sectorsize;

  /* If the driver does not support the write method, then this is read-only */

  if (!inode->u.i_bops->write)
    {
      lun->readonly = TRUE;
    }
  return OK;
}

/****************************************************************************
 * Name: usbstrg_unbindlun
 *
 * Description:
 *   Un-bind the block driver for the specified LUN
 *
 * Input Parameters:
 *   handle - The handle returned by a previous call to usbstrg_configure().
 *   lun    - the LUN to unbind from
 *
 * Returned Value:
 *  0 on success; a negated errno on failure.
 *
 ****************************************************************************/

int usbstrg_unbindlun(FAR void *handle, unsigned int lunno)
{
  FAR struct usbstrg_alloc_s *alloc = (FAR struct usbstrg_alloc_s *)handle;
  FAR struct usbstrg_dev_s *priv;
  FAR struct usbstrg_lun_s *lun;
  int ret;

#ifdef CONFIG_DEBUG
  if (!alloc)
    {
      usbtrace(TRACE_CLSERROR(USBSTRG_TRACEERR_UNBINDLUNINVALIDARGS1), 0);
      return -EINVAL;
    }
#endif

  priv = &alloc->dev;

#ifdef CONFIG_DEBUG
  if (!priv->luntab)
    {
      usbtrace(TRACE_CLSERROR(USBSTRG_TRACEERR_INTERNALCONFUSION2), 0);
      return -EIO;
    }

  if (lunno > priv->nluns)
    {
      usbtrace(TRACE_CLSERROR(USBSTRG_TRACEERR_UNBINDLUNINVALIDARGS2), 0);
      return -EINVAL;
    }
#endif

  lun = &priv->luntab[lunno];
  pthread_mutex_lock(&priv->mutex);

#ifdef CONFIG_DEBUG
  if (lun->inode == NULL)
    {
      usbtrace(TRACE_CLSERROR(USBSTRG_TRACEERR_LUNNOTBOUND), 0);
      pthread_mutex_lock(&priv->mutex);
      ret = -EBUSY;
    }
  else
#endif
   {
      /* Close the block driver */

     usbstrg_lununinitialize(lun);
     ret = OK;
   }

  pthread_mutex_unlock(&priv->mutex);
  return ret;
}

/****************************************************************************
 * Name: usbstrg_exportluns
 *
 * Description:
 *   After all of the LUNs have been bound, this function may be called
 *   in order to export those LUNs in the USB storage device.
 *
 * Input Parameters:
 *   handle - The handle returned by a previous call to usbstrg_configure().
 *
 * Returned Value:
 *   0 on success; a negated errno on failure
 *
 ****************************************************************************/

int usbstrg_exportluns(FAR void *handle)
{
  FAR struct usbstrg_alloc_s *alloc = (FAR struct usbstrg_alloc_s *)handle;
  FAR struct usbstrg_dev_s *priv;
  FAR struct usbstrg_driver_s *drvr;
  irqstate_t flags;
#ifdef SDCC
  pthread_attr_t attr;
#endif
  int ret;

#ifdef CONFIG_DEBUG
  if (!alloc)
    {
      usbtrace(TRACE_CLSERROR(USBSTRG_TRACEERR_EXPORTLUNSINVALIDARGS), 0);
      return -ENXIO;
    }
#endif

  priv = &alloc->dev;
  drvr = &alloc->drvr;

  /* Start the worker thread */

  pthread_mutex_lock(&priv->mutex);
  priv->thstate    = USBSTRG_STATE_NOTSTARTED;
  priv->theventset = USBSTRG_EVENT_NOEVENTS;

#ifdef SDCC
  (void)pthread_attr_init(&attr);
  ret = pthread_create(&priv->thread, &attr, usbstrg_workerthread, (pthread_addr_t)priv);
#else
  ret = pthread_create(&priv->thread, NULL, usbstrg_workerthread, (pthread_addr_t)priv);
#endif
  if (ret != OK)
    {
      usbtrace(TRACE_CLSERROR(USBSTRG_TRACEERR_THREADCREATE), (uint16)-ret);
      goto errout_with_mutex;
    }

  /* Register the USB storage class driver */

  ret = usbdev_register(&drvr->drvr);
  if (ret != OK)
    {
      usbtrace(TRACE_CLSERROR(USBSTRG_TRACEERR_DEVREGISTER), (uint16)-ret);
      goto errout_with_mutex;
    }

  /* Signal to start the thread */

  flags = irqsave();
  priv->theventset |= USBSTRG_EVENT_READY;
  pthread_cond_signal(&priv->cond);
  irqrestore(flags);

errout_with_mutex:
  pthread_mutex_unlock(&priv->mutex);
  return ret;
}

/****************************************************************************
 * Name: usbstrg_uninitialize
 *
 * Description:
 *   Un-initialize the USB storage class driver
 *
 * Input Parameters:
 *   handle - The handle returned by a previous call to usbstrg_configure().
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

void usbstrg_uninitialize(FAR void *handle)
{
  FAR struct usbstrg_alloc_s *alloc = (FAR struct usbstrg_alloc_s *)handle;
  FAR struct usbstrg_dev_s *priv;
  irqstate_t flags;
#ifdef SDCC
  pthread_addr_t result1, result2;
  pthread_attr_t attr;
#endif
  void *value;
  int ret;
  int i;

#ifdef CONFIG_DEBUG
  if (!handle)
    {
      usbtrace(TRACE_CLSERROR(USBSTRG_TRACEERR_UNINITIALIZEINVALIDARGS), 0);
      return;
    }
#endif
  priv = &alloc->dev;

  /* If the thread hasn't already exitted, tell it to exit now */

  if (priv->thstate != USBSTRG_STATE_NOTSTARTED)
    {
       /* The thread was started.. Is it still running? */

      pthread_mutex_lock(&priv->mutex);
      if (priv->thstate != USBSTRG_STATE_TERMINATED)
        {
          /* Yes.. Ask the thread to stop */

          flags = irqsave();
          priv->theventset |= USBSTRG_EVENT_TERMINATEREQUEST;
          pthread_cond_signal(&priv->cond);
          irqrestore(flags);
        }
      pthread_mutex_unlock(&priv->mutex);

      /* Wait for the thread to exit.  This is necessary even if the
       * thread has already exitted in order to collect the join
       * garbage
       */

      ret = pthread_join(priv->thread, &value);
    }
  priv->thread = 0;

  /* Unregister the driver */

  usbdev_unregister(&alloc->drvr.drvr);

  /* Uninitialize and release the LUNs */

  for (i = 0; i < priv->nluns; ++i)
    {
      usbstrg_lununinitialize(&priv->luntab[i]);
    }
  free(priv->luntab);

  /* Release the I/O buffer */

  if (priv->iobuffer)
    {
      free(priv->iobuffer);
    }

  /* Uninitialize and release the driver structure */

  pthread_mutex_destroy(&priv->mutex);
  pthread_cond_destroy(&priv->cond);

  free(priv);
}
