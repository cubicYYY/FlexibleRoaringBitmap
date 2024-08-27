# Flexible Roaring Bitmap

*Flexible Roaring Bitmap* is a variation of [Roaring Bitmap](), with a flexible interface based on templates.

You can custom the index size, the container size and the underlying word size.

It is optimized for small containers: if only a single container is needed, no arrays will be used.

## Usage

```bash
# Clone the repository
git clone ...
cd ./FlexibleRoaringBitmap

# Build examples
mkdir build
cd ./build
cmake ..
make -j
# ctest # if you wish

# Run examples
./example
# ...
```

**Work in Progress.**