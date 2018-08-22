#downloaderd

include $(TOPDIR)/rules.mk

PKG_NAME:=downloaderd
PKG_VERSION:=1.0
PKG_RELEASE:=1

PKG_BUILD_DIR := $(BUILD_DIR)/$(PKG_NAME)
include $(INCLUDE_DIR)/package.mk

define Package/downloaderd
	SECTION:=beeba
	CATEGORY:=Beeba
	DEPENDS+= +libjson-c +libubox +libubus +libcurl +libopenssl +libsqlite3 +libblobmsg-json
	VARIANT:=full
	TITLE:=Beeba downloaderd
endef

define Package/downloaderd/description
	Beeba downloaderd
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

EXTRA_CFLAGS+= $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) -I$(STAGING_DIR)/usr/include

EXTRA_LDFLAGS+= $(TARGET_LDFLAGS) -Wl,-rpath-link,$(STAGING_DIR)/usr/lib $(DEP_LIBS)

define Package/downloaderd/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(CP) $(PKG_BUILD_DIR)/down $(1)/usr/bin/downloaderd
	$(INSTALL_BIN) ./files/start_downloaderd.sh $(1)/usr/bin/start_downloaderd.sh
endef

$(eval $(call BuildPackage,downloaderd))
