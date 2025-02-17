#include <iostream>

#define NAME "ndev_activator"

static void ConsoleOutText(std::string full_text) {
	if(full_text != "")
		std::cout << "[" << NAME << "] " << full_text << std::endl;
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

int main(int argc, char **argv) {
	int ret_val = 0;
	return ret_val;
}
