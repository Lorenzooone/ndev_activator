#include "usb_generic.hpp"
#include "ndev_libusb_communications.hpp"
#include <cstdio>

static int libusb_ctrl_transfer(void* handle, const ndev_usb_data* usb_dev_data, uint8_t bmRequestType, uint8_t bRequest, uint16_t wValue, uint16_t wIndex, unsigned char* data, uint16_t wLength) {
	return libusb_control_transfer((libusb_device_handle*)handle, bmRequestType, bRequest, wValue, wIndex, data, wLength, usb_dev_data->timeout_ms);
}

int libusb_ctrl_transfer_out(void* handle, const ndev_usb_data* usb_dev_data, uint8_t bRequest, uint16_t wValue, uint16_t wIndex, const unsigned char* data, uint16_t wLength) {
	return libusb_ctrl_transfer(handle, usb_dev_data, 0x40, bRequest, wValue, wIndex, (unsigned char*)data, wLength);
}

static int libusb_ctrl_transfer_out_first_setup(void* handle, const ndev_usb_data* usb_dev_data, size_t iteration) {
	size_t remaining_size = usb_dev_data->payload_size - (iteration * usb_dev_data->packet_size);
	if(remaining_size > usb_dev_data->packet_size)
		remaining_size = usb_dev_data->packet_size;
	return libusb_ctrl_transfer_out(handle, usb_dev_data, 160, usb_dev_data->packet_size * iteration, 0, &usb_dev_data->payload_device[iteration * usb_dev_data->packet_size], remaining_size);
}

static int libusb_ctrl_transfer_out_second_setup(void* handle, const ndev_usb_data* usb_dev_data, size_t iteration) {
	size_t remaining_size = usb_dev_data->payload_size - (iteration * usb_dev_data->packet_size);
	if(remaining_size > usb_dev_data->packet_size)
		remaining_size = usb_dev_data->packet_size;
	return libusb_ctrl_transfer_out(handle, usb_dev_data, 5, 3, 0, &usb_dev_data->payload_device[iteration * usb_dev_data->packet_size], remaining_size);
}

int libusb_ctrl_transfer_in(void* handle, const ndev_usb_data* usb_dev_data, uint8_t bRequest, uint16_t wValue, uint16_t wIndex, unsigned char* data, uint16_t wLength) {
	return libusb_ctrl_transfer(handle, usb_dev_data, 0xC0, bRequest, wValue, wIndex, data, wLength);
}

static int libusb_ctrl_transfer_in_start_second_setup(void* handle, const ndev_usb_data* usb_dev_data) {
	uint8_t data = 0;
	int ret = libusb_ctrl_transfer_in(handle, usb_dev_data, 5, 1, 0, &data, 1);
	if(ret < 0)
		return ret;
	if(data != 1)
		return -1;
	return ret;
}

static int libusb_ctrl_transfer_in_complete_second_setup(void* handle, const ndev_usb_data* usb_dev_data) {
	uint8_t data = 0;
	int ret = libusb_ctrl_transfer_in(handle, usb_dev_data, 5, 2, 0, &data, 1);
	if(ret < 0)
		return ret;
	if(data != 1)
		return -1;
	return ret;
}

static int libusb_ctrl_transfer_in_has_done_second_setup(void* handle, const ndev_usb_data* usb_dev_data, bool* done) {
	uint8_t data = 0;
	*done = false;
	int ret = libusb_ctrl_transfer_in(handle, usb_dev_data, 5, 4, 0, &data, 1);
	if(ret < 0)
		return ret;
	if(data != 0)
		*done = true;
	return ret;
}

static int libusb_initial_ndev_setup(void* handle, const ndev_usb_data* usb_dev_data) {
	uint8_t transfer_happening = 1;
	int result = libusb_ctrl_transfer_out(handle, usb_dev_data, 160, 0xE600, 0, &transfer_happening, 1);
	if(result < 0)
		return result;
	for(size_t i = 0; i < ((usb_dev_data->payload_size + usb_dev_data->packet_size - 1) / usb_dev_data->packet_size); i++) {
		result = libusb_ctrl_transfer_out_first_setup(handle, usb_dev_data, i);
		if(result < 0)
			return result;
	}
	transfer_happening = 0;
	return libusb_ctrl_transfer_out(handle, usb_dev_data, 160, 0xE600, 0, &transfer_happening, 1);
}

static int libusb_second_ndev_setup(void* handle, const ndev_usb_data* usb_dev_data) {
	bool done_setup = false;
	int result = libusb_ctrl_transfer_in_has_done_second_setup(handle, usb_dev_data, &done_setup);
	if(result < 0)
		return result;
	if(done_setup)
		return result;
	result = libusb_ctrl_transfer_in_start_second_setup(handle, usb_dev_data);
	if(result < 0)
		return result;
	for(size_t i = 0; i < ((usb_dev_data->payload_size + usb_dev_data->packet_size - 1) / usb_dev_data->packet_size); i++) {
		result = libusb_ctrl_transfer_out_second_setup(handle, usb_dev_data, i);
		if(result < 0)
			return result;
	}
	return libusb_ctrl_transfer_in_complete_second_setup(handle, usb_dev_data);
}

int libusb_get_ndev_serial(void* handle, const ndev_usb_data* usb_dev_data, uint8_t* serial_buffer, size_t length) {
	if(length > 16)
		length = 16;
	return libusb_ctrl_transfer_in(handle, usb_dev_data, 2, 0x01A2, 0x1FF0, serial_buffer, length);
}

bool libusb_prepare_ndev_devices(void) {
	if(!usb_is_initialized())
		return false;
	libusb_device **usb_devices;
	int num_devices = libusb_get_device_list(get_usb_ctx(), &usb_devices);
	libusb_device_descriptor usb_descriptor{};
	bool re_run = false;

	for(int i = 0; i < num_devices; i++) {
		int result = libusb_get_device_descriptor(usb_devices[i], &usb_descriptor);
		if(result < 0)
			continue;
		for(int j = 0; j < get_number_ndev_setup_data(); j++) {
			const ndev_usb_data* possible_usb_data = get_ndev_setup_data(j);
			if((possible_usb_data->vid != usb_descriptor.idVendor) || (possible_usb_data->pid != usb_descriptor.idProduct) || (possible_usb_data->bcd_device != usb_descriptor.bcdDevice))
				continue;
			libusb_device_handle *handle = NULL;
			result = libusb_open(usb_devices[i], &handle);
			if(result || (handle == NULL))
				break;
			result = libusb_claim_interface(handle, possible_usb_data->interface_to_claim);
			if(result < 0) {
				libusb_close(handle);
				break;
			}
			if(possible_usb_data->payload_device != NULL) {
				if(possible_usb_data->is_initial_setup)
					libusb_initial_ndev_setup((void*)handle, possible_usb_data);
				else 
					libusb_second_ndev_setup((void*)handle, possible_usb_data);
			}
			if(possible_usb_data->re_check)
				re_run = true;
			libusb_release_interface(handle, possible_usb_data->interface_to_claim);
			libusb_close(handle);
			break;
		}
	}

	if(num_devices >= 0)
		libusb_free_device_list(usb_devices, 1);
	return re_run;
}

void setup_libusb(void) {
	usb_init();
}

void stop_libusb(void) {
	usb_close();
}
