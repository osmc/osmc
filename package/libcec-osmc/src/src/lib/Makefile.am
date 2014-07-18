lib_LTLIBRARIES = libcec.la
library_includedir=$(includedir)/libcec
library_include_HEADERS = ../../include/cec.h \
                          ../../include/cecc.h \
                          ../../include/cectypes.h \
                          ../../include/cecloader.h

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libcec.pc

## libCEC core
libcec_la_SOURCES = CECProcessor.cpp \
                    LibCEC.cpp \
                    LibCECC.cpp \
                    CECClient.cpp \
                    adapter/AdapterFactory.cpp \
                    devices/CECAudioSystem.cpp \
                    devices/CECBusDevice.cpp \
                    devices/CECDeviceMap.cpp \
                    devices/CECPlaybackDevice.cpp \
                    devices/CECRecordingDevice.cpp \
                    devices/CECTuner.cpp \
                    devices/CECTV.cpp \
                    implementations/ANCommandHandler.cpp \
                    implementations/CECCommandHandler.cpp \
                    implementations/SLCommandHandler.cpp \
                    implementations/VLCommandHandler.cpp \
                    implementations/RLCommandHandler.cpp \
                    implementations/PHCommandHandler.cpp \
                    implementations/RHCommandHandler.cpp \
                    implementations/AQCommandHandler.cpp

## server sockets, currently unused
##libcec_la_SOURCES += platform/posix/serversocket.cpp

## Pulse-Eight USB-CEC support
if USE_P8_USB
libcec_la_SOURCES += adapter/Pulse-Eight/USBCECAdapterMessage.cpp \
                     adapter/Pulse-Eight/USBCECAdapterCommands.cpp \
                     adapter/Pulse-Eight/USBCECAdapterCommunication.cpp \
                     adapter/Pulse-Eight/USBCECAdapterMessageQueue.cpp \
                     adapter/Pulse-Eight/USBCECAdapterDetection.cpp \
                     platform/posix/serialport.cpp \
                     platform/posix/os-edid.cpp \
                     platform/adl/adl-edid.cpp \
                     platform/nvidia/nv-edid.cpp
endif

## Raspberry Pi support
if USE_RPI_API
libcec_la_SOURCES += adapter/RPi/RPiCECAdapterDetection.cpp \
                     adapter/RPi/RPiCECAdapterCommunication.cpp \
                     adapter/RPi/RPiCECAdapterMessageQueue.cpp
endif

## CuBox (NXP) support
if USE_TDA995X_API
libcec_la_SOURCES += adapter/TDA995x/TDA995xCECAdapterDetection.cpp \
                     adapter/TDA995x/TDA995xCECAdapterCommunication.cpp
endif


libcec_la_LDFLAGS = @LIBS_LIBCEC@ -version-info @VERSION@
