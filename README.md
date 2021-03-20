# drift
 
To build:
- bootstrap the vcpkg submodule.
- make a symlink in `assets/` called `encrypted/` that points to encrypted assets folder.
- build in cmake with the vcpkg toolchain file.

RelWithDebInfo is treated as a debug mode with optimisations.

Project is only set up right now for MSVC/clang-cl style flags, but it wouldn't be hard to add in translations for g++/clang++ style flags.