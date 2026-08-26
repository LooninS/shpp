# sh++

A minimal POSIX-like shell written in modern C++ (C++17/20).  
Built as a learning project to understand how shells work under the hood: process creation, `exec`, `wait`, `PATH` lookup, and builtins.

## Features

- Interactive REPL with a simple prompt (`$ `)
- External command execution via `fork()` + `execv()`
- `PATH`-based executable lookup (`find_exec`)
- Basic builtins:
  - `echo` – print arguments
  - `type` – show whether a command is a builtin or an external executable
  - `exit` – leave the shell
- Foreground job handling (shell waits for commands to finish)
- Modern C++ style:
  - `std::filesystem` for path handling
  - `std::string` / `std::vector` for command parsing
  - RAII and exceptions for error handling

## Building

### Requirements

- C++ compiler with C++17 or later support (GCC/Clang)
- CMake 3.16+
- POSIX environment (Linux, macOS, WSL, etc.)

On Arch-based systems:

```bash
sudo pacman -S base-devel cmake
```

On Debian/Ubuntu:

```bash
sudo apt-get update
sudo apt-get install build-essential cmake
```

### Build commands

From the project root:

```bash
cmake -B build -S .
cmake --build ./build
```

This produces the `shell` binary in `build/`.

### One-step build + run script

Use the provide `sh++.sh` to compile and run the shell.
Requires Cmake and make to be installed

Examples:

```text
$ echo hello world
hello world

$ type echo
echo is a shell builtin

$ type ls
ls is /usr/bin/ls

$ ls -l /tmp
...

$ exit
```

Commands not found in `PATH` or as builtins will print:

```text
cmd: command not found
```

## Project structure

- `src/main.cpp` – main shell logic:
  - `Command` struct
  - `parse_cmd()` – split input line into command + args
  - `split_path_variable()` – parse `$PATH`
  - `find_exec()` – search `PATH` for an executable
  - `runExternal()` – `fork()` + `execv()` + `waitpid()`
  - `main()` – REPL loop and builtin handling
- `CMakeLists.txt` – CMake configuration for building `shell`
- `build.sh` – convenience script to configure, build, and run


## Future ideas

Possible extensions:

- Add `cd` and `pwd` builtins
- Implement `&&`, `||`, and `;` command chaining using exit statuses
- Add background jobs (`cmd &`) and basic job control
- Implement simple redirection: `cmd > out`, `cmd < in`
- Support pipelines: `cmd1 | cmd2`
- Better tokenizer (quotes, escapes, globs)

---

**Note:** This project is for learning and experimentation. Do not use it as your daily interactive shell.
