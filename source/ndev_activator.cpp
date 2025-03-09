#include <iostream>
#include <chrono>
#include <thread>
#include "ndev_communications.hpp"

#define NAME "ndev_activator"

static void ConsoleOutText(std::string full_text) {
	if(full_text != "")
		std::cout << "[" << NAME << "] " << full_text << std::endl;
}

static char nybble_to_char(uint8_t nybble) {
	nybble &= 0xF;
	if(nybble < 0xA)
		return '0' + nybble;
	return 'A' + nybble;
}

static uint8_t char_to_nybble(char data) {
	if((data >= '0') && (data <= '9'))
		return data - '0';
	if((data >= 'a') && (data <= 'f'))
		return (data - 'a') + 0xA;
	if((data >= 'A') && (data <= 'F'))
		return (data - 'A') + 0xA;
	return 0xFF;
}

static std::string serial_to_string(uint8_t* serial) {
	std::string out_serial = "";
	for(int i = 0; i < NDEV_SERIAL_DATA_SIZE; i++)
		for(int j = 0; j < 2; j++)
			out_serial += nybble_to_char(serial[i] >> ((1 - j) * 4));
	return out_serial;
}

static bool parse_existence_arg(int &index, char **argv, bool &target, bool existence_value, std::string to_check) {
	if(argv[index] != to_check)
		return false;
	target = existence_value;
	return true;
}

static bool parse_int_arg(int &index, int argc, char **argv, int &target, std::string to_check) {
	if(argv[index] != to_check)
		return false;
	if((++index) >= argc)
		return true;
	try {
		target = std::stoi(argv[index]);
	}
	catch(...) {
		std::cerr << "Error with input for: " << to_check << std::endl;
	}
	return true;
}

static bool parse_serial_arg(int &index, int argc, char **argv, uint8_t *wanted_serial, bool &has_read_serial, std::string to_check) {
	if(argv[index] != to_check)
		return false;
	if((++index) >= argc)
		return true;
	try {
		uint8_t tmp_wanted_serial[NDEV_SERIAL_DATA_SIZE];
		std::string target = std::string(argv[index]);
		for(int i = 0; i < NDEV_SERIAL_DATA_SIZE; i++) {
			tmp_wanted_serial[i] = 0;
			for(int j = 0; j < 2; j++) {
				uint8_t read_output = char_to_nybble(target[(i * 2) + j]);
				if(read_output == 0xFF)
			        throw std::invalid_argument("Error in read string");
				tmp_wanted_serial[i] += read_output << (4 * (1 - j));
			}
		}
		for(int i = 0; i < NDEV_SERIAL_DATA_SIZE; i++)
			wanted_serial[i] = tmp_wanted_serial[i];
		has_read_serial = true;
	}
	catch(...) {
		std::cerr << "Error with input for: " << to_check << std::endl;
	}
	return true;
}

static void thread_do_di_keepalive(ndev_device_handle* di_handle, volatile bool* has_asked_stop) {
	while(di_handle->connected && (!(*has_asked_stop))) {
		iter_keep_disk_interface_active(di_handle);
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
	}
}

int main(int argc, char **argv) {
	int ret_val = 0;
	volatile bool has_asked_stop = false;
	bool has_read_serial = false;
	uint8_t wanted_serial[NDEV_SERIAL_DATA_SIZE];
	uint8_t* serial_ptr = NULL;
	for (int i = 1; i < argc; i++) {
		if(parse_serial_arg(i, argc, argv, wanted_serial, has_read_serial, "--serial"))
			continue;
		if(parse_serial_arg(i, argc, argv, wanted_serial, has_read_serial, "-s"))
			continue;
		std::cout << "Help:" << std::endl;
		std::cout << "  --serial/-s      Sets the serial that this application should look for." << std::endl;
		return 0;
	}
	prepare_communications();
	prepare_ndev_devices();
	if(has_read_serial)
		serial_ptr = wanted_serial;
	ndev_device_handle di_handle = connect_to_disk_interface(serial_ptr);
	if(di_handle.connected)
		ConsoleOutText("Connected to DI of: " + serial_to_string(di_handle.serial));
	setup_disk_interface_active(&di_handle);
	if(di_handle.connected) {
		ConsoleOutText("Setup completed for DI of: " + serial_to_string(di_handle.serial));
		ConsoleOutText("Mantaining DI alive");
		ConsoleOutText("Press CTRL+C to stop the program");
		std::thread keepalive_thread(thread_do_di_keepalive, &di_handle, &has_asked_stop);
		// It's 2025 and there is no portable way to asynchronously check
		// for console input in C++... :/
		// Or with a timeout of sorts...
		while(di_handle.connected && (!has_asked_stop)) {
			std::this_thread::sleep_for(std::chrono::milliseconds(50));
		}
		keepalive_thread.join();
		if(!di_handle.connected)
			ConsoleOutText("DI disconnected");
	}
	disconnect_from_disk_interface(&di_handle);
	close_communications();
	return ret_val;
}
