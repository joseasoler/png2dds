# png2dds

[![License: MPL 2.0](https://img.shields.io/badge/License-MPL%202.0-brightgreen.svg)](https://opensource.org/licenses/MPL-2.0) [![Contributor Covenant](https://img.shields.io/badge/Contributor%20Covenant-2.1-4baaaa.svg)](CODE_OF_CONDUCT.md)

A CPU-based DDS encoder optimized for fast batch conversions with high encoding quality.

### Performance

png2dds is really fast when encoding multiple input files at once, without compromising on encoding quality. It also provides great performance when processing single files, but it may fall behind other implementations in some cases.

When using high-end CPU processors or encoding large amounts of relatively small files, png2dds can compete against or even beat GPU encoding implementations.

You can find detailed reports and benchmark results here: https://github.com/joseasoler/png2dds/wiki/Performance

### Supported encoding formats

BC7 support is implemented using the [bc7e.ispc](https://github.com/richgel999/bc7enc_rdo) library. BC1 support has been implemented with the [rgbcx](https://github.com/richgel999/bc7enc_rdo) library.

As seen in the [Texture Compression in 2020](https://aras-p.info/blog/2020/12/08/Texture-Compression-in-2020/) blog post, these libraries provide great encoding quality and performance.

png2dds also supports a mixed mode called BC1_ALPHA_BC7. When this mode is enabled, PNG files with alpha values will be converted to BC7 while files without alpha will be converted to BC1.

## Building

Compiling png2dds requires a recent C++ compiler version, the [Intel® Implicit SPMD Program Compiler](https://github.com/ispc/ispc) and [CMake](https://cmake.org/).

### CMake options
```
PNG2DDS_CLANG_TIDY -> Analyze the project with clang-tidy. Off by default.
PNG2DDS_CPP_WARNINGS_AS_ERRORS -> Treat warnings as errors. On by default
PNG2DDS_UNIT_TESTS -> Build png2dds unit tests. Off by default.
```

### Dependencies

To compile png2dds, the following dependencies must be available as development libraries.

* [Boost.Filesystem](https://www.boost.org/doc/libs/master/libs/filesystem/doc/index.htm)
* [Boost.NoWide](https://www.boost.org/doc/libs/master/libs/nowide/doc/html/index.html)
* [Boost.String](https://www.boost.org/doc/libs/1_79_0/doc/html/string_algo.html)
* [Catch2](https://github.com/catchorg/Catch2): Only required if PNG2DDS_UNIT_TESTS is on
* [Hyperscan](https://www.hyperscan.io)
* [fmt](https://fmt.dev/latest/index.html)
* [oneTBB](https://github.com/oneapi-src/oneTBB)

The following third party library dependencies are contained as source code in the thirdparty folder of the png2dds repository. Each one of these libraries is under its own license which is also included in the repository.

* [bc7enc_rdo](https://github.com/richgel999/bc7enc_rdo)
* [libspng](https://libspng.org/)
* [miniz](https://github.com/richgel999/miniz)

## Contributing

png2dds encourages community involvement and contributions. Check the [CONTRIBUTING.md](CONTRIBUTING.md) and [CODE_OF_CONDUCT.md](CODE_OF_CONDUCT.md) files for details. You can check all contributors in the [contributors list](https://github.com/joseasoler/png2dds/graphs/contributors).

You should enable the PNG2DDS_CLANG_TIDY option to ensure that your contribution will pass static analysis.

## License

png2dds is licensed under the Mozilla Public License, v. 2.0. See the [LICENSE](LICENSE) file for details. Check the [MPL 2.0 FAQ](https://www.mozilla.org/en-US/MPL/2.0/FAQ/) to learn more.

## Acknowledgements

Read the [ACKNOWLEDGEMENTS.md](ACKNOWLEDGEMENTS.md) file for details.
