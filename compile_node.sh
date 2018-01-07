#!/bin/bash
gcc node.c blockchain.c linked_list_string.c -o node -lcrypto nanomsg/lib/libnanomsg.a -g