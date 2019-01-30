#!/bin/bash

pushd 3rdparty
find ./ -iname '*.c' -type f -exec bash -c 'expand -t 4 "$0" | sponge "$0"' {} \;
find ./ -iname '*.cpp' -type f -exec bash -c 'expand -t 4 "$0" | sponge "$0"' {} \;
find ./ -iname '*.h*' -type f -exec bash -c 'expand -t 4 "$0" | sponge "$0"' {} \;
find ./ -iname '*.inc' -type f -exec bash -c 'expand -t 4 "$0" | sponge "$0"' {} \;
find ./ -iname '*.txt' -type f -exec bash -c 'expand -t 4 "$0" | sponge "$0"' {} \;
find ./ -iname '*.vsh' -type f -exec bash -c 'expand -t 4 "$0" | sponge "$0"' {} \;
popd

pushd src
find ./ -iname '*.c' -type f -exec bash -c 'expand -t 4 "$0" | sponge "$0"' {} \;
find ./ -iname '*.cpp' -type f -exec bash -c 'expand -t 4 "$0" | sponge "$0"' {} \;
find ./ -iname '*.h*' -type f -exec bash -c 'expand -t 4 "$0" | sponge "$0"' {} \;
find ./ -iname '*.inc' -type f -exec bash -c 'expand -t 4 "$0" | sponge "$0"' {} \;
find ./ -iname '*.txt' -type f -exec bash -c 'expand -t 4 "$0" | sponge "$0"' {} \;
find ./ -iname '*.vsh' -type f -exec bash -c 'expand -t 4 "$0" | sponge "$0"' {} \;
popd

