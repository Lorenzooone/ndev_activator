#ifndef __NDEV_COMMUNICATIONS_HPP
#define __NDEV_COMMUNICATIONS_HPP

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

int get_number_ndev_setup_data(void);
const ndev_usb_data* get_ndev_setup_data(int index);
void prepare_ndev_devices(void);
void prepare_communications(void);
void close_communications(void);

#endif
