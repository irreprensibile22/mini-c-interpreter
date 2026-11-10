# Interpreter UML Design
**Author:** Pietro Ravera  

---

## 1. Architecture Overview

The interpreter for the C subset follows the classic translate-then-execute pipeline:

1. **Lexical Analysis (Scanner / Tokenizer):**
   - The `Tokenizer` class reads a source `std::istream` and produces `Token` objects one at a time.
   - Each token is classified by the `TokenType` enumeration and stores its position in the source text (line and column), so error messages can point to the exact location.

2. **Syntax Analysis and Static Semantics (Parser):**
   - The `Parser` class is a *recursive-descent parser* that checks the source against the language's context-free grammar.
   - It performs *type checking* on arithmetic, boolean and relational expressions.
   - It manages the symbol table (`symbolTable`) to ensure every identifier is declared at most once and is only used after its declaration.
   - It builds the abstract syntax tree made of `ASTNode` objects.

3. **Runtime Evaluation and Execution (Evaluator):**
   - The `Evaluator` class walks and executes the abstract syntax tree (`ASTNode`).
   - It holds the single memory space for scalar variables and arrays (`variables` and `arrays`).
   - It implements short-circuit evaluation for the logical operators `&&` and `||`.
   - It performs runtime checks on array bounds (*bounds checking*) and division by zero, raising exceptions that `main()` reports as `Error: <description>`.
   - It handles control-flow interruptions (`break`) through the dedicated `BreakException`.

---

## 2. UML Class Diagram (System Structure)

```mermaid
classDiagram
    direction TB

    class TokenType {
        <<enumeration>>
        IF
        ELSE
        WHILE
        DO
        BREAK
        INT
        BOOLEAN
        TRUE
        FALSE
        PRINT
        ID
        NUM
        PLUS
        MINUS
        TIMES
        DIVIDE
        EQ
        NEQ
        LT
        LEQ
        GT
        GEQ
        AND
        OR
        NOT
        ASSIGN
        SEMICOLON
        LPAREN
        RPAREN
        LBRACE
        RBRACE
        LBRACKET
        RBRACKET
        END_OF_FILE
    }

    class Token {
        +TokenType type
        +string value
        +int line
        +int column
        +Token()
        +Token(TokenType, string, int, int)
    }

    class Tokenizer {
        -istream& input
        -int currentChar
        -int line
        -int column
        +Tokenizer(istream&)
        +getNextToken() Token
        -readNextChar() void
        -skipWhitespace() void
        -scanIdentifier(int, int) Token
        -scanNumber(int, int) Token
    }

    class ValueType {
        <<enumeration>>
        INT_TYPE
        BOOL_TYPE
        VOID_TYPE
    }

    class NodeType {
        <<enumeration>>
        BLOCK
        DECLS
        DECL
        TYPE
        BASIC
        STMTS
        IF
        WHILE
        DO
        BREAK
        PRINT
        ASSIGN
        LOC
        ID
        NUM
        TRUE
        FALSE
        AND
        OR
        NOT
        EQ
        NEQ
        LT
        LEQ
        GT
        GEQ
        PLUS
        MINUS
        TIMES
        DIVIDE
        UNARY_MINUS
    }

    class ASTNode {
        +NodeType type
        +string value
        +ValueType valuetype
        +vector~ASTNode*~ children
        +ASTNode(NodeType, string, ValueType)
        +~ASTNode()
    }

    class Symbol {
        <<struct>>
        +ValueType baseType
        +vector~int~ dims
        +isArray() bool
    }

    class Parser {
        -Tokenizer& tokenizer
        -Token currentToken
        -unordered_map~string, Symbol~ symbolTable
        -int loopDepth
        +Parser(Tokenizer&)
        +parseProgram() ASTNode*
        -consume(TokenType) void
        -parseBlock() ASTNode*
        -parseDecls() ASTNode*
        -parseDecl() ASTNode*
        -parseStmts() ASTNode*
        -parseStmt() ASTNode*
        -parseLoc() ASTNode*
        -parseBool() ASTNode*
        -parseJoin() ASTNode*
        -parseEquality() ASTNode*
        -parseRel() ASTNode*
        -parseExpr() ASTNode*
        -parseTerm() ASTNode*
        -parseUnary() ASTNode*
        -parseFactor() ASTNode*
        -tokenTypeToString(TokenType) string
        -where() string
    }

    class exception {
        <<std>>
    }

    class BreakException {
        +what() const char*
    }

    class Evaluator {
        -map~string, int~ variables
        -map~string, Array~ arrays
        +Evaluator()
        +evaluate(ASTNode*) int
        -cell(ASTNode*) int&
    }

    class Array {
        <<struct>>
        +vector~int~ dims
        +vector~int~ cells
    }

    Token --> TokenType : type
    Tokenizer ..> Token : creates
    Parser --> Tokenizer : references
    Parser *-- Token : currentToken
    Parser *-- Symbol : symbolTable
    Parser ..> ASTNode : builds
    ASTNode *-- ASTNode : children (Composite)
    ASTNode --> NodeType : type
    ASTNode --> ValueType : valuetype
    Symbol --> ValueType : baseType
    Evaluator ..> ASTNode : evaluates
    Evaluator *-- Array : arrays
    Evaluator ..> BreakException : throws / catches
    exception <|-- BreakException
```

A rendered version of this diagram is available in [uml_diagram.svg](uml_diagram.svg) (PlantUML source: [uml_diagram.puml](uml_diagram.puml)).

---

## 3. Conceptual AST Diagram (Interpreter & Composite Patterns)

The logical hierarchy of the AST nodes follows the **Composite** and **Interpreter** patterns. In the code, this hierarchy is not implemented as separate C++ classes: every node is an `ASTNode`, and its role is given by the `NodeType` field. The `Evaluator` dispatches on that field, which plays the part of the virtual `eval()` method. The table after the diagram maps each conceptual class to its `NodeType` values.

```mermaid
classDiagram
    direction TB

    class Node {
        <<abstract>>
        +eval() int
    }

    class Stmt {
        <<abstract>>
    }

    class Expr {
        <<abstract>>
    }

    Node <|-- Stmt
    Node <|-- Expr

    %% Stmt subclasses
    class If
    class Else
    class While
    class Do
    class Set
    class SetElem
    class Break
    class Print
    class Seq

    Stmt <|-- If
    Stmt <|-- Else
    Stmt <|-- While
    Stmt <|-- Do
    Stmt <|-- Set
    Stmt <|-- SetElem
    Stmt <|-- Break
    Stmt <|-- Print
    Stmt <|-- Seq

    %% Expr subclasses
    class Op {
        <<abstract>>
    }
    class Constant
    class Logical {
        <<abstract>>
    }
    class Id

    Expr <|-- Op
    Expr <|-- Constant
    Expr <|-- Logical
    Expr <|-- Id

    %% Operators
    class Unary
    class Access
    class Arith

    Op <|-- Unary
    Op <|-- Access
    Op <|-- Arith

    %% Logical and relational operators
    class Not
    class Or
    class And
    class Rel

    Logical <|-- Not
    Logical <|-- Or
    Logical <|-- And
    Logical <|-- Rel

    %% Composition
    Seq o-- Stmt : contains
    Set o-- Expr : assigns
    SetElem o-- Expr : assigns
    If o-- Expr : condition
    If o-- Stmt : then/else
    While o-- Expr : condition
    While o-- Stmt : body
    Do o-- Stmt : body
    Do o-- Expr : condition
    Op o-- Expr : operands
    Logical o-- Expr : operands
```

| Conceptual class | `NodeType` in the code | Notes |
|---|---|---|
| `Seq` | `BLOCK`, `DECLS`, `STMTS` | `BLOCK` has two children: the declarations and the statements |
| `If` / `Else` | `IF` | 2 children (`if`) or 3 children (`if ... else`) |
| `While` | `WHILE` | children: condition, body |
| `Do` | `DO` | children: body, condition |
| `Set` | `ASSIGN` with an `ID` target | scalar assignment |
| `SetElem` | `ASSIGN` with a `LOC` target | array element assignment |
| `Break` | `BREAK` | raises `BreakException` |
| `Print` | `PRINT` | prints the value of its only child |
| `Arith` | `PLUS`, `MINUS`, `TIMES`, `DIVIDE` | |
| `Unary` | `UNARY_MINUS` | |
| `Access` | `LOC` | one `LOC` per index: `a[i][j]` is `LOC(LOC(ID a, i), j)` |
| `Constant` | `NUM`, `TRUE`, `FALSE` | |
| `Id` | `ID` | |
| `Not`, `And`, `Or` | `NOT`, `AND`, `OR` | `AND` and `OR` short-circuit |
| `Rel` | `EQ`, `NEQ`, `LT`, `LEQ`, `GT`, `GEQ` | |
| *(declarations)* | `DECL`, `TYPE`, `BASIC` | `TYPE` wraps the base type once per array dimension |

---

## 4. Detailed Component Description

### `Token` and `TokenType`
* **Role:** The smallest lexical unit found in the source.
* **Attributes:**
  - `type`: Syntactic category of the token (keywords such as `IF`, `WHILE`, operators such as `PLUS`, `ASSIGN`, separators, identifiers `ID`, numbers `NUM`, etc.).
  - `value`: Literal text of the lexeme (e.g. a variable name or a numeric value).
  - `line`, `column`: Coordinates used to locate errors in diagnostic messages.

### `Tokenizer`
* **Role:** Lexical analyzer.
* **Main methods:**
  - `getNextToken()`: Consumes characters from the `input` stream, skipping whitespace and single-line comments (`//`), and returns the next valid token or `END_OF_FILE`. Invalid characters (e.g. `%`, `^`) raise a lexical error.
  - `scanIdentifier()`: Recognizes identifiers and reserved words (`if`, `else`, `while`, `do`, `break`, `int`, `boolean`, `true`, `false`, `print`).
  - `scanNumber()`: Recognizes decimal integer constants and rejects literals that do not fit in an `int`.

### `Parser`
* **Role:** Recursive-descent syntax analyzer with static validation of types and declarations.
* **Responsibilities:**
  - Implements the grammar productions of the language; left-recursive rules are written as loops to keep left associativity.
  - Checks that every variable is declared before it is used.
  - Forbids declaring the same identifier more than once (`symbolTable`).
  - Checks type consistency in assignments and in arithmetic (`+`, `-`, `*`, `/`), relational (`<`, `<=`, `>`, `>=`), equality (`==`, `!=`) and boolean (`&&`, `||`, `!`) expressions.
  - Recognizes array declarations such as `int[10] v;` and indexing such as `v[i]`.
  - Checks that every `break` statement is inside a loop (`loopDepth > 0`).

### `ASTNode`
* **Role:** Node of the Abstract Syntax Tree (AST).
* **Attributes:**
  - `type`: Kind of syntactic construct the node represents (`NodeType`).
  - `value`: Associated value (variable name, operator string or literal).
  - `valuetype`: Semantic data type (`INT_TYPE`, `BOOL_TYPE`, `VOID_TYPE`).
  - `children`: Vector of pointers to the child subtrees (Composite pattern). The destructor frees the whole subtree.

### `Evaluator`
* **Role:** Engine that interprets and executes the AST.
* **Responsibilities:**
  - Evaluates $\langle decls \rangle$ first and then $\langle stmts \rangle$ in each block.
  - Manages a single global memory space for scalar variables (`variables`) and arrays, including multidimensional ones (`arrays`, stored row-major in `Array::cells`).
  - Executes `if-else` conditionally and `while` / `do-while` iteratively.
  - Exits loops early when a `BreakException` is raised.
  - Uses short-circuit evaluation for the logical operators `&&` and `||`.
  - Detects and reports runtime errors: division by zero and array indices outside the declared bounds.

---

## 5. Execution Sequence Diagram

```mermaid
sequenceDiagram
    autonumber
    actor User
    participant Main as main()
    participant Tokenizer as Tokenizer
    participant Parser as Parser
    participant Evaluator as Evaluator

    User->>Main: Runs ./interpreter file.txt
    activate Main
    Main->>Tokenizer: Tokenizer(fileStream)
    Main->>Parser: Parser(tokenizer)
    Main->>Parser: parseProgram()
    activate Parser
    loop Syntax analysis
        Parser->>Tokenizer: getNextToken()
        Tokenizer-->>Parser: Token
    end
    Parser-->>Main: root (ASTNode*)
    deactivate Parser

    Main->>Evaluator: Evaluator()
    Main->>Evaluator: evaluate(root)
    activate Evaluator
    Evaluator->>Evaluator: Evaluates Decls (allocates memory)
    Evaluator->>Evaluator: Executes Stmts (while, if, assign, print)
    Evaluator-->>User: Prints results to the console
    Evaluator-->>Main: Finished successfully
    deactivate Evaluator

    Main-->>User: Returns exit code 0

    Note over Main,Evaluator: On the first lexical, syntax or runtime error, an exception reaches main(),<br/>which prints a single line "Error: description" and returns exit code 1.
    deactivate Main
```
