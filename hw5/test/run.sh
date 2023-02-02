#!/bin/sh
#audio/x-raw,format=S16LE,channels=1,rate=4800
GST_DEBUG=3,myelement:5 GST_PLUGIN_PATH=../build gst-launch-1.0 myelement location=test.wav ! audio/x-raw,format=S16LE,channels=1,rate=48000 ! autoaudiosink

