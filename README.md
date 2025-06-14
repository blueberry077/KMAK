# 🧱 KMAK Build System

KMAK (Komodore Make) is a lightweight and minimalist build system written in C.<br>
It is inspired by Makefile but much simpler with custom tasks support.<br>
![Build](https://img.shields.io/badge/build-kmak-lightgrey?style=flat-square&logo=c) ![Lang](https://img.shields.io/badge/language-C-blue?style=flat-square&logo=c) ![License](https://img.shields.io/badge/license-GPL-green?style=flat-square) ![Status](https://img.shields.io/badge/status-WIP-orange?style=flat-square)

## Building
To build KMAK, run
```
kmak.exe make.kmk compile
```
from the main directory. It will produce a `kmak0.exe` file that you can then rename `kmak.exe` to use.

## Starting
```
usage: kmak.exe InputFile <Task>
```

For example with `make.kmk` in `example`:
```
# Global Variables
CC = gcc
SRC = game.c
TARGET = game.exe

# Tasks
task clean
	cmd del $(TARGET)

task run
	cmd $(TARGET)

task make
	cmd $(CC) -o $(TARGET) $(SRC)
```

You can execute:
```
kmak make.kmk make
```
or...:
```
kmak make.kmk run
```
To execute game.exe.
