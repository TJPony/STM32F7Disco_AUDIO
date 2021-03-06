#ifndef __USB_CONF__H__
#define __USB_CONF__H__

#include "sys.h"

#ifndef USE_USB_OTG_FS
/* #define USE_USB_OTG_FS */
#endif /* USE_USB_OTG_FS */

#ifdef USE_USB_OTG_FS 
 #define USB_OTG_FS_CORE
#endif

#ifndef USE_USB_OTG_HS
/* #define USE_USB_OTG_HS */
#endif /* USE_USB_OTG_HS */

#ifndef USE_ULPI_PHY
 #define USE_ULPI_PHY */
#endif /* USE_ULPI_PHY */

#ifndef USE_EMBEDDED_PHY
/* #define USE_EMBEDDED_PHY */
#endif /* USE_EMBEDDED_PHY */

#ifdef USE_USB_OTG_HS 
 #define USB_OTG_HS_CORE
#endif

#ifdef USB_OTG_HS_CORE
 #define RX_FIFO_HS_SIZE                          512
 #define TX0_FIFO_HS_SIZE                         128
 #define TX1_FIFO_HS_SIZE                         272
 #define TX2_FIFO_HS_SIZE                          0
 #define TX3_FIFO_HS_SIZE                          0
 #define TX4_FIFO_HS_SIZE                          0
 #define TX5_FIFO_HS_SIZE                          0

 #ifdef USE_ULPI_PHY
  #define USB_OTG_ULPI_PHY_ENABLED
 #endif
 #ifdef USE_EMBEDDED_PHY
   #define USB_OTG_EMBEDDED_PHY_ENABLED
 #endif
 #define USB_OTG_HS_INTERNAL_DMA_ENABLED
 //#define USB_OTG_HS_DEDICATED_EP1_ENABLED
#endif

/****************** USB OTG FS CONFIGURATION **********************************/
#ifdef USB_OTG_FS_CORE
 #define RX_FIFO_FS_SIZE                          128
 #define TX0_FIFO_FS_SIZE                          64
 #define TX1_FIFO_FS_SIZE                          64
 #define TX2_FIFO_FS_SIZE                          64
 #define TX3_FIFO_FS_SIZE                          0

/* #define USB_OTG_FS_LOW_PWR_MGMT_SUPPORT */
/* #define USB_OTG_FS_SOF_OUTPUT_ENABLED */
#endif

/****************** USB OTG MISC CONFIGURATION ********************************/
//#define VBUS_SENSING_ENABLED

/****************** USB OTG MODE CONFIGURATION ********************************/
/* #define USE_HOST_MODE */
#define USE_DEVICE_MODE
/* #define USE_OTG_MODE */

#ifndef USB_OTG_FS_CORE
 #ifndef USB_OTG_HS_CORE
    #error  "USB_OTG_HS_CORE or USB_OTG_FS_CORE should be defined"
 #endif
#endif

#ifndef USE_DEVICE_MODE
 #ifndef USE_HOST_MODE
    #error  "USE_DEVICE_MODE or USE_HOST_MODE should be defined"
 #endif
#endif

#ifndef USE_USB_OTG_HS
 #ifndef USE_USB_OTG_FS
    #error  "USE_USB_OTG_HS or USE_USB_OTG_FS should be defined"
 #endif
#else /* USE_USB_OTG_HS */
 #ifndef USE_ULPI_PHY
  #ifndef USE_EMBEDDED_PHY
     #error  "USE_ULPI_PHY or USE_EMBEDDED_PHY should be defined"
  #endif
 #endif
#endif
#endif
