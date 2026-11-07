# ICU port overlay

This is the ICU port from Microsoft vcpkg registry commit
`9e593bb18ea69cc5095e012465dcd675a822ed0d`, with one Windows/MSVC
configuration change in `portfile.cmake`: set
`ac_cv_path_install=/usr/bin/install`. ICU's configure otherwise selected a
relative `install-sh` path that failed when invoked from build subdirectories
on the Scoop/MSYS2 environment tested. The MSYS2 coreutils install program is
provided by vcpkg's make environment. The port retains its upstream ICU source
hash and version. Remove this overlay after verifying an upstream fix and a
new pinned registry baseline. The accompanying license is the vcpkg port
license; ICU's own distribution terms still apply to the built library.
