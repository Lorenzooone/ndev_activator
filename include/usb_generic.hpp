#ifndef __USB_GENERIC_HPP
#define __USB_GENERIC_HPP

#ifndef PREVENT_USING_LIBUSB
#include <libusb.h>
libusb_context* get_usb_ctx();
#endif

void usb_init();
void usb_close();
bool usb_is_initialized();

#endif
