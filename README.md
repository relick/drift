# drift
 
To build:
- install to your toolchain Bullet and assimp
- run this in abseil-cpp:
```
mkdir build && cd build
cmake .. -DABSL_RUN_TESTS=ON -DABSL_USE_GOOGLETEST_HEAD=ON -DCMAKE_CXX_STANDARD=20
```

Everything else should build automatically if the git submodules are correctly cloned.

RelWithDebInfo is treated as a debug mode with optimisations.

Project is only set up right now for MSVC/clang-cl style flags, but it wouldn't be hard to add in translations for g++/clang++ style flags.