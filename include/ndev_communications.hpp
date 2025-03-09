#ifndef __NDEV_COMMUNICATIONS_HPP
#define __NDEV_COMMUNICATIONS_HPP

#include <cstdint>
#include <string>

#define NDEV_SERIAL_DATA_SIZE 4

enum NdevConnectionType { NO_CONNECTION_DONE, LIBUSB_CONNECTED, ORIGINAL_DRIVER_CONNECTED };

struct ndev_usb_data {
	uint16_t vid;
	uint16_t pid;
	uint16_t bcd_device;
	const uint8_t* payload_device;
	size_t payload_size;
	size_t packet_size;
	bool is_initial_setup;
	bool re_check;
	int timeout_ms;
	int interface_to_claim;
};

struct ndev_device_handle {
	void* usb_handle = NULL;
	void* file_handle = NULL;
	void* mutex_handle = NULL;
	std::string path;
	volatile bool connected = false;
	NdevConnectionType connection_type = NO_CONNECTION_DONE;
	const ndev_usb_data* associated_device = NULL;
	uint8_t serial[NDEV_SERIAL_DATA_SIZE];
	uint16_t hw_rev = 0;
};

int get_number_ndev_setup_data(void);
const ndev_usb_data* get_ndev_setup_data(int index);
void prepare_ndev_devices(void);
void prepare_communications(void);
void close_communications(void);
bool is_ndev_serial_same(uint8_t* expected_data, uint8_t* got_data);
ndev_device_handle connect_to_disk_interface(uint8_t* expected_data);
void setup_disk_interface_active(ndev_device_handle* di_handle);
void iter_keep_disk_interface_active(ndev_device_handle* di_handle);
void disconnect_from_disk_interface(ndev_device_handle* di_handle);

int get_ndev_device_serial(ndev_device_handle* dev_handle);
int get_ndev_device_has_done_second_setup(ndev_device_handle* dev_handle, bool* done);
int ndev_device_wait_transfer_ready(ndev_device_handle* dev_handle);

#endif
