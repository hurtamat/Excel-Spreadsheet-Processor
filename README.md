# Spreadsheet Application

This is a C++ implementation of a simple spreadsheet application that supports arithmetic operations, cell references, and basic formula parsing. The application demonstrates how to parse and evaluate expressions, manage cell dependencies, and handle copy operations within a spreadsheet.

## Features

- **Arithmetic Operations**: Supports addition, subtraction, multiplication, division, and exponentiation.
- **Cell References**: Allows cells to reference other cells, including absolute (`$A$1`) and relative (`A1`) references.
- **Expression Parsing**: Parses mathematical expressions from stack into an Abstract Syntax Tree (AST) for evaluation.
- **String and Number Handling**: Cells can contain numbers, strings, or expressions.
- **Comparison Operations**: Supports comparison operators like equal (`=`), not equal (`<>`), less than (`<`), less than or equal (`<=`), greater than (`>`), and greater than or equal (`>=`).
- **Copying Cell Ranges**: Enables copying a range of cells from one location to another, adjusting cell references appropriately.
- **Serialization**: Provides functionality to load and save the spreadsheet to and from a stream.

## Dependencies

- **C++20**: The code utilizes features from the C++20 standard.

## Compilation

To compile the application, use the following command:

```bash
g++ -std=c++20 main.cpp -L./x86_64-linux-gnu -lexpression_parser -o spreadsheet_app
```

- `-std=c++20`: Specifies the C++ standard version.
- `main.cpp`: The main source file containing the application code.
- `-o spreadsheet_app`: Specifies the output executable name.

## Running the Application

After compilation, run the application using:

```bash
./spreadsheet_app
```

The application includes a series of assertions that act as tests to verify the functionality. If all tests pass, it will output:

```
TESTS SUCCESSFUL
```

## Code Structure

- **`CSpreadsheet`**: Represents the spreadsheet and manages cells.
  - `setCell(CPos pos, std::string contents)`: Sets the contents of a cell.
  - `getValue(CPos pos)`: Retrieves the value of a cell.
  - `copyRect(CPos dst, CPos src, int w = 1, int h = 1)`: Copies a rectangle of cells from source to destination.
  - `load(std::istream &is)`: Loads the spreadsheet from a stream.
  - `save(std::ostream &os)`: Saves the spreadsheet to a stream.

- **`CPos`**: Represents the position of a cell in the spreadsheet.
  - Parses positions like `"A1"` into column and row indices.

- **`cellContents`**: Holds the contents of a cell, which can be a value or an expression.

- **Expression Nodes (`ExprNode` and derived classes)**: Represents nodes in the expression tree (AST) for parsing and evaluating expressions.

