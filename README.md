# Memory Allocator in C

An academic implementation of `malloc`, `free`, `calloc` and `realloc`, built as a shared library for Linux. The allocator obtains memory with `mmap` and manages blocks through metadata stored in a doubly linked list.

This project explores dynamic memory management, pointer arithmetic, alignment, block reuse and fragmentation.

## Features

- First-fit search for a free block large enough to satisfy a request.
- Allocation sizes rounded up to multiples of 16 bytes.
- Page-rounded memory mappings obtained with `mmap`.
- Splitting of oversized blocks when enough space remains for metadata and at least 16 bytes of payload.
- Coalescing of neighboring free blocks when they are physically contiguous and adjacent in the list.
- Release of isolated free blocks through `munmap`.
- Zero-initialized allocations and multiplication overflow detection in `calloc`.
- Data preservation when growing or shrinking an allocation with `realloc`.
- Shared-library loading through `LD_PRELOAD`.

## How it works

Each block starts with a metadata structure containing its allocation state, payload size, and pointers to the previous and next blocks.

```text
Block A                                      Block B
+---------------------+------------------+   +---------------------+------------------+
| Metadata            | Payload          |   | Metadata            | Payload          |
| state, size         | returned pointer |   | state, size         |                  |
| previous, next      | points here      |   | previous, next      |                  |
+---------------------+------------------+   +---------------------+------------------+
```

### Allocation

1. Reject requests larger than `SIZE_MAX / 2` and round the requested size up to a multiple of 16.
2. Traverse the list and select the first suitable free block.
3. If no suitable block exists, create a page-rounded mapping with `mmap` and append it to the list.
4. Split the selected block if the remainder can hold another metadata structure and at least 16 bytes of payload.
5. Mark the block as allocated and return the address immediately after its metadata.

### Deallocation

`free` marks the block as available, merges it with eligible free neighbors, and attempts to release it with `munmap` when neither list neighbor is physically contiguous with it.

### Reallocation

Shrinking keeps the same pointer and splits the block when possible. Growing allocates a new block, copies the previous payload and frees the old block. If the new allocation fails, the original allocation remains valid.

## Requirements

- Linux, using the 64-bit environment targeted by this implementation.
- GCC, including support for `__builtin_mul_overflow`.
- GNU Make.
- Bash, GNU core utilities and `find` for the test script.
- Development headers and libraries required by the Makefile, including pthread linkage.

The code uses Linux memory-mapping interfaces and GCC-specific attributes. Native Windows builds are not supported by the supplied build configuration; use a Linux environment such as WSL for Windows development.

## Build

From the repository root:

```sh
make
```

This generates `libmalloc.so`.

Additional targets:

```sh
make debug   # Clean and rebuild with debug symbols
make check   # Build the library and run the test script
make clean   # Remove object files and the shared library
```

The Makefile uses C99 with strict warnings, position-independent code and hidden symbol visibility. The four allocation functions are explicitly exported.

## Usage

Load the allocator into a dynamically linked program:

```sh
LD_PRELOAD="$PWD/libmalloc.so" /bin/ls
```

To use it with your own program, call the usual allocation functions from `<stdlib.h>`, compile the program normally, then preload the library:

```sh
gcc example.c -o example
LD_PRELOAD="$PWD/libmalloc.so" ./example
```

Here, `example.c` is your own source file. Preloading applies to the program launched by that command; the executable must allow dynamic library preloading.

## API behavior

| Function or case | Behavior in this implementation |
|---|---|
| `malloc(size)` | Returns a pointer to a suitable block, or `NULL` on allocation failure. Requests above `SIZE_MAX / 2` are rejected with `ENOMEM`. |
| `free(NULL)` | Does nothing. |
| `free(ptr)` | Releases the allocation for reuse, coalescing or unmapping. |
| `calloc(count, size)` | Checks multiplication overflow, allocates the total size and clears the requested bytes. Overflow returns `NULL` with `ENOMEM`. |
| `realloc(NULL, size)` | Delegates to `malloc(size)`. |
| `realloc(ptr, 0)` for non-null `ptr` | Frees the allocation and returns `NULL`. |
| `realloc(ptr, smaller_size)` | Keeps the pointer and splits off unused space when possible. |
| `realloc(ptr, larger_size)` | Allocates a new block, preserves the old payload and frees the previous block. |

Pointers passed to `free` and non-null pointers passed to `realloc` must refer to live allocations returned by this allocator.

## Tests

```sh
make check
```

The shell test suite compiles small C programs and runs them with the allocator preloaded. It includes checks for:

- Writing to and reading from allocated memory.
- 16-byte pointer alignment on the target environment.
- Non-overlapping allocations.
- Oversized allocation rejection.
- Handling `free(NULL)`, reusing blocks and merging adjacent free blocks.
- Zero initialization and overflow handling in `calloc`.
- `realloc` behavior for null pointers, zero size, growth and shrinking.

The script also compares command output with and without preloading for `factor`, `ls`, `od` and `find`. It prints a pass/fail summary and returns a nonzero exit status if a check fails.

These checks provide functional smoke tests. They do not establish complete compatibility, performance or correctness for every workload. The test binaries additionally present in `tests/` are not invoked by `make check`.

## Project structure

```text
.
├── Makefile
├── README.md
├── src/
│   ├── malloc.c       # Public allocation functions
│   ├── auxi.c         # Mapping, lookup, splitting and coalescing
│   └── malloc.h       # Metadata definitions and function declarations
└── tests/
    ├── testsuite.sh   # Test script executed by make check
    ├── corruptionproof
    ├── memoryfootprint
    └── speed
```

## Limitations

- **Single-threaded design:** the global block list is not protected by locks. Linking pthread does not make the allocator thread-safe.
- **Linear allocation search:** finding a suitable block takes O(n) time in the number of blocks examined.
- **Basic reallocation strategy:** growth always allocates and copies instead of expanding into a neighboring free block.
- **Platform-dependent layout:** payload alignment also depends on the size and layout of the metadata structure. Portability to other ABIs has not been established.
- **Limited API:** aligned-allocation functions and other allocator extensions are not implemented.
- **Limited error diagnostics:** invalid pointers and double frees are not explicitly detected.

This is an educational allocator intended for studying and experimenting with memory management. Production use would require further correctness, concurrency, compatibility and performance work.
