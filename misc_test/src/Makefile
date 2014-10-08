# CURDIR
SRCDIR := $(CURDIR)

# abspath: returns an absolute name that does not contain any 
# . or .. components, nor any repeated path separators (/). 
# does not resolve symlinks and does not require file to exist
SRCDIR := $(abspath $(SRCDIR))

#
# Set distcc related information
#
ifeq ($(DISTCC_DISABLE),)
  export CCACHE_PREFIX=distcc
  ifeq ($(DISTCC_HOSTS),)
    export DISTCC_HOSTS = localhost
  endif
endif

# Set the Makefile binaries
CMAKE := /usr/bin/cmake
MAKE  := /usr/bin/make

# Set Temporary Path we use for builds
TMP_BUILD_PATH ?= /home/$(USER)/tmp

# Allows user to custom edit and tweak the build defaults.
# wildcard: Matches argument pattern including wildcard characters.
ifneq ($(wildcard $(SRCDIR)/Makefile.custom),)
  include $(SRCDIR)/Makefile.custom
endif

# Var ?= value: Setting of a variable only if it doesn't have a value
BUILD_TESTS ?= 1
CMAKE_ARGS += -D CMAKE_CUSTOM_UNIT_TESTS:BOOLEAN=BUILD_TESTS

BUILD_TYPE ?= Debug
CMAKE_ARGS += -D CMAKE_BUILD_TYPE:STRING="$(BUILD_TYPE)"

BDIR := $(SRCDIR)/../builds
BDIR := $(abspath $(BDIR))

# Have separate build directory for every git branch
GIT_BRANCH := $(notdir $(shell git symbolic-ref HEAD))
GIT_SHA_HASH := $(shell git rev-parse HEAD)

# Extracts all but the directory-part of each file name in names
BDIR_NAME := build-$(GIT_BRANCH)-$(BUILD_TYPE)

# Create a directory based on TMP_BUILD_PATH
# If that fails then create a directory locally
BP_DIR := $(shell                                                           \
            if /bin/mkdir -p $(TMP_BUILD_PATH)$(BDIR) 2>/dev/null ;then     \
              echo $(TMP_BUILD_PATH)$(BDIR)                                 \
            ;else                                                           \
              /bin/mkdir -p $(BDIR)                                         \
              echo $(BDIR)                                                  \
            ;fi)
BP_DIR := $(abspath $(BP_DIR))

BUILD_DIR := $(BP_DIR)/$(BDIR_NAME)

# Compute the build version.
export BUILD_VERSION := $(BUILD_TYPE)-$(GIT_BRANCH)-$(GIT_SHA_HASH)

# Figure out the date when the last commit was made in the build.
export BUILD_LAST_COMMIT_DATE := $(shell git log -1 --pretty=format:'%ci')

# Any ccache output goes under BUILD_DIR so that
# we have different ccache output for every git branch
export CCACHE_DIR := $(BUILD_DIR)/.ccache

# Set the path to be used while doing the real work.
CUSTOM_PATH ?= $(PATH)

#------------------------------------------------------------------------
current: prepare_build_dir
	$(MAKE) -C $(BUILD_DIR)

test: prepare_build_dir
	$(MAKE) -C $(BUILD_DIR) test

allclean: 
	echo Removing $(BP_DIR)
	/bin/rm -rf $(BP_DIR)

localclean:
	echo Removing $(BUILD_DIR)
	/bin/rm -rf $(BUILD_DIR)

prepare_build_dir: emit_build_info
	(cd $(BUILD_DIR); $(CMAKE) $(CMAKE_ARGS) $(SRCDIR))

emit_build_info: create_build_dir
	echo \"$(BUILD_VERSION)\" > $(BUILD_DIR)/build_version.txt.tmp
	if /usr/bin/diff -Nq $(BUILD_DIR)/build_version.txt                 \
	     $(BUILD_DIR)/build_version.txt.tmp > /dev/null 2>&1 ;then      \
          /bin/rm -f $(BUILD_DIR)/build_version.txt.tmp                     \
        ;else                                                               \
          /bin/mv $(BUILD_DIR)/build_version.txt.tmp                        \
            $(BUILD_DIR)/build_version.txt                                  \
        ;fi
	echo \"$(BUILD_LAST_COMMIT_DATE)\" >                                \
          $(BUILD_DIR)/build_last_commit_date.txt.tmp
	if /usr/bin/diff -Nq $(BUILD_DIR)/build_last_commit_date.txt        \
	       $(BUILD_DIR)/build_last_commit_date.txt.tmp >                \
               /dev/null 2>&1 ;then                                         \
          /bin/rm -f $(BUILD_DIR)/build_last_commit_date.txt.tmp            \
        ;else                                                               \
          /bin/mv $(BUILD_DIR)/build_last_commit_date.txt.tmp               \
            $(BUILD_DIR)/build_last_commit_date.txt                         \
        ;fi

create_build_dir:
	/bin/mkdir -p $(BUILD_DIR)

.PHONY: current test heapcheck allclean localclean                          \
        prepare_build_dir emit_build_info create_build_dir
