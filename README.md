# Phase

An interpreted, statically-typed language designed to bridge the gap between Python's expressiveness and C's explicitness.

## Features

- **Clean syntax** for easy readability
- **Static typing** for safety
- **Type checking** at compile time
- **Bytecode compilation** with virtual machine execution
- **Comprehensive error reporting** featuring helpful messages
- **Debug modes** for viewing the source file as tokens or an AST

## Installation

### Prerequisites

- CMake (3.20 or higher)
- C compiler that supports C17

### Building

1. Clone the repo:
   ```bash
   git clone <repo-url>
   cd phase
   ```

2. Build the project:
   ```bash
   mkdir build && cd build
   cmake ..
   cmake --build .
   ```

### Usage

To get started, use:

```bash
./phase --help
```

## Syntax

### Entrypoint

Every Phase program requires an `entry` block:

```phase
entry {
    -- Your code here
}
```

### Hello World

```phase
entry {
    out("Hello world!")
}
```

**Output:**
```
Hello world!
```

### Types and Variables

Phase supports static typing with type safety:

```phase
entry {
    str string = "Hello"
    int integer = 10
    float floating = 5.5
    bool boolean = true

    out(string)
    out(integer)
    out(floating)
    out(boolean)
}
```

**Output:**
```
Hello
10
5.5
true
```

### Variable Declarations and Initializations

Phase supports both single and grouped variable operations:

```phase
entry {
    -- Single declaration
    int num_1

    -- Single initialization
    int num_2 = 100

    -- Grouped declaration
    int (num_3, num_4)

    -- Grouped initialization
    int (x, y, z) = (1, 2, 3)

}
```
