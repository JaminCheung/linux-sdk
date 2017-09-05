 #
 #  Copyright (C) 2017, Zhang YanMing <yanmin.zhang@ingenic.com, jamincheung@126.com>
 #
 #  Ingenic Linux plarform SDK project
 #
 #  This program is free software; you can redistribute it and/or modify it
 #  under  the terms of the GNU General  Public License as published by the
 #  Free Software Foundation;  either version 2 of the License, or (at your
 #  option) any later version.
 #
 #  You should have received a copy of the GNU General Public License along
 #  with this program; if not, write to the Free Software Foundation, Inc.,
 #  675 Mass Ave, Cambridge, MA 02139, USA.
 #
 #


#####################################################
#
# Debug
#
#####################################################
CONFIG_LOCAL_DEBUG=y


#####################################################
#
# Library
#
#####################################################

#
# Fingerprint - Microarray
#
CONFIG_LIB_FINGERPRINT_MA=y

#
# Fingerprint - Goodix
#
CONFIG_LIB_FINGERPRINT_GD=y

#
# Fingerprint - FPC
#
CONFIG_LIB_FINGERPRINT_FPC=y

#
# Face
#
CONFIG_LIB_FACE_DETECT=y

#
# Alsa lib
#
CONFIG_LIB_ALSA=y

#
# Openssl lib
#
CONFIG_LIB_OPENSSL=y

#
# Base64 libray
#
CONFIG_LIB_BASE64=y

#
# MXML lib
#
CONFIG_LIB_MXML=y

#
# ZLIB
#
CONFIG_LIB_ZLIB=y

#
# PNG lib
#
CONFIG_LIB_PNG=y

#
# CRC lib
#
CONFIG_LIB_CRC=y

#
# Serial lib
#
CONFIG_LIB_SERIAL=y

#
# I2C lib
#
CONFIG_LIB_I2C=y

#
# MTD lib
#
CONFIG_LIB_MTD=y

#
# GPIO lib
#
CONFIG_LIB_GPIO=y

#
# Mini-zip lib
#
CONFIG_LIB_MINI_ZIP=y

#####################################################
#
# Manager
#
#####################################################

#
# Ring buffer
#
CONFIG_RING_BUFFER=y

#
# Netlink manager
#
CONFIG_NETLINK_MANAGER=y

#
# Timer manager
#
CONFIG_TIMER_MANAGER=y

#
# Alarm manager
#
CONFIG_ALARM_MANAGER=y

#
# Battery manager
#
CONFIG_BATTERY_MANAGER=y

#
# Uart manager
#
CONFIG_UART_MANAGER=y

#
# Zigbee manager
#
CONFIG_ZIGBEE_MANAGER=y

#
# GPIO manager
#
CONFIG_GPIO_MANAGER=y

#
# Camera manager(char device)
#
CONFIG_CHAR_CAMERA_MANAGER=y

#
# Camera manager(V4L)
#
CONFIG_V4L2_CAMERA_MANAGER=y

#
# PWM Manager
#
CONFIG_PWM_MANAGER=y

#
# Watchdog manager
#
CONFIG_WATCHDOG_MANAGER=y

#
# Power manager
#
CONFIG_POWER_MANAGER=y

#
# I2C manager
#
CONFIG_I2C_MANAGER=y

#
# EFUSE manager
#
CONFIG_EFUSE_MANAGER=y

#
# RTC manager
#
CONFIG_RTC_MANAGER=y

#
# SPI manager
#
CONFIG_SPI_MANAGER=y

#
# 74HC595 manager
#
CONFIG_74HC595_MANAGER=y

#
# Cypress(PSoc4)
#
CONFIG_CYPRESS_MANAGER=y

#
# MTD flash manager
#
CONFIG_MTD_FLASH_MANAGER=y

#
# USB device manager
#
CONFIG_USB_DEVICE_MANAGER=y

#
# Security manager
#
CONFIG_SECURITY_MANAGER=y

#
# Input manager
#
CONFIG_INPUT_MANAGER=y

#
# Vibrator manager
#
CONFIG_VIBRATOR_MANAGER=y

#
# Mount manager
#
CONFIG_MOUNT_MANAGER=y

#
# Framebuffer manager
#
CONFIG_FRMAEBUFFER_MANAGER=y

#
# Graphics drawer
#
CONFIG_GRAPHICS_DRAWER=y

#
# ALSA audio
#
CONFIG_ALSA_AUDIO=y
ifeq ($(CONFIG_ALSA_AUDIO), y)

	# Player
	CONFIG_ALSA_AUDIO_PLAYER=y

	#Recorder
	CONFIG_ALSA_AUDIO_RECORDER=y

	#Mixer
	CONFIG_ALSA_AUDIO_MIXER=y
endif

#
# Mini-zip
#
CONFIG_MINI_ZIP=y

#####################################################
#
# Handle dependency
#
#####################################################
ifeq ($(CONFIG_UART_MANAGER), y)
CONFIG_LIB_SERIAL=y
endif

ifeq ($(CONFIG_ZIGBEE_MANAGER), y)
CONFIG_LIB_SERIAL=y
CONFIG_UART_MANAGER=y
CONFIG_ALARM_MANAGER=y
endif

ifeq ($(CONFIG_GPIO_MANAGER), y)
CONFIG_LIB_GPIO=y
endif

ifeq ($(CONFIG_I2C_MANAGER), y)
CONFIG_LIB_I2C=y
endif

ifeq ($(CONFIG_BATTERY_MANAGER), y)
CONFIG_NETLINK_MANAGER=y
endif

ifeq ($(CONFIG_GRAPHICS_DRAWER), y)
CONFIG_FRMAEBUFFER_MANAGER=y
CONFIG_LIB_PNG=y
endif

ifeq ($(CONFIG_ALSA_AUDIO), y)
CONFIG_LIB_ALSA=y
endif

ifeq ($(CONFIG_MTD_FLASH_MANAGER), y)
CONFIG_LIB_MTD=y
endif

ifeq ($(CONFIG_LIB_PNG), y)
CONFIG_LIB_ZLIB=y
endif

ifeq ($(CONFIG_LIB_FINGERPRINT_FPC), y)
CONFIG_UART_MANAGER=y
CONFIG_ALARM_MANAGER=y
endif

ifeq ($(CONFIG_VIBRATOR_MANAGER), y)
CONFIG_NETLINK_MANAGER=y
endif

ifeq ($(CONFIG_MINI_ZIP), y)
CONFIG_LIB_ZLIB=y
CONFIG_LIB_MINI_ZIP=y
endif