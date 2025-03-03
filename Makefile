MODULE_big = all_hooks
OBJS = all_hooks.o
EXTENSION = all_hooks
DATA = all_hooks--0.1.sql

PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)

