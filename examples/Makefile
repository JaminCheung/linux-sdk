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

TOPDIR ?= ..

include $(TOPDIR)/config.mk

LINK_LIBS := -L$(LIBS_OUTDIR) -l$(TARGET_NAME)

define build_example
	$(QUIET_LINK)$(LINK_OBJS) $(LINK_LIBS) -o $(EXAMPLES_OUTDIR)/$(2) $(1) $(LDFLAGS)
endef

define clean_example
	@rm -rf $(1) $(EXAMPLES_OUTDIR)/$(2)
endef


#
# 74HC595 
#
ifeq ($(CONFIG_74HC595_MANAGER), y)
EXAMPLE_74HC595 := test_74hc595
EXAMPLE_74HC595_CLEAN := test_74hc595_clean
EXAMPLE_74HC595_OBJ := 74hc595/main.o
$(EXAMPLE_74HC595): $(EXAMPLE_74HC595_OBJ)
	$(call build_example,$^,$@)
$(EXAMPLE_74HC595_CLEAN):
	$(call clean_example,$(EXAMPLE_74HC595_OBJ),$(EXAMPLE_74HC595))
endif


#
# Alarm 
#
ifeq ($(CONFIG_ALARM_MANAGER), y)
EXAMPLE_ALARM := test_alarm
EXAMPLE_ALARM_CLEAN := test_alarm_clean
EXAMPLE_ALARM_OBJ := alarm/main.o
$(EXAMPLE_ALARM): $(EXAMPLE_ALARM_OBJ)
	$(call build_example,$^,$@)
$(EXAMPLE_ALARM_CLEAN):
	$(call clean_example,$(EXAMPLE_ALARM_OBJ),$(EXAMPLE_ALARM))
endif


#
# Audio alsa
#
ifeq ($(CONFIG_ALSA_AUDIO_PLAYER)_$(CONFIG_ALSA_AUDIO_MIXER)_$(CONFIG_INPUT_MANAGER), y_y_y)
EXAMPLE_ALSA_PLAY := test_wave_play
EXAMPLE_ALSA_PLAY_CLEAN := test_wave_play_clean
EXAMPLE_ALSA_PLAY_OBJ := audio/alsa/test_wave_play.o
$(EXAMPLE_ALSA_PLAY): $(EXAMPLE_ALSA_PLAY_OBJ)
	$(call build_example,$^,$@)
$(EXAMPLE_ALSA_PLAY_CLEAN):
	$(call clean_example,$(EXAMPLE_ALSA_PLAY_OBJ),$(EXAMPLE_ALSA_PLAY))
endif

ifeq ($(CONFIG_ALSA_AUDIO_RECORDER)_$(CONFIG_ALSA_AUDIO_RECORDER)_$(CONFIG_INPUT_MANAGER), y_y_y)
EXAMPLE_ALSA_RECORD := test_wave_record
EXAMPLE_ALSA_RECORD_CLEAN := test_wave_record_clean
EXAMPLE_ALSA_RECORD_OBJ := audio/alsa/test_wave_record.o
$(EXAMPLE_ALSA_RECORD): $(EXAMPLE_ALSA_RECORD_OBJ)
	$(call build_example,$^,$@)
$(EXAMPLE_ALSA_RECORD_CLEAN):
	$(call clean_example,$(EXAMPLE_ALSA_RECORD_OBJ),$(EXAMPLE_ALSA_RECORD))
endif

ifeq ($(CONFIG_ALSA_AUDIO_MIXER), y)
EXAMPLE_ALSA_MIXER := test_mixer
EXAMPLE_ALSA_MIXER_CLEAN := test_mixer_clean
EXAMPLE_ALSA_MIXER_OBJ := audio/alsa/test_mixer.o
$(EXAMPLE_ALSA_MIXER): $(EXAMPLE_ALSA_MIXER_OBJ)
	$(call build_example,$^,$@)
$(EXAMPLE_ALSA_MIXER_CLEAN):
	$(call clean_example,$(EXAMPLE_ALSA_MIXER_OBJ),$(EXAMPLE_ALSA_MIXER))
endif

ifeq ($(CONFIG_ALSA_AUDIO_PLAYER)_$(CONFIG_ALSA_AUDIO_RECORDER)_$(CONFIG_ALSA_AUDIO_MIXER), y_y_y)
EXAMPLE_ALSA_PCM_LOOP := test_pcm_loop
EXAMPLE_ALSA_PCM_LOOP_CLEAN := test_pcm_loop_clean
EXAMPLE_ALSA_PCM_LOOP_OBJ := audio/alsa/test_pcm_loop.o
$(EXAMPLE_ALSA_PCM_LOOP): $(EXAMPLE_ALSA_PCM_LOOP_OBJ)
	$(call build_example,$^,$@)
$(EXAMPLE_ALSA_PCM_LOOP_CLEAN):
	$(call clean_example,$(EXAMPLE_ALSA_PCM_LOOP_OBJ),$(EXAMPLE_ALSA_PCM_LOOP))
endif


#
# Battery
#
ifeq ($(CONFIG_BATTERY_MANAGER), y)
EXAMPLE_BATTERY := test_battery
EXAMPLE_BATTERY_CLEAN := test_battery_clean
EXAMPLE_BATTERY_OBJ := battery/main.o
$(EXAMPLE_BATTERY): $(EXAMPLE_BATTERY_OBJ)
	$(call build_example,$^,$@)
$(EXAMPLE_BATTERY_CLEAN):
	$(call clean_example,$(EXAMPLE_BATTERY_OBJ),$(EXAMPLE_BATTERY))
endif


#
# Camera(char device)
#
ifeq ($(CONFIG_CHAR_CAMERA_MANAGER), y)
EXAMPLE_CAMERA_CHAR := test_camera_char
EXAMPLE_CAMERA_CHAR_CLEAN := test_camera_char_clean
EXAMPLE_CAMERA_CHAR_OBJ := camera/main.o                                       \
                           camera/sensor.o
$(EXAMPLE_CAMERA_CHAR): $(EXAMPLE_CAMERA_CHAR_OBJ)
	$(call build_example,$^,$@)
$(EXAMPLE_CAMERA_CHAR_CLEAN):
	$(call clean_example,$(EXAMPLE_CAMERA_CHAR_OBJ),$(EXAMPLE_CAMERA_CHAR))
endif


#
# Camera(V4L)
#
ifeq ($(CONFIG_V4L2_CAMERA_MANAGER), y)
EXAMPLE_CAMERA_V4L := test_camerav4l2
EXAMPLE_CAMERA_V4L_CLEAN := test_camerav4l_clean
EXAMPLE_CAMERA_V4L_OBJ :=  camera_v4l/main.o
$(EXAMPLE_CAMERA_V4L): $(EXAMPLE_CAMERA_V4L_OBJ)
	$(call build_example,$^,$@)
$(EXAMPLE_CAMERA_V4L_CLEAN):
	$(call clean_example,$(EXAMPLE_CAMERA_V4L_OBJ),$(EXAMPLE_CAMERA_V4L))
endif



#
# Cypress(PSOC4)
#
ifeq ($(CONFIG_CYPRESS_MANAGER), y)
EXAMPLE_CYPRESS := test_cypress
EXAMPLE_CYPRESS_CLEAN := test_cypress_clean
EXAMPLE_CYPRESS_OBJ :=  cypress/main.o
$(EXAMPLE_CYPRESS): $(EXAMPLE_CYPRESS_OBJ)
	$(call build_example,$^,$@)
$(EXAMPLE_CYPRESS_CLEAN):
	$(call clean_example,$(EXAMPLE_CYPRESS_OBJ),$(EXAMPLE_CYPRESS))
endif


#
# EFUSE
#
ifeq ($(CONFIG_EFUSE_MANAGER), y)
EXAMPLE_EFUSE := test_efuse
EXAMPLE_EFUSE_CLEAN := test_efuse_clean
EXAMPLE_EFUSE_OBJ :=  efuse/main.o
$(EXAMPLE_EFUSE): $(EXAMPLE_EFUSE_OBJ)
	$(call build_example,$^,$@)
$(EXAMPLE_EFUSE_CLEAN):
	$(call clean_example,$(EXAMPLE_EFUSE_OBJ),$(EXAMPLE_EFUSE))
endif


#
# Face detect
#
ifeq ($(CONFIG_LIB_FACE_DETECT)_$(CONFIG_GRAPHICS_DRAWER)_$(CONFIG_FRMAEBUFFER_MANAGER)_$(CONFIG_LIB_PNG), y_y_y_y)
EXAMPLE_FACE_DETECT := test_face_detect
EXAMPLE_FACE_DETECT_CLEAN := test_face_detect_clean
EXAMPLE_FACE_DETECT_OBJ :=  face/main.o
$(EXAMPLE_FACE_DETECT): $(EXAMPLE_FACE_DETECT_OBJ)
	$(call build_example,$^,$@)
$(EXAMPLE_FACE_DETECT_CLEAN):
	$(call clean_example,$(EXAMPLE_FACE_DETECT_OBJ),$(EXAMPLE_FACE_DETECT))
endif


#
# Framebuffer
#
ifeq ($(CONFIG_FRMAEBUFFER_MANAGER), y)
EXAMPLE_FB := test_fb
EXAMPLE_FB_CLEAN := test_fb_clean
EXAMPLE_FB_OBJ :=  fb/main.o
$(EXAMPLE_FB): $(EXAMPLE_FB_OBJ)
	$(call build_example,$^,$@)
$(EXAMPLE_FB_CLEAN):
	$(call clean_example,$(EXAMPLE_FB_OBJ),$(EXAMPLE_FB))
endif


#
# Fingerprint
#
ifeq ($(CONFIG_LIB_FINGERPRINT_MA)_$(CONFIG_POWER_MANAGER), y_y)
EXAMPLE_FINGER_MA := test_ma_fp
EXAMPLE_FINGER_MA_CLEAN := test_ma_fp_clean
EXAMPLE_FINGER_MA_OBJ :=  fingerprint/microarray/test_ma.o
$(EXAMPLE_FINGER_MA): $(EXAMPLE_FINGER_MA_OBJ)
	$(call build_example,$^,$@)
$(EXAMPLE_FINGER_MA_CLEAN):
	$(call clean_example,$(EXAMPLE_FINGER_MA_OBJ),$(EXAMPLE_FINGER_MA))
endif

ifeq ($(CONFIG_LIB_FINGERPRINT_GD)_$(CONFIG_POWER_MANAGER), y_y)
EXAMPLE_FINGER_GF := test_gf_fp
EXAMPLE_FINGER_GF_CLEAN := test_gf_fp_clean
EXAMPLE_FINGER_GF_OBJ :=  fingerprint/goodix/test_gf.o
$(EXAMPLE_FINGER_GF): $(EXAMPLE_FINGER_GF_OBJ)
	$(call build_example,$^,$@)
$(EXAMPLE_FINGER_GF_CLEAN):
	$(call clean_example,$(EXAMPLE_FINGER_GF_OBJ),$(EXAMPLE_FINGER_GF))
endif

ifeq ($(CONFIG_LIB_FINGERPRINT_FPC)_$(CONFIG_POWER_MANAGER), y_y)
EXAMPLE_FINGER_FPC := test_fpc_fp
EXAMPLE_FINGER_FPC_CLEAN := test_fpc_fp_clean
EXAMPLE_FINGER_FPC_OBJ :=  fingerprint/fpc/test_fpc.o
$(EXAMPLE_FINGER_FPC): $(EXAMPLE_FINGER_FPC_OBJ)
	$(call build_example,$^,$@)
$(EXAMPLE_FINGER_FPC_CLEAN):
	$(call clean_example,$(EXAMPLE_FINGER_FPC_OBJ),$(EXAMPLE_FINGER_FPC))
endif


#
# MTD flash
#
ifeq ($(CONFIG_MTD_FLASH_MANAGER), y)
EXAMPLE_MTD_FLASH := test_flash
EXAMPLE_MTD_FLASH_CLEAN := test_flash_clean
EXAMPLE_MTD_FLASH_OBJ :=  flash/main.o
$(EXAMPLE_MTD_FLASH): $(EXAMPLE_MTD_FLASH_OBJ)
	$(call build_example,$^,$@)
$(EXAMPLE_MTD_FLASH_CLEAN):
	$(call clean_example,$(EXAMPLE_MTD_FLASH_OBJ),$(EXAMPLE_MTD_FLASH))
endif


#
# GPIO
#
ifeq ($(CONFIG_GPIO_MANAGER), y)
EXAMPLE_GPIO := test_gpio
EXAMPLE_GPIO_CLEAN := test_gpio_clean
EXAMPLE_GPIO_OBJ :=  gpio/main.o
$(EXAMPLE_GPIO): $(EXAMPLE_GPIO_OBJ)
	$(call build_example,$^,$@)
$(EXAMPLE_GPIO_CLEAN):
	$(call clean_example,$(EXAMPLE_GPIO_OBJ),$(EXAMPLE_GPIO))
endif


#
# Graphics
#
ifeq ($(CONFIG_GRAPHICS_DRAWER), y)
EXAMPLE_GRAPHICS := test_graphics
EXAMPLE_GRAPHICS_CLEAN := test_graphics_clean
EXAMPLE_GRAPHICS_OBJ :=  graphics/main.o
$(EXAMPLE_GRAPHICS): $(EXAMPLE_GRAPHICS_OBJ)
	$(call build_example,$^,$@)
$(EXAMPLE_GRAPHICS_CLEAN):
	$(call clean_example,$(EXAMPLE_GRAPHICS_OBJ),$(EXAMPLE_GRAPHICS))
endif


#
# I2C
#
ifeq ($(CONFIG_I2C_MANAGER), y)
EXAMPLE_I2C := test_i2c
EXAMPLE_I2C_CLEAN := test_i2c_clean
EXAMPLE_I2C_OBJ :=  i2c/main.o
$(EXAMPLE_I2C): $(EXAMPLE_I2C_OBJ)
	$(call build_example,$^,$@)
$(EXAMPLE_I2C_CLEAN):
	$(call clean_example,$(EXAMPLE_I2C_OBJ),$(EXAMPLE_I2C))
endif


#
# Input
#
ifeq ($(CONFIG_INPUT_MANAGER), y)
EXAMPLE_INPUT := test_input
EXAMPLE_INPUT_CLEAN := test_input_clean
EXAMPLE_INPUT_OBJ :=  input/main.o
$(EXAMPLE_INPUT): $(EXAMPLE_INPUT_OBJ)
	$(call build_example,$^,$@)
$(EXAMPLE_INPUT_CLEAN):
	$(call clean_example,$(EXAMPLE_INPUT_OBJ),$(EXAMPLE_INPUT))
endif


#
# Mount
#
ifeq ($(CONFIG_MOUNT_MANAGER), y)
EXAMPLE_MOUNT := test_mount
EXAMPLE_MOUNT_CLEAN := test_mount_clean
EXAMPLE_MOUNT_OBJ :=  mount/main.o
$(EXAMPLE_MOUNT): $(EXAMPLE_MOUNT_OBJ)
	$(call build_example,$^,$@)
$(EXAMPLE_MOUNT_CLEAN):
	$(call clean_example,$(EXAMPLE_MOUNT_OBJ),$(EXAMPLE_MOUNT))
endif


#
# Power
#
ifeq ($(CONFIG_POWER_MANAGER), y)
EXAMPLE_POWER := test_power
EXAMPLE_POWER_CLEAN := test_power_clean
EXAMPLE_POWER_OBJ :=  power/main.o
$(EXAMPLE_POWER): $(EXAMPLE_POWER_OBJ)
	$(call build_example,$^,$@)
$(EXAMPLE_POWER_CLEAN):
	$(call clean_example,$(EXAMPLE_POWER_OBJ),$(EXAMPLE_POWER))
endif


#
# PWM
#
ifeq ($(CONFIG_PWM_MANAGER), y)
EXAMPLE_PWM := test_pwm
EXAMPLE_PWM_CLEAN := test_pwm_clean
EXAMPLE_PWM_OBJ :=  pwm/main.o
$(EXAMPLE_PWM): $(EXAMPLE_PWM_OBJ)
	$(call build_example,$^,$@)
$(EXAMPLE_PWM_CLEAN):
	$(call clean_example,$(EXAMPLE_PWM_OBJ),$(EXAMPLE_PWM))
endif


#
# RTC
#
ifeq ($(CONFIG_RTC_MANAGER), y)
EXAMPLE_RTC := test_rtc
EXAMPLE_RTC_CLEAN := test_rtc_clean
EXAMPLE_RTC_OBJ :=  rtc/main.o
$(EXAMPLE_RTC): $(EXAMPLE_RTC_OBJ)
	$(call build_example,$^,$@)
$(EXAMPLE_RTC_CLEAN):
	$(call clean_example,$(EXAMPLE_RTC_OBJ),$(EXAMPLE_RTC))
endif


#
# Security
#
ifeq ($(CONFIG_SECURITY_MANAGER), y)
EXAMPLE_SECURITY := test_security
EXAMPLE_SECURITY_CLEAN := test_security_clean
EXAMPLE_SECURITY_OBJ :=  security/main.o
$(EXAMPLE_SECURITY): $(EXAMPLE_SECURITY_OBJ)
	$(call build_example,$^,$@)
$(EXAMPLE_SECURITY_CLEAN):
	$(call clean_example,$(EXAMPLE_SECURITY_OBJ),$(EXAMPLE_SECURITY))
endif


#
# SPI
#
ifeq ($(CONFIG_SPI_MANAGER), y)
EXAMPLE_SPI := test_spi
EXAMPLE_SPI_CLEAN := test_spi_clean
EXAMPLE_SPI_OBJ :=  spi/main.o
$(EXAMPLE_SPI): $(EXAMPLE_SPI_OBJ)
	$(call build_example,$^,$@)
$(EXAMPLE_SPI_CLEAN):
	$(call clean_example,$(EXAMPLE_SPI_OBJ),$(EXAMPLE_SPI))
endif


#
# UART
#
ifeq ($(CONFIG_UART_MANAGER), y)
EXAMPLE_UART := test_uart
EXAMPLE_UART_CLEAN := test_uart_clean
EXAMPLE_UART_OBJ :=  uart/main.o
$(EXAMPLE_UART): $(EXAMPLE_UART_OBJ)
	$(call build_example,$^,$@)
$(EXAMPLE_UART_CLEAN):
	$(call clean_example,$(EXAMPLE_UART_OBJ),$(EXAMPLE_UART))
endif


#
# USB device
#
ifeq ($(CONFIG_USB_DEVICE_MANAGER), y)
EXAMPLE_USB_HID := test_hid
EXAMPLE_USB_HID_CLEAN := test_hid_clean
EXAMPLE_USB_HID_OBJ :=  usb/test_hid.o
$(EXAMPLE_USB_HID): $(EXAMPLE_USB_HID_OBJ)
	$(call build_example,$^,$@)
$(EXAMPLE_USB_HID_CLEAN):
	$(call clean_example,$(EXAMPLE_USB_HID_OBJ),$(EXAMPLE_USB_HID))

EXAMPLE_USB_CDC := test_cdc_acm
EXAMPLE_USB_CDC_CLEAN := test_cdc_clean
EXAMPLE_USB_CDC_OBJ :=  usb/test_cdc_acm.o
$(EXAMPLE_USB_CDC): $(EXAMPLE_USB_CDC_OBJ)
	$(call build_example,$^,$@)
$(EXAMPLE_USB_CDC_CLEAN):
	$(call clean_example,$(EXAMPLE_USB_CDC_OBJ),$(EXAMPLE_USB_CDC))

EXAMPLE_USB_SWITCH := test_usb_switch
EXAMPLE_USB_SWITCH_CLEAN := test_usb_switch_clean
EXAMPLE_USB_SWITCH_OBJ :=  usb/test_switch.o
$(EXAMPLE_USB_SWITCH): $(EXAMPLE_USB_SWITCH_OBJ)
	$(call build_example,$^,$@)
$(EXAMPLE_USB_SWITCH_CLEAN):
	$(call clean_example,$(EXAMPLE_USB_SWITCH_OBJ),$(EXAMPLE_USB_SWITCH))
endif


#
# Vibrator
#
ifeq ($(CONFIG_VIBRATOR_MANAGER), y)
EXAMPLE_VIBRATOR := test_vibrator
EXAMPLE_VIBRATOR_CLEAN := test_vibrator_clean
EXAMPLE_VIBRATOR_OBJ :=  vibrator/main.o
$(EXAMPLE_VIBRATOR): $(EXAMPLE_VIBRATOR_OBJ)
	$(call build_example,$^,$@)
$(EXAMPLE_VIBRATOR_CLEAN):
	$(call clean_example,$(EXAMPLE_VIBRATOR_OBJ),$(EXAMPLE_VIBRATOR))
endif


#
# Watchdog
#
ifeq ($(CONFIG_WATCHDOG_MANAGER), y)
EXAMPLE_WATCHDOG := test_watchdog
EXAMPLE_WATCHDOG_CLEAN := test_watchdog_clean
EXAMPLE_WATCHDOG_OBJ :=  watchdog/main.o
$(EXAMPLE_WATCHDOG): $(EXAMPLE_WATCHDOG_OBJ)
	$(call build_example,$^,$@)
$(EXAMPLE_WATCHDOG_CLEAN):
	$(call clean_example,$(EXAMPLE_WATCHDOG_OBJ),$(EXAMPLE_WATCHDOG))
endif


#
# Zigbee
#
ifeq ($(CONFIG_ZIGBEE_MANAGER), y)
EXAMPLE_ZIGBEE := test_zigbee
EXAMPLE_ZIGBEE_CLEAN := test_zigbee_clean
EXAMPLE_ZIGBEE_OBJ :=  zigbee/main.o
$(EXAMPLE_ZIGBEE): $(EXAMPLE_ZIGBEE_OBJ)
	$(call build_example,$^,$@)
$(EXAMPLE_ZIGBEE_CLEAN):
	$(call clean_example,$(EXAMPLE_ZIGBEE_OBJ),$(EXAMPLE_ZIGBEE))
endif


#
# Thread
#
EXAMPLE_THREAD := test_thread
EXAMPLE_THREAD_CLEAN := test_thread_clean
EXAMPLE_THREAD_OBJ :=  thread/main.o
$(EXAMPLE_THREAD): $(EXAMPLE_THREAD_OBJ)
	$(call build_example,$^,$@)
$(EXAMPLE_THREAD_CLEAN):
	$(call clean_example,$(EXAMPLE_THREAD_OBJ),$(EXAMPLE_THREAD))

#
# Ring buffer
#
ifeq ($(CONFIG_RING_BUFFER), y)
EXAMPLE_RING_BUFFER := test_ring_buffer
EXAMPLE_RING_BUFFER_CLEAN := test_ring_buffer_clean
EXAMPLE_RING_BUFFER_OBJ :=  ring_buffer/main.o
$(EXAMPLE_RING_BUFFER): $(EXAMPLE_RING_BUFFER_OBJ)
	$(call build_example,$^,$@)
$(EXAMPLE_RING_BUFFER_CLEAN):
	$(call clean_example,$(EXAMPLE_RING_BUFFER_OBJ),$(EXAMPLE_RING_BUFFER))
endif

all:                                                                           \
	$(EXAMPLE_74HC595)                                                         \
	$(EXAMPLE_ALARM)                                                           \
	$(EXAMPLE_ALSA_PLAY)                                                       \
	$(EXAMPLE_ALSA_RECORD)                                                     \
	$(EXAMPLE_ALSA_MIXER)                                                      \
	$(EXAMPLE_ALSA_PCM_LOOP)                                                   \
	$(EXAMPLE_BATTERY)                                                         \
	$(EXAMPLE_CAMERA_CHAR)                                                     \
	$(EXAMPLE_CAMERA_V4L)                                                      \
	$(EXAMPLE_CYPRESS)                                                         \
	$(EXAMPLE_EFUSE)                                                           \
	$(EXAMPLE_FACE_DETECT)                                                     \
	$(EXAMPLE_FB)                                                              \
	$(EXAMPLE_FINGER_MA)                                                       \
	$(EXAMPLE_FINGER_GF)                                                       \
	$(EXAMPLE_FINGER_FPC)                                                      \
	$(EXAMPLE_MTD_FLASH)                                                       \
	$(EXAMPLE_GPIO)                                                            \
	$(EXAMPLE_GRAPHICS)                                                        \
	$(EXAMPLE_I2C)                                                             \
	$(EXAMPLE_INPUT)                                                           \
	$(EXAMPLE_MOUNT)                                                           \
	$(EXAMPLE_POWER)                                                           \
	$(EXAMPLE_PWM)                                                             \
	$(EXAMPLE_RTC)                                                             \
	$(EXAMPLE_SECURITY)                                                        \
	$(EXAMPLE_SPI)                                                             \
	$(EXAMPLE_UART)                                                            \
	$(EXAMPLE_USB_HID)                                                         \
	$(EXAMPLE_USB_CDC)                                                         \
	$(EXAMPLE_USB_SWITCH)                                                      \
	$(EXAMPLE_VIBRATOR)                                                        \
	$(EXAMPLE_ZIGBEE)                                                          \
	$(EXAMPLE_THREAD)                                                          \
	$(EXAMPLE_RING_BUFFER)

clean:                                                                         \
	$(EXAMPLE_74HC595_CLEAN)                                                   \
	$(EXAMPLE_ALARM_CLEAN)                                                     \
	$(EXAMPLE_ALSA_PLAY_CLEAN)                                                 \
	$(EXAMPLE_ALSA_RECORD_CLEAN)                                               \
	$(EXAMPLE_ALSA_MIXER_CLEAN)                                                \
	$(EXAMPLE_ALSA_PCM_LOOP_CLEAN)                                             \
	$(EXAMPLE_BATTERY_CLEAN)                                                   \
	$(EXAMPLE_CAMERA_CHAR_CLEAN)                                               \
	$(EXAMPLE_CAMERA_V4L_CLEAN)                                                \
	$(EXAMPLE_CYPRESS_CLEAN)                                                   \
	$(EXAMPLE_EFUSE_CLEAN)                                                     \
	$(EXAMPLE_FACE_DETECT_CLEAN)                                               \
	$(EXAMPLE_FB_CLEAN)                                                        \
	$(EXAMPLE_FINGER_MA_CLEAN)                                                 \
	$(EXAMPLE_FINGER_GF_CLEAN)                                                 \
	$(EXAMPLE_FINGER_FPC_CLEAN)                                                \
	$(EXAMPLE_MTD_FLASH_CLEAN)                                                 \
	$(EXAMPLE_GPIO_CLEAN)                                                      \
	$(EXAMPLE_GRAPHICS_CLEAN)                                                  \
	$(EXAMPLE_I2C_CLEAN)                                                       \
	$(EXAMPLE_INPUT_CLEAN)                                                     \
	$(EXAMPLE_MOUNT_CLEAN)                                                     \
	$(EXAMPLE_POWER_CLEAN)                                                     \
	$(EXAMPLE_PWM_CLEAN)                                                       \
	$(EXAMPLE_RTC_CLEAN)                                                       \
	$(EXAMPLE_SECURITY_CLEAN)                                                  \
	$(EXAMPLE_SPI_CLEAN)                                                       \
	$(EXAMPLE_UART_CLEAN)                                                      \
	$(EXAMPLE_USB_HID_CLEAN)                                                   \
	$(EXAMPLE_USB_CDC_CLEAN)                                                   \
	$(EXAMPLE_USB_SWITCH_CLEAN)                                                \
	$(EXAMPLE_VIBRATOR_CLEAN)                                                  \
	$(EXAMPLE_ZIGBEE_CLEAN)                                                    \
	$(EXAMPLE_THREAD_CLEAN)                                                    \
	$(EXAMPLE_RING_BUFFER_CLEAN)


