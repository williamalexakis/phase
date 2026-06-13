# Phase

![GitHub Tag](https://img.shields.io/github/v/tag/williamalexakis/phase)
![GitHub Actions Workflow Status](https://img.shields.io/github/actions/workflow/status/williamalexakis/phase/ci.yml)
[![License](https://img.shields.io/badge/license-MIT-green)
](LICENSE)

A statically-typed bytecode-interpreted programming language in C with zero dependencies.

[Bytecode Interpretation From Scratch in C](bytecode-interpretation-from-scratch.md)

[Chunk-Based File Reading in C](chunk-reading-in-c.md)

## Building

```bash
git clone https://github.com/williamalexakis/phase.git
cd phase
mkdir build
cd build
cmake ..
cmake --build .
```

## Syntax

**Hello World**
```go
entry {
    out("Hello world!")
}
```

**Functions**
```go
func add(num1: int, num2: int): void {
    out(num1 + num2)
}

entry {
    add(5, 5)
}
```

**Loops**
```go
entry {
    let num: int = 0
    
    while num < 11 {
        out(num)
        num += 1
    }
}
```

[More examples](examples/)

## Architecture

**INPUT → Lexer → Parser → Type Checker → Bytecode Generator → Virtual Machine → OUTPUT**

Programs are compiled into hexadecimal bytecode and executed by a stack-based VM supporting 25 opcodes. For example, **Hello World** translates to:
```
0x0000  →  00 00 00  →  OP_PUSH_CONST 0
0x0003  →  01        →  OP_PRINT
0x0004  →  18        →  OP_HALT
```

## Usage

- `phase --help` — list commands and flags
- `phase <file.phase> --tokens` — print the token stream
- `phase <file.phase> --ast` — print the AST
- `phase <file.phase> --loud` — print a success message on exit
