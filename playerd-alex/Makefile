#playerd

include $(TOPDIR)/rules.mk

PKG_NAME:=playerd
PKG_VERSION:=1.0
PKG_RELEASE:=1

PKG_BUILD_DIR := $(BUILD_DIR)/$(PKG_NAME)
include $(INCLUDE_DIR)/package.mk

#TARGET_LDFLAGS += -Wl,-rpath-link,$(STAGING_DIR)/usr/lib

define Package/playerd
	SECTION:=beeba
	CATEGORY:=Beeba
	DEPENDS+= +libpthread +libmosquitto +libjson-c +libmpdclient +libuci +libiwinfo
	VARIANT:=full
	TITLE:=Beeba Playerd
endef

define Package/playerd/description
	Beeba playerd
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

EXTRA_CFLAGS+= $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) -I$(STAGING_DIR)/usr/include

GST_LIBS = -lpthread -lm -lmosquitto -ljson-c -liwinfo

EXTRA_LDFLAGS+= $(TARGET_LDFLAGS) -Wl,-rpath-link,$(STAGING_DIR)/usr/lib $(GST_LIBS)

define Package/playerd/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(CP) $(PKG_BUILD_DIR)/playerd $(1)/usr/bin
	$(INSTALL_BIN) ./files/start_playerd.sh $(1)/usr/bin/start_playerd.sh
#	$(INSTALL_DIR) $(1)/etc
#	$(INSTALL_DIR) $(1)/etc/init.d
#	$(INSTALL_BIN) ./files/playerd.init $(1)/etc/init.d/playerd
endef

$(eval $(call BuildPackage,playerd))
