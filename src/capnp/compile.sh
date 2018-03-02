#!/bin/sh
capnp compile -oc++ message_schema.capnp
mv message_schema.capnp.c++ CapnpGame.cpp
mv message_schema.capnp.h CapnpGame.h