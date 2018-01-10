#!/bin/bash
gcc client.c linked_list_string.c nanomsg/lib/libnanomsg.a -o client -lcrypto