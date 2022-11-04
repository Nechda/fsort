HOST_GCC=g++
GCCPLUGINS_DIR:=$(shell $(TARGET_GCC) -print-file-name=plugin)
CXXFLAGS+=-I$(GCCPLUGINS_DIR)/include -fPIC -fno-rtti -O2


.PHONY: all clean

all: check_env freorder-ipa.so

freorder-ipa.so: freorder-ipa.cpp
	$(HOST_GCC) -shared $(CXXFLAGS) $^ -o $@

clean:
	rm freorder-ipa.so

check_env:
ifndef TARGET_GCC
	echo "TARGET_GCC variable is not set"
	exit 1
endif

