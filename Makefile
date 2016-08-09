bin_dir := bin/
obj_dir := obj/
src_dir := src/
msg_src_dir := $(src_dir)msg/
msg_dir := msg/

server := $(bin_dir)aevo
libs := ev pthread rt

CC := gcc
WARNS := -Wall -Wno-implicit-function-declaration
CFLAGS := $(WARNS) -g -std=c11 -D_POSIX_SOURCE -D_POSIX_C_SOURCE=200112L
LFLAGS := $(addprefix -l, $(libs))

PROTO := protoc
PROTOFLAGS := -I=$(msg_dir) --cpp_out=$(msg_src_dir) $(msg_dir)out.proto

source_dir := $(shell find $(src_dir) -type d)
header_files := $(wildcard $(addsuffix *.h, $(source_dir)))
source_files := $(wildcard $(addsuffix *.c, $(source_dir)))
object_files := $(addprefix $(obj_dir), $(notdir $(patsubst %.c, %.o, $(source_files))))

VPATH:= $(source_dir)
.PHONY : all clean prebuild protobuf

all: prebuild protobuf $(server) 

$(obj_dir)%.o: %.c $(header_files) Makefile
	$(CC) -c $(CFLAGS) -o $@ $<

$(server): $(object_files)
	$(CC) $(LFLAGS) -o $@ $^

protobuf:
	$(PROTO) $(PROTOFLAGS)

prebuild:
	@[ -d $(msg_src_dir) ] || mkdir $(msg_src_dir)
	@[ -d $(bin_dir) ] || mkdir $(bin_dir)
	@[ -d $(obj_dir) ] || mkdir $(obj_dir)

clean:
	rm -rf $(server) $(object_files) $(msg_src_dir)
