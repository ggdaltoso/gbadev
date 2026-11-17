# Shared settings for every snippet. Each snippet folder is its own ROM:
# its Makefile only includes this file.
# Point LIBBUTANO at your Butano checkout: make LIBBUTANO=/path/to/butano/butano
TARGET      	:=  $(notdir $(CURDIR))
BUILD       	:=  build
LIBBUTANO   	?=  ../../../butano/butano
PYTHON      	:=  python3
SOURCES     	:=  src
# common/include holds the code every snippet can share, like hero_walk.h.
INCLUDES    	:=  include ../common/include
DATA        	:=
# common/graphics holds the art every snippet shares, like the hero.
GRAPHICS    	:=  graphics ../common/graphics
AUDIO       	:=  audio
AUDIOBACKEND	:=  maxmod
AUDIOTOOL		:=
DMGAUDIO    	:=  dmg_audio
DMGAUDIOBACKEND	:=  default
ROMTITLE    	:=  SNIPPET
ROMCODE     	:=  SBTP
USERFLAGS   	?=
USERCXXFLAGS	:=
USERASFLAGS 	:=
USERLDFLAGS 	:=
USERLIBDIRS 	:=
USERLIBS    	:=
DEFAULTLIBS 	:=
STACKTRACE		:=
USERBUILD   	:=
EXTTOOL     	:=

ifndef LIBBUTANOABS
	export LIBBUTANOABS	:=	$(realpath $(LIBBUTANO))
endif

include $(LIBBUTANOABS)/butano.mak
