#!/bin/bash

set -e

docmd() {
    echo ">>> $*"
    "$@"
}

for dep in lx_lib minilib get_opts
do
    echo ""
    if [[ -d $dep ]]; then
        echo "*** $dep already exists"
    else
        echo "*** retrieving dependency: $dep"
        echo ""
        docmd git clone https://github.com/edelkind/$dep
    fi
    echo ""
    echo "*** building dependency: $dep"
    echo ""
    (cd $dep && docmd make clean all)
done
