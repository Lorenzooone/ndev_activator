#ifndef __NDEV_COMMUNICATIONS_HPP
#define __NDEV_COMMUNICATIONS_HPP

#define NDEV_SERIAL_DATA_SIZE 4

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

struct ndev_di_handle {
	void* handle;
	volatile bool connected = false;
	uint8_t serial[NDEV_SERIAL_DATA_SIZE];
	uint16_t hw_rev = 0;
};

int get_number_ndev_setup_data(void);
const ndev_usb_data* get_ndev_setup_data(int index);
const ndev_usb_data* get_di_ndev_data(void);
void prepare_ndev_devices(void);
void prepare_communications(void);
void close_communications(void);
bool is_ndev_serial_same(uint8_t* expected_data, uint8_t* got_data);
ndev_di_handle connect_to_disk_interface(uint8_t* expected_data);
void setup_disk_interface_active(ndev_di_handle* di_handle);
void iter_keep_disk_interface_active(ndev_di_handle* di_handle);
void disconnect_from_disk_interface(ndev_di_handle* di_handle);

#endif
