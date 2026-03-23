# aosh

A lightweight Windows shell written in C++17.

## Features

- **Built-in commands**: `echo`, `pwd`, `clear`, `exit`, `help`, `mkdir`, `rmdir`, `mkfile`, `rmfile`
- **External program execution**: Run any `.exe` on PATH or in the current directory
  - `program` — searches PATH
  - `./program` — current directory only
- **Tab autocompletion** (zsh-style):
  - Built-in command names
  - Executables on PATH
  - File and directory paths
  - Single Tab inserts longest common prefix
  - Double Tab shows all candidates
  - Further Tabs cycle through matches with highlighting

## Building

Requires CMake 3.10+ and a C++17 compiler (MSVC or MinGW).

```bash
cmake -S . -B build -G Ninja
cmake --build build
```

The executable is placed at `build/aosh.exe`.

## Project Structure

```
aosh/
├── include/aosh/
│   ├── commands/          # Built-in command headers
│   │   ├── registry.h     # Command registry
│   │   ├── echo.h
│   │   ├── filesystem.h
│   │   └── help.h
│   ├── completion/        # Autocompletion system
│   │   ├── completer.h    # Orchestrator — manages Tab state and cycling
│   │   ├── completion_context.h  # Parses cursor position into context
│   │   ├── provider.h     # Abstract completion provider interface
│   │   ├── command_provider.h    # Completes built-in commands
│   │   ├── path_exe_provider.h   # Completes executables on PATH
│   │   └── filesystem_provider.h # Completes files and directories
│   ├── shell.h            # Main shell loop
│   ├── parser.h           # Input tokenizer
│   ├── process.h          # External program execution
│   └── color.h            # ANSI color codes
├── src/
│   ├── commands/          # Built-in command implementations
│   ├── completion/        # Completion provider implementations
│   ├── shell.cpp          # Shell loop, raw console input, Tab integration
│   ├── parser.cpp
│   ├── process.cpp
│   ├── color.cpp
│   └── main.cpp
└── CMakeLists.txt
```

## Usage

```
aosh C:\Users\You> hel<Tab>     →  help
aosh C:\Users\You> echo <Tab>   →  shows files/dirs in current directory
aosh C:\Users\You> git<Tab>     →  git (from PATH)
```
