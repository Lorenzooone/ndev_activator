#include "ndev_driver_communications.hpp"
#include "utils.hpp"
#include <string>

#include <iostream>
#include <cstdio>

// Used for Errors
#include <libusb.h>

#define MAX_TIMEOUT_MS_INITIAL_IO_CONTROL 500

#ifdef _WIN32
#include <windows.h>
#include <setupapi.h>
#include <filesystem>
#include <algorithm>

#pragma pack(push, 1)

struct PACKED ndev_di_driver_info {
	uint32_t unk[2];
	uint16_t unk2;
	uint16_t vid;
	uint16_t pid;
	uint16_t bcd_device;
	uint32_t unk4[4];
};

struct PACKED ndev_com_debug_driver_info {
	uint32_t unk[2];
	uint16_t unk2[2];
	uint16_t vid;
	uint16_t pid;
	uint16_t bcd_device;
	uint16_t unk3;
	uint32_t unk4[3];
};

struct PACKED ndev_driver_ctrl_in_packet {
	uint16_t bRequest;
	uint16_t wValue;
	uint16_t wIndex;
	uint16_t wLength;
};

#pragma pack(pop)

static bool SynchronousDeviceIoControl(HANDLE handle, uint32_t ctl, uint8_t* buf_in, size_t size_in, uint8_t* buf_out, size_t size_out, int* num_read, int timeout_ms = MAX_TIMEOUT_MS_INITIAL_IO_CONTROL);

const GUID ndev_di_driver_guid = { .Data1 = 0x5744627F, .Data2 = 0x5ED9, .Data3 = 0x4DA4, .Data4 = {0x86, 0xE9, 0x62, 0x48, 0xE9, 0x18, 0xAE, 0x80} };
const GUID ndev_com_debug_driver_guid = { .Data1 = 0x9DA9ADEF, .Data2 = 0xC187, .Data3 = 0x4DEC, .Data4 = {0x8B, 0x85, 0x3D, 0x36, 0xAA, 0xDE, 0x1A, 0x88} };

static int wait_result_io_operation(HANDLE handle, OVERLAPPED* overlapped_var, int* transferred, int timeout_ms) {
	DWORD num_bytes = 0;
	int retval = 0;
	int error = 0;
	do {
		retval = GetOverlappedResultEx(handle, overlapped_var, &num_bytes, timeout_ms, true);
		error = GetLastError();
	} while ((retval == 0) && (error == WAIT_IO_COMPLETION));
	*transferred = num_bytes;
	if (retval == 0) {
		if (error == WAIT_TIMEOUT) {
			CancelIoEx(handle, overlapped_var);
			return LIBUSB_ERROR_TIMEOUT;
		}
		return LIBUSB_ERROR_OTHER;
	}
	return LIBUSB_SUCCESS;
}

static int wait_result_io_operation(HANDLE handle, OVERLAPPED* overlapped_var, int* transferred, const ndev_usb_data* usb_device_desc) {
	return wait_result_io_operation(handle, overlapped_var, transferred, usb_device_desc->timeout_ms);
}

static bool SynchronousDeviceIoControl(HANDLE handle, uint32_t ctl, uint8_t* buf_in, size_t size_in, uint8_t* buf_out, size_t size_out, int* num_read, int timeout_ms) {
	DWORD bytes_read = 0;
	OVERLAPPED overlapped_var;
	memset(&overlapped_var, 0, sizeof(OVERLAPPED));
	bool result = DeviceIoControl(handle, ctl, buf_in, size_in, buf_out, size_out, &bytes_read, &overlapped_var);
	if ((!result) && (GetLastError() == ERROR_IO_PENDING)) {
		int wait_result = wait_result_io_operation(handle, &overlapped_var, num_read, timeout_ms);
		if(wait_result < 0)
			return false;
		return true;
	}
	else if (result)
		*num_read = bytes_read;
	return result;
}

static bool read_ndev_driver_device_info(HANDLE handle, uint8_t* buffer, size_t size, int* num_read) {
	return SynchronousDeviceIoControl(handle, 0x80002000, NULL, 0, buffer, size, num_read);
}

static bool _ndev_driver_ctrl_transfer_in(HANDLE handle, uint8_t bRequest, uint16_t wValue, uint16_t wIndex, unsigned char* data, uint16_t wLength, int* num_read) {
	ndev_driver_ctrl_in_packet ctrl_in_packet;
	ctrl_in_packet.bRequest = bRequest;
	ctrl_in_packet.wValue = wValue;
	ctrl_in_packet.wIndex = wIndex;
	ctrl_in_packet.wLength = wLength;
	return SynchronousDeviceIoControl(handle, 0x80002004, (uint8_t*)&ctrl_in_packet, sizeof(ndev_driver_ctrl_in_packet), data, wLength, num_read);
}

static bool _ndev_driver_fifo_transfer_out(HANDLE handle, const unsigned char* data, size_t length, int* num_read) {
	return SynchronousDeviceIoControl(handle, 0x80002034, (uint8_t*)data, length, NULL, 0, num_read);
}

static bool create_file_and_read_ndev_driver_device_info(std::string path, uint8_t* data, size_t size) {
	memset(data, 0, size);
	int num_read = 0;
	HANDLE handle = CreateFile(path.c_str(), (GENERIC_READ | GENERIC_WRITE), (FILE_SHARE_READ | FILE_SHARE_WRITE), NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
	if (handle == INVALID_HANDLE_VALUE)
		return false;
	bool result = read_ndev_driver_device_info(handle, data, size, &num_read);
	CloseHandle(handle);
	if (!result)
		return false;
	if (num_read < size)
		return false;
	return true;
}

static bool ndev_com_debug_driver_get_device_pid_vid_device(std::string path, uint16_t& out_vid, uint16_t& out_pid, uint16_t& out_bcd_device) {
	ndev_com_debug_driver_info com_debug_driver_data;
	bool result = create_file_and_read_ndev_driver_device_info(path, (uint8_t*)&com_debug_driver_data, sizeof(ndev_com_debug_driver_info));
	if (!result)
		return false;
	out_vid = com_debug_driver_data.vid;
	out_pid = com_debug_driver_data.pid;
	out_bcd_device = com_debug_driver_data.bcd_device;
	return true;
}

static bool ndev_di_driver_get_device_pid_vid_device(std::string path, uint16_t& out_vid, uint16_t& out_pid, uint16_t& out_bcd_device) {
	ndev_di_driver_info di_driver_data;
	bool result = create_file_and_read_ndev_driver_device_info(path, (uint8_t*)&di_driver_data, sizeof(ndev_di_driver_info));
	if (!result)
		return false;
	out_vid = di_driver_data.vid;
	out_pid = di_driver_data.pid;
	out_bcd_device = di_driver_data.bcd_device;
	return true;
}

static bool ndev_driver_get_device_pid_vid_device(std::string path, uint16_t& out_vid, uint16_t& out_pid, uint16_t& out_bcd_device, bool is_di) {
	if (is_di)
		return ndev_di_driver_get_device_pid_vid_device(path, out_vid, out_pid, out_bcd_device);
	return ndev_com_debug_driver_get_device_pid_vid_device(path, out_vid, out_pid, out_bcd_device);
}

static std::string ndev_driver_get_device_path(HDEVINFO DeviceInfoSet, SP_DEVICE_INTERFACE_DATA* DeviceInterfaceData) {
	std::string result = "";
	// Call this with an empty buffer to the required size of the structure.
	ULONG requiredSize;
	SetupDiGetDeviceInterfaceDetail(DeviceInfoSet, DeviceInterfaceData, NULL, 0, &requiredSize, NULL);
	if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
		return result;

	PSP_DEVICE_INTERFACE_DETAIL_DATA devInterfaceDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)(new uint8_t[requiredSize]);
	if (!devInterfaceDetailData)
		return result;

	devInterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
	if (SetupDiGetDeviceInterfaceDetail(DeviceInfoSet, DeviceInterfaceData, devInterfaceDetailData, requiredSize, &requiredSize, NULL))
		result = (std::string)(devInterfaceDetailData->DevicePath);

	delete[]((uint8_t*)devInterfaceDetailData);
	return result;
}

static bool ndev_driver_setup_connection(ndev_device_handle* out_handle, std::string path) {
	out_handle->usb_handle = NULL;
	out_handle->mutex_handle = NULL;
	out_handle->file_handle = INVALID_HANDLE_VALUE;
	out_handle->path = path;
	out_handle->connected = false;
	out_handle->connection_type = ORIGINAL_DRIVER_CONNECTED;
	std::string mutex_name = path;
	std::replace(mutex_name.begin(), mutex_name.end(), '\\', '_');
	out_handle->mutex_handle = CreateMutex(NULL, true, mutex_name.c_str());
	if ((out_handle->mutex_handle != NULL) && (GetLastError() == ERROR_ALREADY_EXISTS)) {
		CloseHandle(out_handle->mutex_handle);
		out_handle->mutex_handle = NULL;
	}
	if(out_handle->mutex_handle == NULL)
		return false;
	out_handle->file_handle = CreateFile(path.c_str(), (GENERIC_READ | GENERIC_WRITE), (FILE_SHARE_READ | FILE_SHARE_WRITE), NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
	if(out_handle->file_handle == INVALID_HANDLE_VALUE)
		return false;
	out_handle->connected = true;
	return true;
}

static void ndev_driver_close_handle(void** handle, void* default_value = INVALID_HANDLE_VALUE) {
	if ((*handle) == default_value)
		return;
	CloseHandle(*handle);
	*handle = default_value;
}

#endif

int ndev_driver_ctrl_transfer_in(void* handle, uint8_t bRequest, uint16_t wValue, uint16_t wIndex, unsigned char* data, uint16_t wLength) {
#ifdef _WIN32
	int num_read = 0;
	bool result = _ndev_driver_ctrl_transfer_in((HANDLE)handle, bRequest, wValue, wIndex, data, wLength, &num_read);
	if (!result)
		return -1;
	return num_read;
#endif
	return 0;
}

int ndev_driver_bulk_transfer_in(void* handle, const ndev_usb_data* usb_device_desc, unsigned char* data, size_t length, int* num_transferred) {
#ifdef _WIN32
	DWORD bytes_read = 0;
	OVERLAPPED overlapped_var;
	memset(&overlapped_var, 0, sizeof(OVERLAPPED));
	bool result = ReadFile((HANDLE)handle, data, length, &bytes_read, &overlapped_var);
	if ((!result) && (GetLastError() == ERROR_IO_PENDING))
		return wait_result_io_operation(handle, &overlapped_var, num_transferred, usb_device_desc);
	else if (result) {
		*num_transferred = bytes_read;
		return 0;
	}
	else
		return -1;
#endif
	return 0;
}

int ndev_driver_bulk_transfer_out(void* handle, const ndev_usb_data* usb_device_desc, const unsigned char* data, size_t length, int* num_transferred) {
#ifdef _WIN32
	DWORD bytes_written = 0;
	OVERLAPPED overlapped_var;
	memset(&overlapped_var, 0, sizeof(OVERLAPPED));
	bool result = WriteFile((HANDLE)handle, data, length, &bytes_written, &overlapped_var);
	if ((!result) && (GetLastError() == ERROR_IO_PENDING))
		return wait_result_io_operation(handle, &overlapped_var, num_transferred, usb_device_desc);
	else if (result) {
		*num_transferred = bytes_written;
		return 0;
	}
	else
		return -1;
#endif
	return 0;
}

int ndev_driver_fifo_transfer_out(void* handle, const unsigned char* data, size_t length, int* num_transferred) {
#ifdef _WIN32
	bool result = _ndev_driver_fifo_transfer_out((HANDLE)handle, data, length, num_transferred);
	if (!result)
		return -1;
	*num_transferred = length;
	return 0;
#endif
	return 0;
}

void driver_connect_to_ndev_device(uint8_t* expected_data, ndev_device_handle* out_handle) {
#ifdef _WIN32
	HDEVINFO DeviceInfoSet = SetupDiGetClassDevs(
		&ndev_di_driver_guid,
		NULL,
		NULL,
		DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);

	SP_DEVICE_INTERFACE_DATA DeviceInterfaceData;
	ZeroMemory(&DeviceInterfaceData, sizeof(SP_DEVICE_INTERFACE_DATA));
	DeviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
	uint32_t i = 0;
	while (SetupDiEnumDeviceInterfaces(DeviceInfoSet, NULL, &ndev_di_driver_guid, i++, &DeviceInterfaceData)) {
		ndev_device_handle internal_handle = *out_handle;
		std::string path = ndev_driver_get_device_path(DeviceInfoSet, &DeviceInterfaceData);
		if (path == "")
			continue;
		uint16_t vid = 0;
		uint16_t pid = 0;
		uint16_t bcd_device = 0;
		bool success = ndev_driver_get_device_pid_vid_device(path, vid, pid, bcd_device, true);
		if(!success)
			continue;
		const ndev_usb_data* possible_usb_data = internal_handle.associated_device;
		if ((possible_usb_data->pid != pid) || (possible_usb_data->vid != vid) || (possible_usb_data->bcd_device != bcd_device))
			continue;
		if (!ndev_driver_setup_connection(&internal_handle, path)) {
			driver_disconnect_ndev_device(&internal_handle);
			continue;
		}
		bool done_setup = false;
		int result = get_ndev_device_has_done_second_setup(&internal_handle, &done_setup);
		if ((result < 0) || (!done_setup)) {
			driver_disconnect_ndev_device(&internal_handle);
			continue;
		}
		result = get_ndev_device_serial(&internal_handle);
		if ((result >= 0) && is_ndev_serial_same(expected_data, internal_handle.serial) && (ndev_device_wait_transfer_ready(&internal_handle) >= 0)) {
			*out_handle = internal_handle;
			break;
		}
		driver_disconnect_ndev_device(&internal_handle);
	}

	if (DeviceInfoSet) {
		SetupDiDestroyDeviceInfoList(DeviceInfoSet);
	}
#endif
}

void driver_disconnect_ndev_device(ndev_device_handle* handlers) {
#ifdef _WIN32
	ndev_driver_close_handle(&handlers->file_handle);
	ndev_driver_close_handle(&handlers->mutex_handle, NULL);
	handlers->connected = false;
#endif
}
