# Phase

An interpreted, statically-typed language designed to bridge the gap between Python's expressiveness and C's explicitness.

## Features

- **Clean syntax** for easy readability
- **Static typing** for safety
- **Type checking** at compile time
- **Bytecode compilation** with virtual machine execution
- **Comprehensive error reporting** featuring helpful messages
- **Debug modes** for viewing the source file as tokens or an AST

## Examples

### Hello World
```
entry {
    out("Hello world!")
}
```

```
>> Hello world!
```

### Types
```
entry {

    int integer = 10
    str string = "Hello"

    out(integer)
    out(string)

}
```

```
>> 10
>> Hello
```

### Declarations and Initializations
```
entry {

    int num_1  -- Single declaration
    int num_2 = 100  -- Single initialization

    int (num_3, num_4)  -- Grouped declaration

    -- Grouped initializations
    int (x, y, z) = (1, 2, 3)
    str (i, j, k) = ("Hello", "world", "!")

}
```
