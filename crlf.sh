#!/bin/bash

pushd 3rdparty
find . -type f -print0 | xargs -0 unix2dos
popd

pushd src
find . -type f -print0 | xargs -0 unix2dos
popd

