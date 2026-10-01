#!/usr/bin/env bash
set -euo pipefail
em++ src/wasm.cpp -std=c++17 -O2 --no-entry \
  -sMODULARIZE=1 -sEXPORT_NAME=createLifeEngine \
  -sEXPORTED_FUNCTIONS='["_simulate_world"]' \
  -sEXPORTED_RUNTIME_METHODS='["ccall"]' \
  -sALLOW_MEMORY_GROWTH=1 -sDISABLE_EXCEPTION_CATCHING=0 \
  -sENVIRONMENT=web,worker,node -sFILESYSTEM=0 -o docs/engine.js
