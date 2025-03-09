#include <iostream>
#include <chrono>
#include <thread>
#include <cstring>

#include "ndev_communications.hpp"
#include "ndev_libusb_communications.hpp"
#include "ndev_driver_communications.hpp"
#include "utils.hpp"

#include "usb_setup_data_0301_0100.h"
#include "usb_setup_data_0301_0200.h"
#include "usb_setup_data_0302_0100.h"
#include "payload_0301_0201.h"
#include "payload_0302_0101.h"
#include "no_disk_inserted.h"

#define USB_TIMEOUT_MS 500

#define NDEV_DATA_EPILOGUE_VALUE 0xBEEF

#pragma pack(push, 1)

struct PACKED ALIGNED(1) command_ndev_data_prologue {
	uint16_t unk_base : 15;
	uint16_t is_data_out : 1;
	uint16_t command;
	uint16_t unk_2;
	uint16_t num_hwords;
};

struct PACKED ALIGNED(1) command_ndev_data_epilogue {
	uint16_t unk_3;
};

typedef union {
	struct PACKED ALIGNED(2) {
		uint16_t unk : 3;
		uint16_t is_disk_inserted : 1;
		uint16_t unk2 : 3;
		uint16_t is_request_ready : 1;
		uint16_t unk3 : 8;
	} fields;
	uint16_t bits;
} ndev_data_disk_status;

typedef union {
	struct PACKED ALIGNED(2) {
		uint16_t unk : 2;
		// Unknown... Power off?
		uint16_t unk2 : 1;
		uint16_t unk3 : 3;
		// Unknown... Crash?
		uint16_t unk4 : 1;
		uint16_t disk_request_acknowledged : 1;
		uint16_t unk5 : 8;
	} fields;
	uint16_t bits;
} ndev_data_status;

struct PACKED ALIGNED(2) ndev_data_ping_status {
	uint16_t unk[2];
	uint32_t disk_request_position_be;
	uint32_t disk_request_length_be;
	ndev_data_status status;
	ndev_data_disk_status disk_status;
	uint16_t unk_byte_2;
	uint16_t unk_byte_3;
	uint16_t unk2[6];
};

#pragma pack(pop)

enum ndev_di_commands {
	NDEV_DI_GET_STATUS_PING = 0,
	NDEV_DI_GET_STATUS_BITFIELD = 0x0C,
	NDEV_DI_SET_STATUS_BITFIELD = 0x0C,
	NDEV_DI_GET_DISK_STATUS = 0x0E,
	NDEV_DI_SET_DISK_STATUS = 0x0E,
	NDEV_DI_GET_UNK_BYTE_2 = 0x10,
	NDEV_DI_SET_UNK_BYTE_2 = 0x10,
	NDEV_DI_GET_UNK_BYTE_3 = 0x12,
	NDEV_DI_SET_UNK_BYTE_3 = 0x12,
	NDEV_DI_RESET_USB = 0x16,
	NDEV_DI_GET_HARDWARE_REV = 0x1E,
};

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

static const ndev_usb_data* get_di_ndev_data(void) {
	return ndev_disk_interface;
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

static int ndev_device_bulk_transfer_out(ndev_device_handle* dev_handle, const uint8_t* data, size_t length, int* num_transferred) {
	if(!dev_handle->connected)
		return -1;
	switch (dev_handle->connection_type) {
		case LIBUSB_CONNECTED:
			return libusb_bulk_transfer_out(dev_handle->usb_handle, dev_handle->associated_device, data, length, num_transferred);
		case ORIGINAL_DRIVER_CONNECTED:
			return ndev_driver_bulk_transfer_out(dev_handle->file_handle, dev_handle->associated_device, data, length, num_transferred);
		default:
			return -1;
	}
}

static int ndev_device_fifo_transfer_out(ndev_device_handle* dev_handle, const uint8_t* data, size_t length, int* num_transferred) {
	if(!dev_handle->connected)
		return -1;
	switch (dev_handle->connection_type) {
		case LIBUSB_CONNECTED:
			return libusb_fifo_transfer_out(dev_handle->usb_handle, dev_handle->associated_device, data, length, num_transferred);
		case ORIGINAL_DRIVER_CONNECTED:
			return ndev_driver_fifo_transfer_out(dev_handle->file_handle, data, length, num_transferred);
		default:
			return -1;
	}
}

static int ndev_device_bulk_transfer_in(ndev_device_handle* dev_handle, uint8_t* data, size_t length, int* num_transferred) {
	if (!dev_handle->connected)
		return -1;
	switch (dev_handle->connection_type) {
		case LIBUSB_CONNECTED:
			return libusb_bulk_transfer_in(dev_handle->usb_handle, dev_handle->associated_device, data, length, num_transferred);
		case ORIGINAL_DRIVER_CONNECTED:
			return ndev_driver_bulk_transfer_in(dev_handle->file_handle, dev_handle->associated_device, data, length, num_transferred);
		default:
			return -1;
	}
}

static int ndev_device_ctrl_transfer_in(ndev_device_handle* dev_handle, uint8_t bRequest, uint16_t wValue, uint16_t wIndex, uint8_t* data, uint16_t wLength) {
	if (!dev_handle->connected)
		return -1;
	switch (dev_handle->connection_type) {
		case LIBUSB_CONNECTED:
			return libusb_ctrl_transfer_in(dev_handle->usb_handle, dev_handle->associated_device, bRequest, wValue, wIndex, data, wLength);
		case ORIGINAL_DRIVER_CONNECTED:
			return ndev_driver_ctrl_transfer_in(dev_handle->file_handle, bRequest, wValue, wIndex, data, wLength);
		default:
			return -1;
	}
}

int get_ndev_device_serial(ndev_device_handle* dev_handle) {
	return ndev_device_ctrl_transfer_in(dev_handle, 2, 0x01A2, 0x1FF0, dev_handle->serial, NDEV_SERIAL_DATA_SIZE);
}

int get_ndev_device_has_done_second_setup(ndev_device_handle* dev_handle, bool* done) {
	uint8_t data = 0;
	*done = false;
	int ret = ndev_device_ctrl_transfer_in(dev_handle, 5, 4, 0, &data, 1);
	if (ret < 0)
		return ret;
	if (data != 0)
		*done = true;
	return ret;
}

int ndev_device_wait_transfer_ready(ndev_device_handle* dev_handle) {
	uint8_t data = 0;
	int ret = ndev_device_ctrl_transfer_in(dev_handle, 5, 5, 0, &data, 1);
	if (ret < 0)
		return ret;
	if (data != 1)
		return -1;
	return ret;
}

static void command_data_filler(uint8_t* data, bool is_out, uint16_t command, uint16_t num_elems, size_t data_size) {
	memset(data, 0, data_size);
	uint16_t prologue_base_data = 0;
	if(is_out)
		prologue_base_data |= 0x8000;
	write_le16(data, 0, prologue_base_data);
	write_le16(data, 2, command);
	write_le16(data, 4, 0);
	write_le16(data, 6, num_elems);
	write_le16(data, data_size - sizeof(command_ndev_data_epilogue), NDEV_DATA_EPILOGUE_VALUE);
}

static uint8_t* command_out_generator(const uint16_t* data, uint16_t command, uint16_t num_elems, size_t* data_out_size) {
	*data_out_size = sizeof(command_ndev_data_prologue) + (num_elems * 2) + sizeof(command_ndev_data_epilogue);
	uint8_t* data_out = new uint8_t[*data_out_size];
	command_data_filler(data_out, true, command, num_elems, *data_out_size);
	for(int i = 0; i < num_elems; i++)
		write_le16(data_out, sizeof(command_ndev_data_prologue) + (i * 2), data[i]);
	return data_out;
}

static uint8_t* command_in_generator(uint16_t command, uint16_t num_elems, size_t* data_out_size) {
	*data_out_size = sizeof(command_ndev_data_prologue) + sizeof(command_ndev_data_epilogue);
	uint8_t* data_out = new uint8_t[*data_out_size];
	command_data_filler(data_out, false, command, num_elems, *data_out_size);
	return data_out;
}

static uint8_t* command_in_buffer_generator(uint16_t command, uint16_t num_elems, size_t* data_out_size) {
	*data_out_size = sizeof(command_ndev_data_prologue) + (num_elems * 2) + sizeof(command_ndev_data_epilogue);
	uint8_t* data_out = new uint8_t[*data_out_size];
	return data_out;
}

static bool command_in_buffer_reader(uint16_t* data_out, const uint8_t* command_in, size_t command_in_size, const uint8_t* command_in_buffer, size_t command_in_buffer_size) {
	for(int i = 0; i < sizeof(command_ndev_data_prologue); i++)
		if(command_in[i] != command_in_buffer[i])
			return false;
	for(int i = 0; i < sizeof(command_ndev_data_epilogue); i++)
		if(command_in[i + command_in_size - sizeof(command_ndev_data_epilogue)] != command_in_buffer[i + command_in_buffer_size - sizeof(command_ndev_data_epilogue)])
			return false;
	uint16_t num_elems = read_le16(command_in_buffer, 6);
	for(int i = 0; i < num_elems; i++)
		data_out[i] = read_le16(command_in_buffer, sizeof(command_ndev_data_prologue) + (i * 2));
	return true;
}

static void command_deallocator(uint8_t* data) {
	delete []data;
}

static int ndev_device_send_command_out(ndev_device_handle* dev_handle, const uint16_t* data, uint16_t command, uint16_t num_elems) {
	size_t data_out_size = 0;
	int num_transferred = 0;
	uint8_t* data_out = command_out_generator(data, command, num_elems, &data_out_size);
	int result = ndev_device_bulk_transfer_out(dev_handle, data_out, data_out_size, &num_transferred);
	command_deallocator(data_out);
	if(result < 0)
		return result;
	if(num_transferred < data_out_size)
		return -1;
	return result;
}

static int ndev_device_send_command_out_u16(ndev_device_handle* dev_handle, uint16_t value, uint16_t command) {
	return ndev_device_send_command_out(dev_handle, &value, command, 1);
}

static int internal_ndev_device_send_command_in(ndev_device_handle* dev_handle, uint16_t* data, const uint8_t* data_out, size_t data_out_size, uint8_t* data_in, size_t data_in_size) {
	int num_transferred = 0;
	int result = ndev_device_bulk_transfer_out(dev_handle, data_out, data_out_size, &num_transferred);
	if(result < 0)
		return result;
	if(num_transferred < data_out_size)
		return -1;
	result = ndev_device_bulk_transfer_in(dev_handle, data_in, data_in_size, &num_transferred);
	if(result < 0)
		return result;
	if(num_transferred < data_in_size)
		return -1;
	if(!command_in_buffer_reader(data, data_out, data_out_size, data_in, data_in_size))
		return -1;
	return result;
}

static int ndev_device_send_command_in(ndev_device_handle* dev_handle, uint16_t* data, uint16_t command, uint16_t num_elems) {
	size_t data_out_size = 0;
	size_t data_in_size = 0;
	uint8_t* data_out = command_in_generator(command, num_elems, &data_out_size);
	uint8_t* data_in = command_in_buffer_generator(command, num_elems, &data_in_size);
	int result = internal_ndev_device_send_command_in(dev_handle, data, data_out, data_out_size, data_in, data_in_size);
	command_deallocator(data_in);
	command_deallocator(data_out);
	return result;
}

static int ndev_device_send_command_in_u16(ndev_device_handle* dev_handle, uint16_t* value, uint16_t command) {
	return ndev_device_send_command_in(dev_handle, value, command, 1);
}

ndev_device_handle connect_to_disk_interface(uint8_t* expected_data) {
	ndev_device_handle out_ndev_di_handle;
	out_ndev_di_handle.usb_handle = NULL;
	out_ndev_di_handle.file_handle = NULL;
	out_ndev_di_handle.mutex_handle = NULL;
	out_ndev_di_handle.path = "";
	out_ndev_di_handle.connected = false;
	out_ndev_di_handle.connection_type = NO_CONNECTION_DONE;
	out_ndev_di_handle.associated_device = get_di_ndev_data();
	bool has_failed_driver_possible = false;
	libusb_connect_to_ndev_device(expected_data, &out_ndev_di_handle, &has_failed_driver_possible);
	if ((!out_ndev_di_handle.connected) && has_failed_driver_possible)
		driver_connect_to_ndev_device(expected_data, &out_ndev_di_handle);
	return out_ndev_di_handle;
}

static void disconnect_on_failure(ndev_device_handle* dev_handle) {
	dev_handle->connected = false;
}

static int setup_unk_bytes(ndev_device_handle* di_handle, bool do_first_unk3_send, bool do_final_unk3_send) {
	uint16_t unk_byte2 = 0;
	int result = ndev_device_send_command_in_u16(di_handle, &unk_byte2, NDEV_DI_GET_UNK_BYTE_2);
	if(result < 0)
		return result;
	uint16_t out_unk_byte2 = 0xC3;
	result = ndev_device_send_command_out_u16(di_handle, out_unk_byte2, NDEV_DI_SET_UNK_BYTE_2);
	if(result < 0)
		return result;
	out_unk_byte2 = 0xCF;
	result = ndev_device_send_command_out_u16(di_handle, out_unk_byte2, NDEV_DI_SET_UNK_BYTE_2);
	if(result < 0)
		return result;
	result = ndev_device_send_command_in_u16(di_handle, &unk_byte2, NDEV_DI_GET_UNK_BYTE_2);
	if(result < 0)
		return result;
	out_unk_byte2 = 0xCE;
	result = ndev_device_send_command_out_u16(di_handle, out_unk_byte2, NDEV_DI_SET_UNK_BYTE_2);
	if(result < 0)
		return result;
	uint16_t unk_byte3 = 0;
	result = ndev_device_send_command_in_u16(di_handle, &unk_byte3, NDEV_DI_GET_UNK_BYTE_3);
	if(result < 0)
		return result;
	uint16_t out_unk_byte3 = 0x85;
	if(do_first_unk3_send) {
		result = ndev_device_send_command_out_u16(di_handle, out_unk_byte3, NDEV_DI_SET_UNK_BYTE_3);
		if(result < 0)
			return result;
		result = ndev_device_send_command_in_u16(di_handle, &unk_byte3, NDEV_DI_GET_UNK_BYTE_3);
		if(result < 0)
			return result;
	}
	out_unk_byte3 = 0x81;
	result = ndev_device_send_command_out_u16(di_handle, out_unk_byte3, NDEV_DI_SET_UNK_BYTE_3);
	if(result < 0)
		return result;
	out_unk_byte3 = 0x85;
	result = ndev_device_send_command_out_u16(di_handle, out_unk_byte3, NDEV_DI_SET_UNK_BYTE_3);
	if(result < 0)
		return result;
	if(do_final_unk3_send) {
		result = ndev_device_send_command_in_u16(di_handle, &unk_byte3, NDEV_DI_GET_UNK_BYTE_3);
		if(result < 0)
			return result;
		out_unk_byte3 = 0x84;
		result = ndev_device_send_command_out_u16(di_handle, out_unk_byte3, NDEV_DI_SET_UNK_BYTE_3);
		if(result < 0)
			return result;
	}
	ndev_data_disk_status disk_status;
	disk_status.bits = 0;
	result = ndev_device_send_command_in_u16(di_handle, &disk_status.bits, NDEV_DI_GET_DISK_STATUS);
	if(result < 0)
		return result;
	disk_status.fields.is_request_ready = 0;
	disk_status.fields.is_disk_inserted = 0;
	return ndev_device_send_command_out_u16(di_handle, disk_status.bits, NDEV_DI_SET_DISK_STATUS);
}

void setup_disk_interface_active(ndev_device_handle* di_handle) {
	if(!di_handle->connected)
		return;
	int result = ndev_device_send_command_out_u16(di_handle, 0x000F, NDEV_DI_RESET_USB);
	if(result < 0)
		return disconnect_on_failure(di_handle);
	result = ndev_device_send_command_out_u16(di_handle, 0x000F, NDEV_DI_RESET_USB);
	if(result < 0)
		return disconnect_on_failure(di_handle);
	result = ndev_device_send_command_in_u16(di_handle, &di_handle->hw_rev, NDEV_DI_GET_HARDWARE_REV);
	if(result < 0)
		return disconnect_on_failure(di_handle);
	ndev_data_status status;
	status.bits = 0;
	result = ndev_device_send_command_in_u16(di_handle, &status.bits, NDEV_DI_GET_STATUS_BITFIELD);
	if(result < 0)
		return disconnect_on_failure(di_handle);
	status.bits = 0xFFFF;
	result = ndev_device_send_command_out_u16(di_handle, status.bits, NDEV_DI_SET_STATUS_BITFIELD);
	if(result < 0)
		return disconnect_on_failure(di_handle);
	ndev_data_disk_status disk_status;
	disk_status.bits = 0;
	result = ndev_device_send_command_in_u16(di_handle, &disk_status.bits, NDEV_DI_GET_DISK_STATUS);
	if(result < 0)
		return disconnect_on_failure(di_handle);
	disk_status.fields.is_request_ready = 0;
	result = ndev_device_send_command_out_u16(di_handle, disk_status.bits, NDEV_DI_SET_DISK_STATUS);
	if(result < 0)
		return disconnect_on_failure(di_handle);
	result = setup_unk_bytes(di_handle, true, false);
	if(result < 0)
		return disconnect_on_failure(di_handle);
}

void iter_keep_disk_interface_active(ndev_device_handle* di_handle) {
	if(!di_handle->connected)
		return;
	ndev_data_ping_status ping_buffer;
	int result = ndev_device_send_command_in(di_handle, (uint16_t*)&ping_buffer, NDEV_DI_GET_STATUS_PING, sizeof(ping_buffer) / sizeof(uint16_t));
	if(result < 0)
		return disconnect_on_failure(di_handle);
	if((ping_buffer.status.bits & 0x00C4) == 0x00C4)
		return;
	ndev_data_status status;
	status.bits = 0;
	result = ndev_device_send_command_in_u16(di_handle, &status.bits, NDEV_DI_GET_STATUS_BITFIELD);
	if(result < 0)
		return disconnect_on_failure(di_handle);
	result = ndev_device_send_command_out_u16(di_handle, (~status.bits) & 0x00C4, NDEV_DI_SET_STATUS_BITFIELD);
	if(result < 0)
		return disconnect_on_failure(di_handle);
	if(!status.fields.disk_request_acknowledged) {
		ndev_data_disk_status disk_status;
		disk_status.bits = 0;
		result = ndev_device_send_command_in_u16(di_handle, &disk_status.bits, NDEV_DI_GET_DISK_STATUS);
		if(result < 0)
			return disconnect_on_failure(di_handle);
		disk_status.fields.is_request_ready = 1;
		disk_status.fields.is_disk_inserted = 0;
		result = ndev_device_send_command_out_u16(di_handle, disk_status.bits, NDEV_DI_SET_DISK_STATUS);
		if(result < 0)
			return disconnect_on_failure(di_handle);
		int num_transferred = 0;
		ndev_device_fifo_transfer_out(di_handle, no_disk_inserted, no_disk_inserted_len, &num_transferred);
		if(result < 0)
			return disconnect_on_failure(di_handle);
		if(num_transferred != no_disk_inserted_len)
			return disconnect_on_failure(di_handle);
	}
	if(!status.fields.unk4) {
		result = setup_unk_bytes(di_handle, false, true);
		if(result < 0)
			return disconnect_on_failure(di_handle);
	}
}

void disconnect_from_disk_interface(ndev_device_handle* dev_handle) {
	switch (dev_handle->connection_type) {
		case LIBUSB_CONNECTED:
			libusb_disconnect_from_ndev_device(dev_handle);
			break;
		case ORIGINAL_DRIVER_CONNECTED:
			driver_disconnect_ndev_device(dev_handle);
			break;
		default:
			break;
	}
	dev_handle->connected = false;
	dev_handle->connection_type = NO_CONNECTION_DONE;
}

bool is_ndev_serial_same(uint8_t* expected_data, uint8_t* got_data) {
	if(expected_data == NULL)
		return true;
	for(int i = 0; i < NDEV_SERIAL_DATA_SIZE; i++)
		if(expected_data[i] != got_data[i])
			return false;
	return true;
}
