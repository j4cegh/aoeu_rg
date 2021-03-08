CC := gcc

ODIR_RELEASE = obj_release
ODIR_DEBUG = obj_debug
ODIR_RELEASE_SERV = obj_release_serv
ODIR_DEBUG_SERV = obj_debug_serv

DEFS_RELEASE = -DNDEBUG -DRELEASE -DRGPUB
DEFS_DEBUG =

DEFS_RELEASE_SERV = -DSERV -DNDEBUG -DRELEASE
DEFS_DEBUG_SERV = -DSERV

LIBS := -lSDL2 -lSDL2_ttf -lSDL2_image -lGL -lGLU -lGLEW -lcsfml-system -lcsfml-network -lbass -lbass_fx -lsodium -lm
LIBS_SERV := -lSDL2 -lcsfml-system -lcsfml-network -lsodium -lm

SDIR = ../src
SRCS := $(wildcard $(SDIR)/*.c)
$(info $(SRCS))
SRCS_SERV_FILES := se_utils.c se_logger.c se_timer.c se_list.c rg_online_server.c rg_online_packet.c rg_online_db.c rg_replay.c rg_online_player.c rg_online_replay_list.c
SRCS_SERV := $(patsubst %, $(SDIR)/%, $(SRCS_SERV_FILES))

CFLAGS_RELEASE := -Os -ffast-math
CFLAGS_DEBUG := -g3
CFLAGS_RELEASE_SERV := -Os -ffast-math
CFLAGS_DEBUG_SERV := -g3

LFLAGS_RELEASE := -s
LFLAGS_DEBUG :=
LFLAGS_RELEASE_SERV := -s
LFLAGS_DEBUG_SERV :=

OBJS_RELEASE := $(subst $(SDIR),$(ODIR_RELEASE),$(SRCS:.c=.o))
OBJS_DEBUG := $(subst $(SDIR),$(ODIR_DEBUG),$(SRCS:.c=.o))

OBJS_RELEASE_SERV := $(subst $(SDIR),$(ODIR_RELEASE_SERV),$(SRCS_SERV:.c=.o))
OBJS_DEBUG_SERV := $(subst $(SDIR),$(ODIR_DEBUG_SERV),$(SRCS_SERV:.c=.o))

-include $(subst $(SDIR),$(ODIR_DEBUG),$(SRCS:.c=.d))
-include $(subst $(SDIR),$(ODIR_DEBUG_SERV),$(SRCS_SERV:.c=.d))

RELEASE_EXEC := rb
DEBUG_EXEC := db
RELEASE_EXEC_SERV := rbs
DEBUG_EXEC_SERV := dbs

$(ODIR_RELEASE)/%.o: $(SDIR)/%.c
	$(CC) $(DEFS_RELEASE) -o $@ -MMD -MP -c $< $(CFLAGS_RELEASE)

$(ODIR_DEBUG)/%.o: $(SDIR)/%.c
	$(CC) $(DEFS_DEBUG) -o $@ -MMD -MP -c $< $(CFLAGS_DEBUG)

$(ODIR_RELEASE_SERV)/%.o: $(SDIR)/%.c
	$(CC) $(DEFS_RELEASE_SERV) -o $@ -MMD -MP -c $< $(CFLAGS_RELEASE_SERV)

$(ODIR_DEBUG_SERV)/%.o: $(SDIR)/%.c
	$(CC) $(DEFS_DEBUG_SERV) -o $@ -MMD -MP -c $< $(CFLAGS_DEBUG_SERV)

$(RELEASE_EXEC): create_release_dir $(OBJS_RELEASE) Makefile
	$(CC) -o $@ $(OBJS_RELEASE) $(LFLAGS_RELEASE) $(LIBS)

$(DEBUG_EXEC): create_debug_dir $(OBJS_DEBUG) Makefile
	$(CC) -o $@ $(OBJS_DEBUG) $(LFLAGS_DEBUG) $(LIBS)

$(RELEASE_EXEC_SERV): create_release_dir_serv $(OBJS_RELEASE_SERV) Makefile
	$(CC) -o $@ $(OBJS_RELEASE_SERV) $(LFLAGS_RELEASE_SERV) $(LIBS_SERV)

$(DEBUG_EXEC_SERV): create_debug_dir_serv $(OBJS_DEBUG_SERV) Makefile
	$(CC) -o $@ $(OBJS_DEBUG_SERV) $(LFLAGS_DEBUG_SERV) $(LIBS_SERV)

r: $(RELEASE_EXEC)
d: $(DEBUG_EXEC)
rs: $(RELEASE_EXEC_SERV)
ds: $(DEBUG_EXEC_SERV)

create_release_dir:
	mkdir -p $(ODIR_RELEASE)

create_debug_dir:
	mkdir -p $(ODIR_DEBUG)

create_release_dir_serv:
	mkdir -p $(ODIR_RELEASE_SERV)

create_debug_dir_serv:
	mkdir -p $(ODIR_DEBUG_SERV)

clean:
	rm -rf $(RELEASE_EXEC) $(DEBUG_EXEC) $(ODIR_RELEASE) $(ODIR_DEBUG) $(RELEASE_EXEC_SERV) $(DEBUG_EXEC_SERV) $(ODIR_RELEASE_SERV) $(ODIR_DEBUG_SERV)
