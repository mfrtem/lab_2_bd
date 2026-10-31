cmake_minimum_required(VERSION 3.16)

project(LibraryDB)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_executable(LibraryDB
    main.cpp
    database.cpp
    hashindex.cpp
)

target_link_libraries(LibraryDB PRIVATE Qt6::Widgets)

add_executable(HashTest
    hash_test.cpp
    hashindex.cpp
)

add_executable(DatabaseTest
    database_test.cpp
    database.cpp
    hashindex.cpp
)

add_executable(BTreeTest
    btree_test.cpp
    btree.cpp
)