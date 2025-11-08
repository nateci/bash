# NUFS - Custom File System Implementation

A user-space file system implementation using FUSE (Filesystem in Userspace), built in C. This project implements core file system operations including file creation, deletion, reading, writing, and directory management.

## Overview

NUFS (NU File System) is a fully functional file system that operates in user space, demonstrating low-level systems programming concepts including:
- Inode-based file storage
- Directory structure management
- Block allocation and management
- FUSE API integration

## Key Components

- **`nufs.c`** - Main FUSE operations and file system interface
- **`storage.c/h`** - Block storage management and allocation
- **`inode.c/h`** - Inode structure and operations
- **`directory.c/h`** - Directory operations and path resolution
- **`test.pl`** - Comprehensive test suite

## Features

- Create, read, write, and delete files
- Directory creation and navigation
- File metadata management (permissions, timestamps)
- Efficient block-based storage system
- POSIX-compliant file operations

## Building & Running
```bash
# Compile the file system
make

# Mount the file system
./nufs [mount_point]

# In another terminal, interact with it
cd [mount_point]
touch testfile.txt
echo "Hello, NUFS!" > testfile.txt
cat testfile.txt

# Unmount when done
fusermount -u [mount_point]
```

## Testing
```bash
# Run the test suite
perl test.pl
```

## Technologies Used

- **C** - Core implementation
- **FUSE** - Filesystem in Userspace library
- **Perl** - Testing framework
- **Make** - Build automation

## Project Context

Developed as part of systems programming coursework, exploring operating systems concepts including file systems, memory management, and low-level I/O operations.

## Requirements

- Linux/Unix environment
- FUSE library (`libfuse-dev`)
- GCC compiler
- Make

## License

MIT License
