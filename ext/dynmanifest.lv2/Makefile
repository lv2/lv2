PREFIX = /usr/local
LV2_DIR = $(PREFIX)/lib/lv2
INCLUDE_DIR = $(PREFIX)/include

all:
	@echo "There's nothing to build here, just 'make install'"
	@echo
	@echo "The variables PREFIX, LV2_DIR, and INCLUDE_DIR are used, e.g.:"
	@echo "make PREFIX=/usr LV2_DIR=/usr/lib/lv2 INCLUDE_DIR=/usr/include"
	@echo
	@echo "Defaults:"
	@echo "    PREFIX      = /usr/local"
	@echo "    LV2_DIR     = PREFIX/lib/lv2"
	@echo "    INCLUDE_DIR = PREFIX/include"

install:
	install -d $(LV2_DIR)/lv2_dyn_manifest.lv2
	install -m 0644 lv2_dyn_manifest.ttl $(LV2_DIR)/lv2_dyn_manifest.lv2/
	install -d $(INCLUDE_DIR)/
	install -m 0644 lv2_dyn_manifest.h $(INCLUDE_DIR)/
