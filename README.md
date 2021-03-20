# drift
 
To build:
- install to your toolchain assimp, Bullet, and FreeType.
- make a symlink in `assets/` called `encrypted/` that points to encrypted assets folder.

Everything else should build automatically if the git submodules are correctly cloned.

RelWithDebInfo is treated as a debug mode with optimisations.

Project is only set up right now for MSVC/clang-cl style flags, but it wouldn't be hard to add in translations for g++/clang++ style flags.