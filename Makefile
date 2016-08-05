bin_dir := bin/
obj_dir := obj/
src_dir := src/ 

server := $(bin_dir)aevo
libs := ev pthread

CC := gcc
CFLAGS := -Wall -g -std=c11 -D_POSIX_SOURCE
LFLAGS := $(addprefix -l, $(libs))

header_files := $(wildcard $(addsuffix *.h, $(src_dir)))
source_files := $(wildcard $(addsuffix *.c, $(src_dir)))
object_files := $(addprefix $(obj_dir), $(notdir $(patsubst %.c, %.o, $(source_files))))

VPATH:= $(src_dir)
.PHONY : all clean prebuild

all: prebuild $(server) 

$(obj_dir)%.o: %.c $(header_files) Makefile
	$(CC) -c $(CFLAGS) -o $@ $<

$(server): $(object_files)
	$(CC) $(LFLAGS) -o $@ $^

prebuild:
	@[ -d $(bin_dir) ] || mkdir $(bin_dir)
	@[ -d $(obj_dir) ] || mkdir $(obj_dir)

clean:
	rm -f $(server) $(object_files)
