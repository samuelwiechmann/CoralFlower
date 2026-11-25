#!/bin/bash
set -eu
# FM1VST adapter revision c6f8de7b233ef4ef0343e4421083a8286ce57768
# Generate assets first with tools/build.py's generate() function.
cd "$(dirname "$0")/.."
v=$(realpath "${FM1VST:?set FM1VST to the adapter checkout}")
build=build/remote
mkdir -p "$build"
id=coralflower_expressive
version=${VERSION:-1.2.0}
revision=${SOURCE_COMMIT:-$(git rev-parse HEAD)}
emcc -O2 -std=gnu11 -w -mcpu=mvp -msign-ext -mnontrapping-fptoint -mbulk-memory -mmutable-globals \
    -DFM1CORE_RFM1=1 -DFM1CORE_ID="\"$id\"" -DFM1CORE_VERSION="\"$version\"" -DFM1CORE_FAMILY='"choralroot"' \
    -DFM1CORE_COMMIT="\"$revision\"" \
    -I. -Ibuild/gen -Ifirmware/src -Itools/emu -I"$v/core-api" -I"$v/core-glue/choralroot" \
    -I"$v/core-glue/common" -I"$v/core-glue/common/wasm_compat" \
    remote/core.c remote/descriptor.c "$v/core-glue/common/rfm1_shim.c" \
    "$v/core-glue/common/nor.c" "$v/core-glue/common/fifo.c" \
    --no-entry -sSTANDALONE_WASM -sFILESYSTEM=0 -sSTACK_SIZE=1048576 -sINITIAL_MEMORY=8388608 \
    -sALLOW_MEMORY_GROWTH=0 -sEXPORTED_FUNCTIONS=[] -o "$build/core.wasm"
printf '{"id":"%s","name":"CoralFlower Expressive","version":"%s","license":"GPL-3.0-only","source_url":"https://github.com/samuelwiechmann/CoralFlower@%s"}' "$id" "$version" "$revision" > "$build/descriptor.json"
licenses=(--license-file "GPL-3.0-only=LICENSE")
for f in LICENSES/*.txt; do licenses+=(--license-file "$(basename "$f" .txt)=$f"); done
out="$build/$id-$version.rfm1"
python3 "$v/tools/rfm1_pack.py" --wasm "$build/core.wasm" --describe "$build/descriptor.json" --out "$out" "${licenses[@]}" --credits-file LICENSING.md
python3 "$v/tools/rfm1_check.py" "$out"
