ARDUINO_DIR            = /usr/share/arduino/
ARDMK_DIR              = /usr
AVR_TOOLS_DIR          = /usr
TARGET                 = "Ardccino"
ARDUINO_LIBS           = UTFT
BOARD_TAG              = mega2560
#MCU                    = atmega328p
#F_CPU                  = 16000000
ARDUINO_PORT           = /dev/`dmesg |grep 'FTDI USB Serial Device converter now attached to'|sed 's#^.*attached to ##'`
AVRDUDE_ARD_BAUDRATE   = 115200
AVRDUDE_ARD_PROGRAMMER = arduino
USER_LIB_PATH          = ./libraries/
 
include $(ARDUINO_DIR)/Arduino.mk
