#!/bin/bash
gcc client.c data_conatiners/linked_list_string.c nanomsg/lib/libnanomsg.a -o client -lcrypto -g