# abspath: returns an absolute name that does not contain any 
# . or .. components, nor any repeated path separators (/). 
# does not resolve symlinks and does not require file to exist
SRCDIR := $(abspath $(SRCDIR))

# Include temporary changes allowing modification of
# default flags or new targets for build
# Note Makefile.tmp is git ignored and not source controlled
ifneq ($(wildcard $(SRCDIR)/Makefile.tmp),)
  #
  # Customize Examples:
  # 1. Set make variables to non-default values
  # 2. Define new targets or arguments
  # 3. Anything that fancies you...
  #
  # BUILD_TESTS     := 0
  # BUILD_TYPE      := Debug
  # VERBOSE         := 1
  # BUILD_DOCS      := 0
  # DASH_SCRIPTS    := 0
  # TMP_BUILD_PATH  := 
  include $(SRCDIR)/Makefile.tmp
endif

include $(SRCDIR)/Makefile.args

include $(SRCDIR)/Makefile.targets
