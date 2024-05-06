- cloning repository
```
git clone --recursive-submodules [path]
```
- Update the submodules from dbcppp
```
git submodule update --init --recursive
```

- conan install . --output-folder=build --build=missing

- cd build 
- cmake .. -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Release
- cmake --build . -v