#!/bin/bash

cmake -S . -B build
cmake --build build/

if [ $? -eq 0 ]; then
  echo "Build successful. Running..."
  ./build/bin/noddy_gui
else
  echo "Failed build"
  exit 1
fi
