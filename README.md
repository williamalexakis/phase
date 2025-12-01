# Phase

## Overview

An interpreted, statically-typed programming language designed to bridge the gap between the expressiveness of dynamic, high-level languages and the explicitness of stricter, low-level languages.

## Features

- **Clean Semantics** – Syntax is clear, balanced, and predictable.
- **Static Typing** – Type declarations are syntactically consistent and safety is ensured by compile-time checking.
- **Bytecode Compilation** – Sourcecode is compiled into a portable instruction set executed by a handwritten VM.
- **Error Reporting** – Errors are comprehensively handled, displaying helpful and verbose messages.
- **Debug Modes** – View token streams and AST structure to effectively diagnose sourcecode.

## Installation

### Prerequisites

- CMake 3.20+
- C compiler supporting C17

### Building

1. Clone the repo:
   ```bash
   git clone https://github.com/williamalexakis/phase.git
   cd phase
   ```

2. Build the project:
   ```bash
   mkdir build
   cd build
   cmake ..
   cmake --build .
   ```

### Usage

To get started, run the `phase` executable using the `--help` flag.

## Syntax

### Entrypoint

Every Phase program requires an `entry` block to signify its entrypoint:

```c
entry {
    -- Your code here
}
```

### Printing

Use the `out()` statement to print expressions and variables:

```c
entry { 
    out("Hello world!") 
}
```

**Output:**
```
Hello world!
```

### Types

Phase supports strings (`str`), integers (`int`), floats (`float`), and booleans (`bool`):

```c
entry {
    str string = "Hello"
    int integer = 10
    float floating = 5.5
    bool boolean = true
}
```

Values are implicitly converted to `str` type when using `out()`:

```c
entry {
    int num = 3
    float decimal = 3.14
    out(num)
    out(decimal)
}
```

**Output:**
```
3
3.14
```

### Variables

Variables can be declared or initialized:

```c
entry { 
    str var_1         -- (null)
    str var_2 = "Hi"  -- hi
}
```

And can be assigned/reassigned a value as expected:

```c
entry {
    int num    -- (null)
    num = 3    -- 3
    num = 300  -- 300
}
```

But Phase also supports grouped declarations and initializations:

```c
entry {
    -- Grouped declaration
    str (name, surname)

    -- Grouped initialization
    int (x, y, z) = (1, 2, 3)
}
```
