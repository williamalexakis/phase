# Chunk-Based File Reading in C

*4 June 2026*

The biggest fix I wrote in Phase [v1.1.0](https://github.com/williamalexakis/phase/releases/tag/v1.1.0) was replacing the old `fseek()` / `ftell()` file reading system with a much safer chunk-based mechanism only using `fread()`. This issue was brought up by [Christopher Wellons](https://github.com/skeeto) whose code review inspired the release, but I want to look closer at this system because it's useful in general.

For both implementations, the goal was the same — to read a `.phase` file into a C string called `file_content` — so I'll start with the old code here:

```c
// phase/src/main.c

// We first add our file to a pointer, and
// check for a file not found error
FILE *input_file = fopen(argv[1], "r");
if (!input_file) error_ifnf(argv[1]);

// Move the file pointer to the end of the
// file, using fseek(), before we use
// ftell() to get the exact size
fseek(input_file, 0, SEEK_END);
size_t file_size = (size_t)ftell(input_file);
// Rewind the pointer back to the start
rewind(input_file);

// Then we allocate the necessary memory to
// our string pointer, plus 1 for a null
// terminator at the end
char *file_content = malloc(file_size + 1);
// Raise an out of memory error if
// file_content is NULL
if (!file_content) {
    fclose(input_file);
    error_oom();
}

// Finally, we use fread() to insert the file
// content into the string, null-terminate,
// and close the file
fread(file_content, sizeof(char), file_size, input_file);
file_content[file_size] = '\0';
fclose(input_file);
```

This old code is basically the standard way of reading files into C strings, and it worked fine because I only tested sane cases instead of extreme edge cases like Wellons did.

The main bug is that if a file is unseekable (e.g., passed via pipe, socket, etc.) then `ftell()` returns `-1L`, a `long` integer, to represent an error. But our typecast of `size_t` is an `unsigned` type (`unsigned long` on LP64 macOS) — so we have to convert the negative `signed` value to a positive `unsigned` value. This is done using a standard formula for conversion, assuming $`u`$ is the `unsigned` value, $`s`$ is the `signed` value, $`w`$ is the bit width of the `unsigned` type, and $`\mathrm{max}`$ is the maximum value of the `unsigned` type (in this case, `SIZE_MAX`):

```math
u = s \mod 2^w \qquad
w = \log_2(\mathrm{max} + 1)
```

With this, converting `-1L` gives us `SIZE_MAX`. And since we `malloc()` by `file_size + 1` then an integer overflow occurs and we allocate 0 bytes — returning a non- `NULL` pointer that silently evades the `!file_content` check. And with this check passed, we then get a buffer overflow when calling `fread()`, plus a second buffer overflow when we add the null terminator.

The fix I implemented is a chunk-based reader which doesn't use `fseek()` / `ftell()` at all. The premise is that we dynamically reallocate the string's memory based on `fread()` returning full chunks, which tells us there is still more content to read so we should continue. Specifically, I set `size=1` and `nmemb=CHUNK_SIZE` for `fread()`, which returns the exact byte count even on partial reads; if I had done the opposite, a partial final chunk would return 0 (for zero fulfilled elements) so I'd lose the exact byte count:

```c
// phase/src/main.c

// (*input_file and error_ifnf lines stay the same)

// Each chunk is a constant 4096 bytes
const size_t CHUNK_SIZE = 4096;
size_t file_len = 0;
size_t file_cap = CHUNK_SIZE;  // Set to one chunk

// We do an initial allocation to start off with
// some space already there, plus we check for
// an OOM error now so things don't get
// messy later
char *file_content = malloc(file_cap);
if (!file_content) {
    fclose(input_file);
    error_oom();
}

// Read the source file in CHUNK_SIZE byte segments
// to handle potentially any unseekable inputs
while (true) {
    // Double cap for extra memory space
    if ((file_len + CHUNK_SIZE) > file_cap) {
        size_t new_cap = file_cap * 2;
        // Sanitize our reallocation using a
        // temporary pointer to check for
        // validity, BEFORE we touch the
        // content string
        char *temp_ptr = realloc(file_content, new_cap);
        if (!temp_ptr) {
            free(file_content);
            fclose(input_file);
            error_oom();
        }

        file_content = temp_ptr;  // Update our memory
        file_cap = new_cap;       // Update our capacity
    }
    // Read up to CHUNK_SIZE bytes (size=1 * nmemb=CHUNK_SIZE)
    // and return the exact number of bytes read
    size_t num_bytes = fread(file_content + file_len,
                             1, CHUNK_SIZE, input_file);
    file_len += num_bytes;  // Increase our length counter
                            // by the bytes read

    // If the bytes read is LESS than the expected
    // constant, we stop because either we've hit
    // EOF or an error happened
    if (num_bytes < CHUNK_SIZE) break;
}

// Nonzero return from ferror() tells us it's
// an actual error instead of just the EOF
if (ferror(input_file) != 0) {
    free(file_content);
    fclose(input_file);
    error_io(argv[1]);
}

// We sanitize our reallocation again, this
// time for the extra null terminator byte
char *temp_ptr = realloc(file_content, file_len + 1);
if (!temp_ptr) {
    free(file_content);
    fclose(input_file);
    error_oom();
}

// And finally we update file_content's memory,
// terminate, and close the file like usual
file_content = temp_ptr;
file_content[file_len] = '\0';
fclose(input_file);
```

With this new implementation, we can now safely pass in source files in any way we want, and there are no overflows, memory leakage, or undefined behaviour.

It took me some thinking to figure out how I can use `fread()` to inject a file into a string without knowing its size already, and that's always annoyed me with how weird files in C are, but solving this issue made everything much clearer. Mainly it taught me to never use `ftell()` to size a buffer, and instead take the safer route of dynamically growing one using `fread()` 's return value.

And this piece of code isn't really specific to my project, so you could use it in your own projects too — it's really useful and it'll save you from future headaches.

[phase/src/main.c](https://github.com/williamalexakis/phase/blob/main/src/main.c)
