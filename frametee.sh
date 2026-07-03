#!/bin/sh
export ASAN_OPTIONS=detect_odr_violation=0
export LSAN_OPTIONS=suppressions=/home/teero/software/frametee_c/build/asan_suppressions.txt
exec /home/teero/software/frametee/build/frametee "$@"
