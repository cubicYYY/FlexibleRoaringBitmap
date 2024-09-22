# Flexible Roaring Bitmap

**Work in Progress.**

*Flexible Roaring Bitmap* is a variation of [Roaring Bitmap](https://github.com/RoaringBitmap/CRoaring), with a flexible interface based on templates.

You can custom the index size, the container size and the underlying word size.

## Components

### Flexible Roaring Bitmap

Out-of-the-box bitmap for handling sparse bit maps.

- Minimizes memory usage.
- Structure: `BinSearchIndex -> Containers`, or `Single container` for small ones.

### Containers

- Bitmap: use bits to represent 0 & 1.
- Array: number indices.
- RLE: Run-Length Encoded array

### Index Layer

- BinSearchIndex
- RBTreeIndex (TODO)

## Usage

```bash
# Clone the repository
git clone https://github.com/cubicYYY/FlexibleRoaring.git
cd ./FlexibleRoaring

# Build examples
mkdir build
cd ./build
cmake ..
make -j
# ctest # if you wish, or even:
# valgrind --leak-check=full ./integration_test

# Run examples
./example
# ...
```