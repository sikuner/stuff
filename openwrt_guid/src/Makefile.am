
bin_PROGRAMS =

if GUI_CLI_SUPPORT
bin_PROGRAMS += gui_cli
endif

if GUID_SUPPORT
bin_PROGRAMS += guid force_poweroff
endif

common_SOURCES = \
	gui/display/disp_manager.c \
	gui/encoding/encoding_manager.c \
	gui/encoding/utf-8.c \
	gui/fonts/fonts_manager.c \
	gui/fonts/freetype.c \
	gui/widget/widget.c \
	utils/log.c 
	
common_CFLAGS = -g -Wall -I../include -I./gui/include
common_LDADD = -lm -lfreetype -lubox

if SDL_SUPPORT
common_SOURCES += \
	gui/display/sdl.c
common_LDADD +=  -lSDL 
common_CFLAGS += -DSDL_SUPPORT=1 -I/usr/include/freetype2
else
common_SOURCES += \
	utils/volume.c \
	utils/battery.c \
	gui/display/oled.c \
	gui/display/oled_api.c
common_LDADD +=  -liwinfo
common_CFLAGS += -DSDL_SUPPORT=0
endif

gui_cli_CFLAGS = $(common_CFLAGS) -DGUI_CLI_SUPPORT=1
gui_cli_LDADD = $(common_LDADD)

gui_cli_SOURCES= $(common_SOURCES) gui_cli.c 

guid_CFLAGS = $(common_CFLAGS)
guid_LDADD = $(common_LDADD) -ljson-c -lpthread -lubus -lubox -lblobmsg_json -lmpdclient -lasound -lmp3lame

guid_SOURCES = $(common_SOURCES) main.c \
	application.c	\
	input/button.c \
	input/input_manager.c \
	ubus/server.c \
	ubus/srv_proc.c \
	utils/mpdclient.c \
	utils/uri_parse.c \
	utils/common.c \
	net/network.c \
	view/linein_view.c \
	view/airplay_view.c \
	view/upgrade_view.c \
	view/power_view.c \
	view/view_manager.c \
	view/download_view.c \
	view/ab_view.c \
	view/channel_view.c \
	view/volume_view.c \
	view/record_view.c \
	view/set_view.c \
	view/voice_view.c \
	view/lock_view.c \
	view/light_view.c \
	view/bluetooth_view.c \
	view/play_view.c 

force_poweroff_CFLAGS = $(common_CFLAGS)
force_poweroff_LDADD = -lubox
force_poweroff_SOURCES= force_poweroff.c utils/log.c
