#!/bin/bash

cmake -S . -B build

if cmake --build build/ --target noddy_test
then
  echo "Build successful. Running..."
  ./build/bin/noddy_test
else
  echo "Failed build"
  exit 1
fi
