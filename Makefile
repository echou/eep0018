ERL_TOP = $(shell pwd)

PRIV_DIR = $(ERL_TOP)/priv
EBIN_DIR = $(ERL_TOP)/ebin

OBJ_DIR = $(PRIV_DIR)/obj

LIB_DIR = $(PRIV_DIR)/lib

SRC_DIR = $(ERL_TOP)/src

ERLENV = $(shell env escript $(ERL_TOP)/make/erlenv.escript)
ERL_ROOT_DIR = $(word 1,$(ERLENV))
ERL_EI_DIR = $(word 2,$(ERLENV))

CC = gcc
LD = gcc
CFLAGS = -g -O2 -D_REENTRANT -DUSE_THREADS -D_GNU_SOURCE -fPIC 
LDFLAGS = -shared

ALL_CFLAGS = -I . -I $(ERL_ROOT_DIR)/usr/include -I $(ERL_EI_DIR)/include
LDLIBS = -L $(ERL_EI_DIR)/lib -L $(ERL_ROOT_DIR)/usr/lib -lei -lerl_interface

# ----------------------------------------------------
#	Erlang language section
# ----------------------------------------------------
EMULATOR = beam
ERL_COMPILE_FLAGS += +debug_info 


.SUFFIXES: .erl .beam 
.PRECIOUS: %.erl

ERLC_WFLAGS = -W
ERLC = erlc $(ERLC_WFLAGS) $(ERLC_FLAGS)
ERL = erl -boot start_clean

EEP0018_OBJS = $(OBJ_DIR)/decode_json.o \
			 $(OBJ_DIR)/eep0018.o \
			 $(OBJ_DIR)/encode_json.o \
			 $(OBJ_DIR)/term_buf.o \
			 $(OBJ_DIR)/yajl_alloc.o \
			 $(OBJ_DIR)/yajl_buf.o \
			 $(OBJ_DIR)/yajl.o \
			 $(OBJ_DIR)/yajl_encode.o \
			 $(OBJ_DIR)/yajl_gen.o \
			 $(OBJ_DIR)/yajl_lex.o \
			 $(OBJ_DIR)/yajl_parser.o

DYN_DRIVER = $(LIB_DIR)/eep0018_drv.so


all: $(OBJ_DIR) $(LIB_DIR) $(EBIN_DIR) $(DYN_DRIVER) $(EBIN_DIR)/eep0018.beam

$(OBJ_DIR):
	-@mkdir -p $(OBJ_DIR)

$(LIB_DIR):
	-@mkdir -p $(LIB_DIR)

$(EBIN_DIR):
	-@mkdir -p $(EBIN_DIR)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	-$(INSTALL_DIR) $(OBJ_DIR)
	$(CC) -c -o $@ $(CFLAGS) $(ALL_CFLAGS) $<

$(LIB_DIR)/eep0018_drv.so: $(EEP0018_OBJS) 
	-$(INSTALL_DIR) $(LIB_DIR)
	$(LD) $(LDFLAGS) -o $@ $(EEP0018_OBJS) $(LDLIBS)

$(EBIN_DIR)/%.beam: $(SRC_DIR)/%.erl Makefile
	$(ERLC) $(ERL_COMPILE_FLAGS) -o $(EBIN_DIR) $<

clean:
	-@rm -rf $(LIB_DIR) $(OBJ_DIR) $(EBIN_DIR)
