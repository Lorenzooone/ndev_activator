cmake_minimum_required(VERSION 3.16)
set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE STRING "Build archs for Mac OS X" FORCE)
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING "Minimum OS X deployment version")
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
set(TARGET_LANGUAGES CXX C)
project(ndev_activator VERSION 1.0.0 LANGUAGES ${TARGET_LANGUAGES})
include(ExternalProject)

set(FTDI_EEPROM OFF)
set(WINDOWS_ARM64 0)
set(WINDOWS_x86_32 0)
set(FETCHCONTENT_UPDATES_DISCONNECTED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set(CMAKE_HOST_CXX_COMPILER ${CMAKE_CXX_COMPILER} CACHE STRING "Host Compiler")
set(FETCHCONTENT_QUIET FALSE)
set(EXTRA_CXX_FLAGS "")
set(EXTRA_LIBRARIES "")
if(MINGW_CROSS_COMPILATION)
	list(APPEND EXTRA_LIBRARIES libpthread.a)
endif()
set(FINAL_EXTENSION "")
set(HOST_FINAL_EXTENSION "")
set(SETUP_SCRIPTS_DIR ${CMAKE_SOURCE_DIR}/setup_scripts)
set(USB_RULES_DIR ${CMAKE_SOURCE_DIR}/usb_rules)
set(SCRIPT_EXTENSION "")

if((${CMAKE_SYSTEM_NAME} STREQUAL "Windows") AND ((${CMAKE_GENERATOR_PLATFORM} MATCHES "ARM64") OR (("${CMAKE_GENERATOR_PLATFORM}" STREQUAL "") AND (${CMAKE_SYSTEM_PROCESSOR} MATCHES "ARM64"))))
	set(WINDOWS_ARM64 1)
endif()

if((${CMAKE_SYSTEM_NAME} STREQUAL "Windows") AND ((${CMAKE_GENERATOR_PLATFORM} MATCHES "Win32") OR (("${CMAKE_GENERATOR_PLATFORM}" STREQUAL "") AND ((${CMAKE_SYSTEM_PROCESSOR} MATCHES "Win32") OR (${CMAKE_SYSTEM_PROCESSOR} MATCHES "x86") OR (${CMAKE_SYSTEM_PROCESSOR} MATCHES "X86")))))
	set(WINDOWS_x86_32 1)
endif()

# Avoid warning about DOWNLOAD_EXTRACT_TIMESTAMP in CMake 3.24:
if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.24.0")
	cmake_policy(SET CMP0135 NEW)
endif()

include(FetchContent)
	FetchContent_Declare(LibUSB
		GIT_REPOSITORY https://github.com/libusb/libusb-cmake.git
		GIT_TAG main
		OVERRIDE_FIND_PACKAGE)

if(${CMAKE_HOST_SYSTEM_NAME} STREQUAL "Windows")
	set(HOST_FINAL_EXTENSION ".exe")
	set(SCRIPT_EXTENSION ".bat")
else()
	set(SCRIPT_EXTENSION ".sh")
endif()

if(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
	set(FINAL_EXTENSION ".exe")
	list(APPEND EXTRA_LIBRARIES setupapi.lib)
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
else()
endif()

set(TOOLS_DATA_DIR ${CMAKE_BINARY_DIR}/tools_and_data)
file(MAKE_DIRECTORY ${TOOLS_DATA_DIR})

set(FETCH_CONTENT_MAKE_AVAILABLE_LIBRARIES "")
set(FETCH_CONTENT_MAKE_AVAILABLE_LIBRARIES_SEPARATE "")
set(SOURCE_CPP_EXTRA_FILES "")
set(EXTRA_DEPENDENCIES "")
list(APPEND FETCH_CONTENT_MAKE_AVAILABLE_LIBRARIES LibUSB)

FetchContent_MakeAvailable(${FETCH_CONTENT_MAKE_AVAILABLE_LIBRARIES})

# Find libusb
set(LIBUSB_INCLUDE_DIR ${libusb_SOURCE_DIR}/libusb/libusb)
FetchContent_MakeAvailable(${FETCH_CONTENT_MAKE_AVAILABLE_LIBRARIES_SEPARATE})

set(EXTRA_INCLUDE_DIRECTORIES "")
list(APPEND EXTRA_INCLUDE_DIRECTORIES ${libusb_SOURCE_DIR}/libusb/libusb)
if(IS_DIRECTORY "${libusb_SOURCE_DIR}")
	set_property(DIRECTORY ${libusb_SOURCE_DIR} PROPERTY EXCLUDE_FROM_ALL YES)
endif()

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

# search headers and libraries in the target environment
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

execute_process(COMMAND ${CMAKE_COMMAND} -B ${TOOLS_DATA_DIR} ${CMAKE_SOURCE_DIR}/tools)
execute_process(COMMAND ${CMAKE_COMMAND} --build ${TOOLS_DATA_DIR})

set(OUTPUT_NAME ndev_activator)

add_executable(${OUTPUT_NAME} source/ndev_activator.cpp source/ndev_communications.cpp source/ndev_libusb_communications.cpp source/ndev_driver_communications.cpp source/usb_generic.cpp source/utils.cpp ${TOOLS_DATA_DIR}/usb_setup_data_0301_0100.cpp ${TOOLS_DATA_DIR}/usb_setup_data_0301_0200.cpp ${TOOLS_DATA_DIR}/usb_setup_data_0302_0100.cpp ${TOOLS_DATA_DIR}/payload_0301_0201.cpp ${TOOLS_DATA_DIR}/payload_0302_0101.cpp ${TOOLS_DATA_DIR}/no_disk_inserted.cpp ${SOURCE_CPP_EXTRA_FILES})

if(NOT ("${EXTRA_DEPENDENCIES}" STREQUAL ""))
	add_dependencies(${OUTPUT_NAME} ${EXTRA_DEPENDENCIES})
endif()

target_link_libraries(${OUTPUT_NAME} PRIVATE usb-1.0 ${EXTRA_LIBRARIES})
target_include_directories(${OUTPUT_NAME} PRIVATE ${EXTRA_INCLUDE_DIRECTORIES} ${TOOLS_DATA_DIR} ${CMAKE_SOURCE_DIR}/include)
target_compile_features(${OUTPUT_NAME} PRIVATE cxx_std_20)
target_compile_options(${OUTPUT_NAME} PRIVATE ${EXTRA_CXX_FLAGS})

add_custom_command(
	OUTPUT ${TOOLS_DATA_DIR}/usb_setup_data_0301_0100.cpp
	COMMENT "Convert USB Setup data to C - 0301 0100"
	COMMAND ${TOOLS_DATA_DIR}/CMakeBin2C${HOST_FINAL_EXTENSION} ${CMAKE_SOURCE_DIR}/bin/usb_setup_data_0301_0100.bin ${TOOLS_DATA_DIR} usb_setup_data_0301_0100 usb_setup_data_0301_0100
	DEPENDS ${CMAKE_SOURCE_DIR}/bin/usb_setup_data_0301_0100.bin ${TOOLS_DATA_DIR}/CMakeBin2C${HOST_FINAL_EXTENSION}
)

add_custom_command(
	OUTPUT ${TOOLS_DATA_DIR}/usb_setup_data_0301_0200.cpp
	COMMENT "Convert USB Setup data to C - 0301 0200"
	COMMAND ${TOOLS_DATA_DIR}/CMakeBin2C${HOST_FINAL_EXTENSION} ${CMAKE_SOURCE_DIR}/bin/usb_setup_data_0301_0200.bin ${TOOLS_DATA_DIR} usb_setup_data_0301_0200 usb_setup_data_0301_0200
	DEPENDS ${CMAKE_SOURCE_DIR}/bin/usb_setup_data_0301_0200.bin ${TOOLS_DATA_DIR}/CMakeBin2C${HOST_FINAL_EXTENSION}
)

add_custom_command(
	OUTPUT ${TOOLS_DATA_DIR}/usb_setup_data_0302_0100.cpp
	COMMENT "Convert USB Setup data to C - 0302 0100"
	COMMAND ${TOOLS_DATA_DIR}/CMakeBin2C${HOST_FINAL_EXTENSION} ${CMAKE_SOURCE_DIR}/bin/usb_setup_data_0302_0100.bin ${TOOLS_DATA_DIR} usb_setup_data_0302_0100 usb_setup_data_0302_0100
	DEPENDS ${CMAKE_SOURCE_DIR}/bin/usb_setup_data_0302_0100.bin ${TOOLS_DATA_DIR}/CMakeBin2C${HOST_FINAL_EXTENSION}
)

add_custom_command(
	OUTPUT ${TOOLS_DATA_DIR}/payload_0301_0201.cpp
	COMMENT "Convert USB Setup data to C - 0301 0100"
	COMMAND ${TOOLS_DATA_DIR}/CMakeBin2C${HOST_FINAL_EXTENSION} ${CMAKE_SOURCE_DIR}/bin/payload_0301_0201.bin ${TOOLS_DATA_DIR} payload_0301_0201 payload_0301_0201
	DEPENDS ${CMAKE_SOURCE_DIR}/bin/payload_0301_0201.bin ${TOOLS_DATA_DIR}/CMakeBin2C${HOST_FINAL_EXTENSION}
)

add_custom_command(
	OUTPUT ${TOOLS_DATA_DIR}/payload_0302_0101.cpp
	COMMENT "Convert USB Payload to C - 0302 0101"
	COMMAND ${TOOLS_DATA_DIR}/CMakeBin2C${HOST_FINAL_EXTENSION} ${CMAKE_SOURCE_DIR}/bin/payload_0302_0101.bin ${TOOLS_DATA_DIR} payload_0302_0101 payload_0302_0101
	DEPENDS ${CMAKE_SOURCE_DIR}/bin/payload_0302_0101.bin ${TOOLS_DATA_DIR}/CMakeBin2C${HOST_FINAL_EXTENSION}
)

add_custom_command(
	OUTPUT ${TOOLS_DATA_DIR}/no_disk_inserted.cpp
	COMMENT "Convert USB Payload to C - No disk inserted data"
	COMMAND ${TOOLS_DATA_DIR}/CMakeBin2C${HOST_FINAL_EXTENSION} ${CMAKE_SOURCE_DIR}/bin/no_disk_inserted.bin ${TOOLS_DATA_DIR} no_disk_inserted no_disk_inserted
	DEPENDS ${CMAKE_SOURCE_DIR}/bin/no_disk_inserted.bin ${TOOLS_DATA_DIR}/CMakeBin2C${HOST_FINAL_EXTENSION}
)

add_custom_command(
	TARGET ${OUTPUT_NAME}
	COMMENT "Copy Output"
	POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${OUTPUT_NAME}> ${CMAKE_SOURCE_DIR}
	VERBATIM
)

if(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
	install(TARGETS ${OUTPUT_NAME} DESTINATION .)
else()
	install(FILES ${CMAKE_SOURCE_DIR}/${OUTPUT_NAME}${FINAL_EXTENSION} DESTINATION .)
endif()
install(FILES LICENSE README.md DESTINATION .)
install(DIRECTORY "other licenses" DESTINATION .)

if(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
	set_target_properties(${OUTPUT_NAME} PROPERTIES
						  MACOSX_BUNDLE TRUE
						  MACOSX_BUNDLE_BUNDLE_NAME ${OUTPUT_NAME}
						  MACOSX_BUNDLE_GUI_IDENTIFIER ${OUTPUT_NAME}
						  MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/data/MacOSXBundleInfo.plist.in)
	set(CMAKE_MACOSX_RPATH TRUE)

elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
else()
	install(FILES ${USB_RULES_DIR}/51-ndev.rules DESTINATION .)
	install(FILES ${SETUP_SCRIPTS_DIR}/install_usb_rules${SCRIPT_EXTENSION} DESTINATION .)
endif()


set(CPACK_PACKAGE_NAME ${OUTPUT_NAME})
set(CPACK_PACKAGE_VENDOR Lorenzooone)
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "NDEV Activator software")

set(CPACK_PACKAGE_INSTALL_DIRECTORY ${CPACK_PACKAGE_NAME})
set(CPACK_VERBATIM_VARIABLES TRUE)

if(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
	set(CPACK_PACKAGE_FILE_NAME ${CPACK_PACKAGE_NAME}_macos)
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
	if(WINDOWS_ARM64)
		set(CPACK_PACKAGE_FILE_NAME ${CPACK_PACKAGE_NAME}_windows_arm64)
	elseif(WINDOWS_x86_32)
		set(CPACK_PACKAGE_FILE_NAME ${CPACK_PACKAGE_NAME}_windows_x86_32)
	else()
		set(CPACK_PACKAGE_FILE_NAME ${CPACK_PACKAGE_NAME}_windows_x86_64)
	endif()
else()
	set(CPACK_PACKAGE_FILE_NAME ${CPACK_PACKAGE_NAME}_linux_${CMAKE_SYSTEM_PROCESSOR})
endif()

include(CPack)
