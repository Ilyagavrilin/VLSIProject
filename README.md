# VLSIProject repo

## Build and Debug
Quick start:
```
$> git clone https://github.com/Ilyagavrilin/VLSIProject.git
$> cmake -B build && cmake --build build
$> ./build/VLSIProject tests/data/tech1.json tests/data/test_new.json
```
## Full build rules
```
$> nix-shell / nix develop (if flakes enabled)
$> cmake -DCMAKE_BUILD_TYPE={Release, Debug} -B build
$> cmake --build build
$> ./build/VLSIProject tests/data/tech1.json tests/data/test_new.json
```

