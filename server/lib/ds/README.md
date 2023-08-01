#### libgenerics
libgenerics is a small static library for personal use consists of 4 'generics' data structures written in C. The library contains an implementation of a vector, doubly linked list (aka list), a hash table and a binary search tree.

#### vector
Vector provides an implementation of a heap allocated vector. The underlying array saves a shallow copy of the data passed in. Note that you should avoid accessing the members of `struct vector` directly, use the various methods provided below.

#### list
List provides an implementation of a heap allocated, doubly linked list. Each node contains a shallow copy of the data one pass in. Note that you should avoid accessing the members of `struct list` directly, use the various methods provided below.

#### hash table
Hash table provides an implementation of a heap allocated hash table. Under the hood the hash table consists of a vector which holds `size` entries. Each entry is a doubly linked list which contains a shallow copy of the data one might pass in. Note that you should avoid accessing the members of `struct hash_table` directly, use the various methods provided below.

#### compiling and building
You have 2 options when it comes to build and link your project against the library:
1. Clone the repository. Create a subdirectory in the code base with your project in it and set CMake accordingly
2. Download the `.a` file via releases or clone and build the project yourself. Copy the `.a` file into your project as well as the header files, and link the library against your project.
An example project might looks like:
```bash
.
├── CMakeLists.txt
├── include
│   ├── hash_table.h
│   ├── list.h
│   └── vector.h
├── lib
│   └── libgenerics.a
└── src
    └── example.c
```
CMakeLists.txt will then contains the following lines:
```
# import the library
add_library(libgenerics STATIC IMPORTED)
set_target_properties(
  libgenerics
  PROPERTIES
  IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/lib/libgenerics.a
  INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(<executable name> PRIVATE libgenerics)
```