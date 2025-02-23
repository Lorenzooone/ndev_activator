#include <iostream>
#include <chrono>
#include <thread>

#include "ndev_communications.hpp"
#include "ndev_libusb_communications.hpp"

#include "usb_setup_data_0301_0100.h"
#include "usb_setup_data_0301_0200.h"
#include "usb_setup_data_0302_0100.h"
#include "payload_0301_0201.h"
#include "payload_0302_0101.h"

#define USB_TIMEOUT_MS 500

const ndev_usb_data usb_di_setup_data = {
.vid = 0x057E, .pid = 0x0302, .bcd_device = 0x0100,
.payload_device = usb_setup_data_0302_0100, .payload_size = usb_setup_data_0302_0100_len,
.packet_size = 2048,
.is_initial_setup = true, .re_check = true,
.timeout_ms = USB_TIMEOUT_MS, .interface_to_claim = 0
};

const ndev_usb_data usb_com_setup_data = {
.vid = 0x057E, .pid = 0x0301, .bcd_device = 0x200,
.payload_device = usb_setup_data_0301_0200, .payload_size = usb_setup_data_0301_0200_len,
.packet_size = 2048,
.is_initial_setup = true, .re_check = true,
.timeout_ms = USB_TIMEOUT_MS, .interface_to_claim = 0
};

const ndev_usb_data usb_debug_setup_data = {
.vid = 0x057E, .pid = 0x0301, .bcd_device = 0x0100,
.payload_device = usb_setup_data_0301_0100, .payload_size = usb_setup_data_0301_0100_len,
.packet_size = 2048,
.is_initial_setup = true, .re_check = true,
.timeout_ms = USB_TIMEOUT_MS, .interface_to_claim = 0
};

const ndev_usb_data usb_di_post_setup_data = {
.vid = 0x057E, .pid = 0x0302, .bcd_device = 0x0101,
.payload_device = payload_0302_0101, .payload_size = payload_0302_0101_len,
.packet_size = 64,
.is_initial_setup = false, .re_check = false,
.timeout_ms = USB_TIMEOUT_MS, .interface_to_claim = 0
};

const ndev_usb_data usb_com_post_setup_data = {
.vid = 0x057E, .pid = 0x0301, .bcd_device = 0x201,
.payload_device = payload_0301_0201, .payload_size = payload_0301_0201_len,
.packet_size = 64,
.is_initial_setup = false, .re_check = false,
.timeout_ms = USB_TIMEOUT_MS, .interface_to_claim = 0
};

const ndev_usb_data usb_debug_post_setup_data = {
.vid = 0x057E, .pid = 0x0301, .bcd_device = 0x101,
.payload_device = NULL, .payload_size = 0,
.packet_size = 64,
.is_initial_setup = false, .re_check = false,
.timeout_ms = USB_TIMEOUT_MS, .interface_to_claim = 0
};

const ndev_usb_data* ndev_data_ptrs[] = {
	&usb_di_setup_data,
	&usb_com_setup_data,
	&usb_debug_setup_data,
	&usb_di_post_setup_data,
	&usb_com_post_setup_data,
	&usb_debug_post_setup_data,
};

const ndev_usb_data* ndev_disk_interface = &usb_di_post_setup_data;

int get_number_ndev_setup_data(void) {
	return sizeof(ndev_data_ptrs) / sizeof(ndev_data_ptrs[0]);
}

const ndev_usb_data* get_ndev_setup_data(int index) {
	if((index < 0) || (index >= get_number_ndev_setup_data()))
		return NULL;
	return ndev_data_ptrs[index];
}

void prepare_ndev_devices(void) {
	bool run_prepare = true;
	while(run_prepare) {
		run_prepare = libusb_prepare_ndev_devices();
		// Wait for reconnections...
		if(run_prepare)
			std::this_thread::sleep_for(std::chrono::seconds(5));
	}
}

void prepare_communications(void) {
	setup_libusb();
}

void close_communications(void) {
	stop_libusb();
}
