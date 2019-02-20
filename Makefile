include $(TOPDIR)/rules.mk

PKG_NAME:=QFlash
PKG_VERSION:=2015-08-06
PKG_RELEASE=1

PKG_LICENSE:=GPL-2.0
PKG_LICENSE_FILES:=

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)
PKG_MAINTAINER:=John Crispin <blogic@openwrt.org>

include $(INCLUDE_DIR)/package.mk

define Package/QFlash
  SECTION:=Applications
  CATEGORY:=Puppies
  DEPENDS:=
  TITLE:=Generic QCom 4G firmware upgrade
endef

define Package/QFlash/description
  QFlash system for user track & flow track.
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

TARGET_LDFLAGS += 

MAKE_FLAGS += CFLAGS="$(TARGET_CFLAGS)" \
	LDFLAGS="$(TARGET_LDFLAGS)"

define Package/QFlash/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/QFlash $(1)/usr/sbin/QFlash
endef

$(eval $(call BuildPackage,QFlash))

