#ifndef __USB_MSC_HID_CORE_H_
#define __USB_MSC_HID_CORE_H_

#include "usbd_msc_core.h"
#include "usbd_customhid_core.h"
#include "usbd_desc.h"
#include "usbd_req.h"
#include "usbd_ioreq.h"

#define HID_INTERFACE 0x0
#define MSC_INTERFACE 0x1

#define USB_MSC_HID_CONFIG_DESC_SIZ  (USB_CUSTOM_HID_CONFIG_DESC_SIZ - 9 + USB_MSC_CONFIG_DESC_SIZ)

extern USBD_Class_cb_TypeDef  USBD_MSC_HID_cb;

#endif
