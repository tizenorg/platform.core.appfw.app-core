CC ?= gcc

TARGETS =  \
	  utc_ApplicationFW_appcore_efl_main_func \
	  utc_ApplicationFW_appcore_init_func \
	  utc_ApplicationFW_appcore_exit_func \
	  utc_ApplicationFW_appcore_set_rotation_cb_func \
	  utc_ApplicationFW_appcore_unset_rotation_cb_func \
	  utc_ApplicationFW_appcore_get_rotation_state_func \
	  utc_ApplicationFW_appcore_set_i18n_func \
	  utc_ApplicationFW_appcore_measure_start_func \
	  utc_ApplicationFW_appcore_measure_time_func \
	  utc_ApplicationFW_appcore_measure_time_from_func \
	  utc_ApplicationFW_appcore_set_event_callback_func

PKGS = appcore-efl

LDFLAGS = `pkg-config --libs $(PKGS)`
LDFLAGS += $(TET_ROOT)/lib/tet3/tcm_s.o
LDFLAGS += -L$(TET_ROOT)/lib/tet3 -ltcm_s
LDFLAGS += -L$(TET_ROOT)/lib/tet3 -lapi_s

CFLAGS = -I. `pkg-config --cflags $(PKGS)`
CFLAGS += -I$(TET_ROOT)/inc/tet3
CFLAGS += -Wall

all: $(TARGETS)

$(TARGETS): %: %.c
	$(CC) -o $@ $< $(CFLAGS) $(LDFLAGS)

clean:
	rm -f $(TARGETS)
