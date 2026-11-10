# mini-c-interpreter

An interpreter for a small subset of C, written in standard **ISO C++17**: recursive-descent parser, static type checking and a tree-walking evaluator.

**Author:** Pietro Ravera

---

## Overview

The interpreter reads a program from a single text file, checks it lexically, syntactically and semantically (static type checking), and runs it only if all checks pass. The whole program is checked before execution starts, so a program with a syntax or type error produces no output other than the error.

On a compile-time error or a runtime error (division by zero, out-of-bounds array access, …) the interpreter stops at the first error and prints a single line in the following format:

```text
Error: <description>
```

---

## Language

* **Types:** `int` (signed integers) and `boolean` (`true`, `false`).
* **Arrays:** declared as `int[N] id;` or `boolean[N] id;`, including multi-dimensional ones such as `int[2][3] m;`. Elements are accessed with any integer expression, e.g. `m[i][j + 1]`, and every index is bounds-checked at runtime.
* **Memory:** a single global space. A variable is visible from its declaration onwards, even outside the block that declared it, and an identifier may be declared only once in the whole program.
* **Statements:**
  * Assignment: `<loc> = <bool> ;`
  * Conditionals: `if ( <bool> ) <stmt>` and `if ( <bool> ) <stmt> else <stmt>`
  * Loops: `while ( <bool> ) <stmt>` and `do <stmt> while ( <bool> ) ;`
  * `break ;` (only allowed inside a loop)
  * Output: `print ( <bool> ) ;`
  * Nested blocks: `{ <decls> <stmts> }`
* **Operators:**
  * Logical, with short-circuit evaluation: `||`, `&&`, `!`
  * Equality and relational: `==`, `!=`, `<`, `<=`, `>`, `>=`
  * Arithmetic: `+`, `-` (binary and unary), `*`, `/` (integer division)
* **Comments:** `//` line comments.

`print` writes integers as numbers and booleans as `1` / `0`.

---

## Project layout

```text
mini-c-interpreter/
├── main.cpp              # Entry point: command-line handling and error reporting
├── token.h / .cpp        # Token class and TokenType enum
├── tokenizer.h / .cpp    # Lexer: turns the source into tokens, skips '//' comments
├── parser.h / .cpp       # Recursive-descent parser, type checking, AST construction
├── interpreter.h / .cpp  # Evaluator: runs the AST, manages memory and bounds checks
├── UML.md                # Architecture notes and Mermaid diagrams
├── uml_diagram.svg       # UML class diagram (rendered)
├── uml_diagram.puml      # PlantUML source of the class diagram
├── tests/                # Test programs, expected outputs and the test runner
├── LICENSE
└── README.md
```

---

## Building

Requires a C++17 compiler (e.g. `g++ >= 7`, `clang++ >= 5`, MSVC 2017+).

**Linux / macOS**
```bash
g++ -std=c++17 -o interpreter main.cpp token.cpp tokenizer.cpp parser.cpp interpreter.cpp
```

**Windows (MinGW)**
```cmd
g++ -std=c++17 -o interpreter.exe main.cpp token.cpp tokenizer.cpp parser.cpp interpreter.cpp
```

**Windows (MSVC Developer Command Prompt)**
```cmd
cl /std:c++17 /EHsc main.cpp token.cpp tokenizer.cpp parser.cpp interpreter.cpp /Fe:interpreter.exe
```

---

## Usage

Pass the path of the source file as the only argument:

```bash
# Linux / macOS
./interpreter path/to/program.txt

# Windows
interpreter.exe path\to\program.txt
```

### Example

```c
{
  int[5] v;
  int i;
  int sum;

  i = 0;
  sum = 0;
  while (i < 5) {
    v[i] = (i + 1) * 10;
    sum = sum + v[i];
    i = i + 1;
  }
  print(sum);
}
```

Output:
```text
150
```

An error example:

```c
{
  int[3] a;
  a[3] = 1;
}
```

```text
Error: index 3 out of bounds for 'a' (size 3)
```

---

## Testing

The `tests/` folder holds sample programs, each with the exact output it must produce:

* `pass_*.txt`: valid programs; the interpreter must print the expected output and exit with status `0`.
* `fail_*.txt`: programs with a lexical, syntax, type or runtime error; the interpreter must print the expected output (any lines printed before the error, then the `Error:` line) and exit with status `1`.

Run them all with:

```bash
tests/run_tests.sh                 # builds the interpreter with g++ in a temporary folder
tests/run_tests.sh ./interpreter   # or uses an existing build
```

The script prints a diff for every failing test and a final summary such as `34 passed, 0 failed`.

---

## Design and UML

The AST and evaluator follow the **Interpreter** and **Composite** patterns:

* **[UML.md](UML.md):** Mermaid diagrams for the class structure, the AST and the execution sequence, with explanations.
* **[uml_diagram.svg](uml_diagram.svg):** rendered class diagram, viewable in any browser.
* **[uml_diagram.puml](uml_diagram.puml):** PlantUML source of the class diagram.

---

## License

Released under the [MIT License](LICENSE).
