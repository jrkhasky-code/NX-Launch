# Ensure devkitPro environment mappings are fetched automatically
ifeq ($(strip $(DEVKITPRO)),)
$(error "Please set DEVKITPRO in your system variables.")
endif

include $(DEVKITPRO)/libnx/switch_rules

# Setup project framework paths
TARGET      :=  Launcher-NX
BUILD       :=  build
SOURCES     :=  source
DATA        :=  data
INCLUDES    :=  include

# Homebrew grid interface metadata properties
APP_TITLE   :=  Launcher-NX
APP_AUTHOR  :=  Homebrew Dev
APP_VERSION :=  1.0.0

include $(DEVKITPRO)/devkitA64/base_rules

