LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_ARM_MODE:= $(APP_ARM_MODE)

LOCAL_SRC_FILES:= \
	pcre_compile.c \
	pcre_chartables.c \
	pcre_config.c \
	pcre_dfa_exec.c \
	pcre_exec.c \
	pcre_fullinfo.c \
	pcre_get.c \
	pcre_globals.c \
	pcre_newline.c \
	pcre_ord2utf8.c \
	pcre_study.c \
	pcre_tables.c \
	pcre_try_flipped.c \
	pcre_ucp_searchfuncs.c \
	pcre_xclass.c

LOCAL_MODULE:= libpcre

LOCAL_C_INCLUDES :=				\
	$(GLIB_TOP)/android-internal		\
	$(GLIB_TOP)/glib/android-internal	\
	$(GLIB_C_INCLUDES)

LOCAL_CFLAGS := \
    -DG_LOG_DOMAIN=\"GLib-GRegex\" \
    -DSUPPORT_UCP \
    -DSUPPORT_UTF8 \
    -DNEWLINE=-1 \
    -DMATCH_LIMIT=10000000 \
    -DMATCH_LIMIT_RECURSION=10000000 \
    -DMAX_NAME_SIZE=32 \
    -DMAX_NAME_COUNT=10000 \
    -DMAX_DUPLENGTH=30000 \
    -DLINK_SIZE=2 \
    -DPCRE_STATIC \
    -DPOSIX_MALLOC_THRESHOLD=10
 

#LIMBO
LOCAL_CFLAGS += $(ARCH_CFLAGS)
#FIXME: Need to find out why this is failing
LOCAL_CFLAGS += -include $(FIXUTILS_MEM) -include $(LOGUTILS)
LOCAL_STATIC_LIBRARIES += liblimbocompat
LOCAL_ARM_MODE := $(ARM_MODE)

include $(BUILD_STATIC_LIBRARY)
