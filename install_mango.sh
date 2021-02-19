#!/usr/bin/bash


generator=""

if [ $(uname) == "Linux" ]; then
  if [ -x $(command -v ninja) ]; then
    generator="-G Ninja"
  fi
fi

rm -rf mango
git clone https://github.com/t0rakka/mango
mkdir -p mango/bld
cd mango/bld
cmake ../build -DCMAKE_BUILD_TYPE=Release $generator
sudo cmake --build . --target install