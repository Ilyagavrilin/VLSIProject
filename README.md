# VLSIProject repo

## Build and Debug
Quick start:
```
$> cmake -DCMAKE_BUILD_TYPE={Release, Debug} -B build
$> cmake --build build
```
## Full build rules
```
$> nix-shell or nix develop (if flakes enabled)
$> cmake -DCMAKE_BUILD_TYPE={Release, Debug} -B build
$> cmake --build build
$> ./build/VLSIProject tests/data/tech1.json tests/data/test_new.json
```

