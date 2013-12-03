#
# OpenWRT Makefile for E-MailRelay.
#
# Install this as "package/emailrelay/Makefile"; then run "makemenu config" 
# and select "Network->emailrelay". Running "make" should then download
# the E-MailRelay source code from sourceforge and build it.
#
# To use a local tarball rather than downloading from sourceforge edit the 
# PKG_SOURCE and PKD_SOURCE_URL values below.
#
# To get a better idea of the build process try this:
#   $ cd <openwrt>
#   $ rm -rf tmp
#	$ scripts/feeds install emailrelay
#	$ make package/emailrelay/compile V=s CONFIG_PACKAGE_emailrelay=y
#
# Note that the "configure" options below are chosen to build a small binary
# so some significant functionality is disabled.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=emailrelay
PKG_VERSION:=1.9
PKG_RELEASE:=1

PKG_SOURCE:=emailrelay-$(PKG_VERSION)-src.tar.gz
PKG_SOURCE_URL:=http://sourceforge.net/projects/emailrelay/files/emailrelay/$(PKG_VERSION)
PKG_CAT:=zcat

PKG_BUILD_DIR:=$(BUILD_DIR)/emailrelay-$(PKG_VERSION)

include $(INCLUDE_DIR)/uclibc++.mk
include $(INCLUDE_DIR)/package.mk

define Package/emailrelay
	SECTION:=net
	CATEGORY:=Network
	TITLE:=SMTP/POP3 store-and-forward server
	URL:=http://emailrelay.sourceforge.net
	DEPENDS:=+uclibcxx
endef

define Package/emailrelay/description
	E-MailRelay is a store-and-forward SMTP relay with POP3 support.
endef

define Build/Configure
	$(call Build/Configure/Default, \
		--disable-gui \
		--without-glob \
		--without-openssl \
		--without-zlib \
		--without-doxygen \
		--without-man2html \
		--without-pam \
		--disable-exec \
		--disable-debug \
		--enable-auth \
		--disable-admin \
		--disable-identity \
		--enable-pop \
		--disable-dns \
		--enable-small-config \
		--enable-small-exceptions \
		--disable-verbose \
	)
endef

define Package/emailrelay/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/src/main/emailrelay $(1)/usr/sbin
endef

$(eval $(call BuildPackage,emailrelay))

