#!/bin/bash

MODE=$1
FILE=$2
VOLUME=$3
REPEATS=$4

# Protect this script from bad invocations
if [ -z "$REPEATS" ]; then REPEATS=1; fi
if [ -z "$VOLUME" ]; then VOLUME=1; fi

if [ $MODE == "p" ]; then
  mplayer -ao alsa -loop $REPEATS -volume $VOLUME $FILE >& /dev/null
fi

if [ $MODE == "s" ]; then
  killall mplayer >& /dev/null
fi



