#!/bin/bash
gcc node.c blockchain.c data_containers/linked_list.c data_containers/dict.c -o node -lcrypto nanomsg/lib/libnanomsg.a -g