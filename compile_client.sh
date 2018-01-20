#!/bin/bash
gcc client.c data_containers/linked_list.c nanomsg/lib/libnanomsg.a -o client -lcrypto -g