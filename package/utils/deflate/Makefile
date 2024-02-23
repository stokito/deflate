include $(TOPDIR)/rules.mk

PKG_NAME:=deflate
PKG_VERSION:=0.2.0
PKG_RELEASE:=1

#PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE:=v$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=https://github.com/stokito/deflate/archive/
PKG_HASH:=f63e3ddc91f753bb545957c776e1f157aa928be22cf1f24858812fc6ba7aed05
#CMAKE_INSTALL:=1
PKG_INSTALL:=1

PKG_MAINTAINER:=Sergey Ponomarev <stokito@gmail.com>
PKG_LICENSE:=0BSD

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

define Package/deflate
  SECTION:=utils
  CATEGORY:=Utilities
  DEPENDS:=+zlib
  TITLE:=deflate stream utility
  URL:=https://github.com/stokito/deflate
endef

define Package/deflate/description
  A low level gzip utility to create a deflate streams.
endef


define Package/deflate/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/bin/deflate $(1)/usr/bin/
endef

$(eval $(call BuildPackage,deflate))
