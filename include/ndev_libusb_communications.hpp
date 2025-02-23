#ifndef __NDEV_LIBUSB_COMMUNICATIONS_HPP
#define __NDEV_LIBUSB_COMMUNICATIONS_HPP

#include "ndev_communications.hpp"

int libusb_ctrl_transfer_out(void* handle, const ndev_usb_data* usb_dev_data, uint8_t bRequest, uint16_t wValue, uint16_t wIndex, const unsigned char* data, uint16_t wLength);
int libusb_ctrl_transfer_in(void* handle, const ndev_usb_data* usb_dev_data, uint8_t bRequest, uint16_t wValue, uint16_t wIndex, unsigned char* data, uint16_t wLength);
int libusb_get_ndev_serial(void* handle, const ndev_usb_data* usb_dev_data, uint8_t* serial_buffer, size_t length);
bool libusb_prepare_ndev_devices(void);
void setup_libusb(void);
void stop_libusb(void);

#endif
