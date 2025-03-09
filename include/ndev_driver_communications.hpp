#ifndef __NDEV_DRIVER_COMMUNICATIONS_HPP
#define __NDEV_DRIVER_COMMUNICATIONS_HPP

#include "ndev_communications.hpp"

int ndev_driver_ctrl_transfer_in(void* handle, uint8_t bRequest, uint16_t wValue, uint16_t wIndex, unsigned char* data, uint16_t wLength);
int ndev_driver_bulk_transfer_in(void* handle, const ndev_usb_data* usb_device_desc, unsigned char* data, size_t length, int* num_transferred);
int ndev_driver_bulk_transfer_out(void* handle, const ndev_usb_data* usb_device_desc, const unsigned char* data, size_t length, int* num_transferred);
int ndev_driver_fifo_transfer_out(void* handle, const unsigned char* data, size_t length, int* num_transferred);

void driver_connect_to_ndev_device(uint8_t* expected_data, ndev_device_handle* out_handle);
void driver_disconnect_ndev_device(ndev_device_handle* handlers);

#endif
