# Phase

A minimal interpreter with a bytecode VM for the **Phase** programming language.

## Overview

Phase is a simple, statically-typed programming language designed for learning compiler construction concepts. It features a clean syntax, type safety, and compiles to bytecode that runs on a virtual machine.

## Features

- **Static typing** with `int` and `str` data types
- **Variable declarations** with optional initialization
- **Grouped variable declarations** for concise code
- **Type checking** at compile time
- **Bytecode compilation** with virtual machine execution
- **Comprehensive error reporting** with line numbers and helpful messages
- **Debug modes** for inspecting tokens and AST

## Language Syntax

### Basic Program Structure
```phase
entry {
    out("Hello, Phase!")
}
```

### Variable Declarations
```phase
entry {
    -- Single variable declarations
    int x = 42
    str message = "Hello"
    
    -- Grouped declarations
    int (a, b, c) = (1, 2, 3)
    str (first, last) = ("John", "Doe")
    
    -- Declarations without initialization
    int result
    str buffer
    
    -- Assignments
    result = a + b  -- Note: arithmetic not yet implemented
    buffer = "Ready"
}
```

### Comments
```phase
-- This is a single-line comment
entry {
    out("Comments start with --")
}
```

## Building and Running

### Prerequisites
- CMake 3.20 or higher
- GCC 15 (or compatible C compiler)
- macOS, Linux, or WSL

### Building
```bash
# Create build directory
mkdir build
cd build

# Configure and build
cmake ..
cmake --build .

# Or use the convenience script
./run examples/test_01.phase
```

### Usage
```bash
# Run a Phase program
./phase <input_file.phase>

# View tokens (lexer output)
./phase <input_file.phase> --tokens

# View AST (parser output)
./phase <input_file.phase> --ast

# Get help
./phase --help
```

## Examples

### Hello World
```phase
-- examples/test_01.phase
entry {
    out("Hello world!")
}
```

### Variable Declarations and Assignments
```phase
-- examples/test_02.phase
entry {
    -- Grouped variable initializations
    int (a, b, c) = (5, 10, 15)
    str (x, y, z) = ("hello", "world", "!")

    -- Declarations
    str i
    str j
    str k

    -- Cross assignments
    i = x
    j = y
    k = z

    -- Print results
    out("Numbers:")
    out(a)
    out(b)
    out(c)
    out("")
    out("Strings:")
    out(i)
    out(j)
    out(k)
}
```

## Architecture

The Phase interpreter follows a traditional compiler pipeline:

1. **Lexer** (`lexer.c`) - Tokenizes source code
2. **Parser** (`parser.c`) - Builds Abstract Syntax Tree (AST)
3. **Code Generator** (`codegen.c`) - Emits bytecode instructions
4. **Virtual Machine** (`codegen.c`) - Executes bytecode with stack-based VM

### Bytecode Instructions
- `OP_PUSH_CONST` - Push constant to stack
- `OP_PRINT` - Print top of stack
- `OP_SET_VAR` - Store value in variable
- `OP_GET_VAR` - Load variable to stack
- `OP_HALT` - Stop execution

## Error Handling

Phase provides detailed error messages with context:

```
╭ ERROR [114]: Type mismatch for variable 'x'
|
╰ Note: Expected int but received str.
```

Error categories include:
- Syntax errors (missing symbols, unexpected tokens)
- Type errors (mismatched assignments)
- Runtime errors (undefined variables, VM errors)

## Current Limitations

This is a minimal implementation with several intentional limitations:

- **No arithmetic operations** (yet)
- **No control flow** (if/else, loops)
- **No functions** beyond the entry point
- **Limited data types** (only int and str)
- **No file I/O** beyond console output

These limitations make Phase ideal for learning compiler basics without overwhelming complexity.

## Development Status

Phase is a **learning project** and **proof of concept**. It demonstrates:

- Complete lexer/parser implementation
- Type-safe AST construction
- Bytecode generation
- Stack-based virtual machine
- Comprehensive error handling
- Memory management in C

## Contributing

This project is primarily educational, but contributions are welcome! Areas for enhancement:

- Add arithmetic and comparison operators
- Implement control flow statements
- Add function definitions and calls
- Expand type system
- Improve error recovery
- Add more comprehensive tests

## License

This project is open source. See the repository for license details.

## Technical Details

- **Language**: C17
- **Build System**: CMake
- **Compiler**: GCC 15
- **Architecture**: Bytecode VM with stack machine
- **Memory Management**: Manual allocation with cleanup
- **Error Handling**: Structured error reporting with ANSI colors

---

*Phase - A simple language for learning compiler construction*