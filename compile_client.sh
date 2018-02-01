#!/bin/bash
gcc client.c data_containers/linked_list.c data_containers/dict.c blockchain.c nanomsg/lib/libnanomsg.a -o client -lcrypto -g
