# The profiler only exists when this flag is on, so a release build pays nothing for it.
USERFLAGS := -DBN_CFG_PROFILER_ENABLED=true

include $(dir $(lastword $(MAKEFILE_LIST)))../butano.mk
