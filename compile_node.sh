#!/bin/bash
gcc node.c blockchain.c data_containers/linked_list.c -o node -lcrypto nanomsg/lib/libnanomsg.a -g