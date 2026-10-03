#!/bin/sh
# Copyright (c) 2011 The Bitcoin developers
# Distributed under the MIT/X11 software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

if [ -e "$1" ]; then
    rm -f "$1"
fi

# Note: this file is not a "real" header file, it is meant to be included
# once per build by the Makefile.
cat > "$1" <<EOF
#ifndef BITCOIN_BUILD_H
#define BITCOIN_BUILD_H

#define BUILD_DATE "$(date -u +'%Y-%m-%d %H:%M:%S UTC')"
#define BUILD_TIME $(date +%s)
EOF

if [ -d .git ]; then
    gitrev=$(git log --format=%h -1 2>/dev/null)
    if [ -n "$gitrev" ]; then
        cat >> "$1" <<EOF
#define GIT_COMMIT_HASH "$gitrev"
EOF
    fi
fi

cat >> "$1" <<EOF
#endif // BITCOIN_BUILD_H
EOF
