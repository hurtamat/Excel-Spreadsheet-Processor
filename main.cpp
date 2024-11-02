#include <cstdlib>
#include <cstring>
#include <cctype>
#include <cfloat>
#include <cassert>
#include <cmath>
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <array>
#include <utility>
#include <vector>
#include <list>
#include <set>
#include <map>
#include <stack>
#include <queue>
#include <unordered_set>
#include <unordered_map>
#include <memory>
#include <functional>
#include <stdexcept>
#include <variant>
#include <compare>
#include "expression.h"

using namespace std::literals;
using CValue = std::variant<std::monostate, double, std::string>;

constexpr unsigned SPREADSHEET_CYCLIC_DEPS = 0x01;
constexpr unsigned SPREADSHEET_FUNCTIONS = 0;
constexpr unsigned SPREADSHEET_FILE_IO = 0;
constexpr unsigned SPREADSHEET_SPEED = 0;
constexpr unsigned SPREADSHEET_PARSER = 0;

int counter;  // Global counter for cyclic dependency detection

class ExprNode;  // Forward declaration

class CPos;  // Position class representing cell positions

class cellContents;  // Class representing the contents of a cell

// Expression builder class for parsing and building expressions
class MyExprBuilder : public CExprBuilder {
private:
    std::stack<std::shared_ptr<ExprNode>> stack;  // Stack for expression nodes
    const std::map<CPos, std::shared_ptr<cellContents>> &arr;  // Reference to cell contents
public:
    MyExprBuilder(const std::map<CPos, std::shared_ptr<cellContents>> &array);

    MyExprBuilder(const MyExprBuilder &other, const std::map<CPos, std::shared_ptr<cellContents>> &array, int w, int h);

    void opAdd() override;

    void opSub() override;

    void opMul() override;

    void opDiv() override;

    void opPow() override;

    void opNeg() override;

    void opEq() override;

    void opNe() override;

    void opLt() override;

    void opLe() override;

    void opGt() override;

    void opGe() override;

    void valNumber(double val) override;

    void valString(std::string val) override;

    void valReference(std::string val) override;

    void valRange(std::string val) override;

    void funcCall(std::string fnName, int paramCount) override;

    std::shared_ptr<ExprNode> getRoot() const;
};

class CPos {
public:
    CPos(std::string_view str);

    CPos(int x, int y);

    CPos(const CPos &other);

    ~CPos();

    void setColumn(int input);

    void setRow(int input);

    CPos();

    std::strong_ordering operator<=>(const CPos &other) const;

    int getColumn() const;

    int getRow() const;

    std::string getReverseColumn() const;  // Converts column index back to letters

private:
    int column;
    int row;
};

CPos::CPos(std::string_view str) : column(0), row(0) {
    // Parses a string like "A1" into column and row numbers
    bool intPresent = false;
    bool charPresent = false;
    if (str.empty()) throw std::invalid_argument("Invalid_Argument");

    for (auto character: str) {
        character = std::toupper(character);
        if (character >= 'A' && character <= 'Z') {
            charPresent = true;
            if (intPresent) throw std::invalid_argument("Invalid_Argument");
            column = column * 26 + (character - 'A' + 1);
        } else if (character >= '0' && character <= '9') {
            intPresent = true;
            if (!charPresent) throw std::invalid_argument("Invalid_Argument");
            row = row * 10 + (character - '0');
        } else {
            throw std::invalid_argument("Invalid_Argument");
        }
    }
    if (!intPresent) throw std::invalid_argument("Invalid_Argument");
}

CPos::CPos(int x, int y) : column(x), row(y) {}

CPos::CPos(const CPos &other) : column(other.column), row(other.row) {}

CPos::~CPos() = default;

void CPos::setColumn(int input) {
    column = input;
}

void CPos::setRow(int input) {
    row = input;
}

CPos::CPos() : column(0), row(0) {}

std::strong_ordering CPos::operator<=>(const CPos &other) const {
    // Compares positions for sorting
    if (column == other.column) return row <=> other.row;
    return column <=> other.column;
}

int CPos::getColumn() const {
    return column;
}

int CPos::getRow() const {
    return row;
}

std::string CPos::getReverseColumn() const {
    // Converts column number back to letter representation (e.g., 1 -> "A")
    int tempColumn = column;
    std::string ret;
    while (tempColumn > 0) {
        int rest = (tempColumn - 1) % 26;
        tempColumn = (tempColumn - rest) / 26;
        ret = char(rest + 'A') + ret;
    }
    return ret;
}

class cellContents {
public:
    bool state;  // True if the cell contains an expression
    CValue val;  // Value of the cell if not an expression
    MyExprBuilder expression;  // Expression builder for the cell

    cellContents(std::string input, const std::map<CPos, std::shared_ptr<cellContents>> &array);

    cellContents(const cellContents &other, const std::map<CPos, std::shared_ptr<cellContents>> &array, int w, int h);

    CValue getResult() const;  // Evaluates and returns the cell value
};

// Abstract base class for expression nodes
class ExprNode {
public:
    virtual ~ExprNode() = default;

    virtual CValue eval() const = 0;  // Evaluates the expression node

    virtual std::shared_ptr<ExprNode>
    clone(const std::map<CPos, std::shared_ptr<cellContents>> &array, int w, int h) const = 0;

    virtual std::string toString(bool top) const = 0;  // Converts the expression to a string
};

// Expression node for numbers
class Number : public ExprNode {
public:
    Number(double input) {
        val = input;
    }

    CValue eval() const override {
        return CValue(val);
    }

    Number(const Number &other, const std::map<CPos, std::shared_ptr<cellContents>> &array, int w, int h) {
        val = other.val;
    }

    std::shared_ptr<ExprNode>
    clone(const std::map<CPos, std::shared_ptr<cellContents>> &array, int w, int h) const override {
        return std::make_shared<Number>(*this, array, w, h);
    }

    std::string toString(bool top) const override {
        std::ostringstream oss;
        if (top) oss << '=';
        oss << val;
        return oss.str();
    }

private:
    double val;
};

// Expression node for strings
class String : public ExprNode {
public:
    explicit String(std::string input) {
        val = input;
    }

    CValue eval() const override {
        return CValue(val);
    }

    String(const String &other, const std::map<CPos, std::shared_ptr<cellContents>> &array, int w, int h) {
        val = other.val;
    }

    std::shared_ptr<ExprNode>
    clone(const std::map<CPos, std::shared_ptr<cellContents>> &array, int w, int h) const override {
        return std::make_shared<String>(*this, array, w, h);
    }

    std::string toString(bool top) const override {
        std::ostringstream oss;
        oss << val;
        return oss.str();
    }

private:
    std::string val;
};

// Expression node for cell references
class Reference : public ExprNode {
public:
    // Parses a cell reference, handling fixed positions with '$'
    Reference(std::string input, const std::map<CPos, std::shared_ptr<cellContents>> &array) : arr(array) {
        CPos temp;
        bool intPresent = false;
        bool charPresent = false;
        fixed1 = false;
        fixed2 = false;
        if (input.empty()) throw std::invalid_argument("Invalid_Argument");

        for (size_t i = 0; i < input.size(); i++) {
            input[i] = std::toupper(input[i]);

            if (input[i] == '$') {
                if (charPresent == false && i == 0 && fixed1 == false) {
                    fixed1 = true;
                } else if ((intPresent == false && fixed2 == false)) {
                    if (charPresent == false) throw std::invalid_argument("Invalid_Argument");
                    fixed2 = true;
                } else throw std::invalid_argument("Invalid_Argument");

            } else if (input[i] >= 65 && input[i] <= 90) {
                charPresent = true;
                if (intPresent) throw std::invalid_argument("Invalid_Argument");
                temp.setColumn(temp.getColumn() * 26 + (input[i] - 'A' + 1));

            } else if (input[i] >= 48 && input[i] <= 57) {
                intPresent = true;
                if (!charPresent) throw std::invalid_argument("Invalid_Argument");
                temp.setRow(temp.getRow() * 10 + (input[i] - '0'));
            } else {
                throw std::invalid_argument("Invalid_Argument");
            }
        }
        if (!intPresent) throw std::invalid_argument("Invalid_Argument");
        position = temp;
    }

    Reference(const Reference &other, const std::map<CPos, std::shared_ptr<cellContents>> &array, int w, int h) :
            arr(array), position(other.position), fixed1(other.fixed1), fixed2(other.fixed2)
    {
        // Adjusts the position based on fixed flags and offset
        if ((!fixed1 || !fixed2))
        {
            if (!fixed1) {
                position.setColumn(other.position.getColumn() + w);
            }
            if (!fixed2) {
                position.setRow(other.position.getRow() + h);
            }
        }
    }

    CValue eval() const override {
        // Evaluates the referenced cell's value
        auto it = arr.find(position);

        if (it != arr.end()) {
            counter++;
            if (counter > 500) throw std::invalid_argument("Invalid_Argument");
            return it->second->getResult();
        }
        return CValue();
    }

    std::shared_ptr<ExprNode>
    clone(const std::map<CPos, std::shared_ptr<cellContents>> &array, int w, int h) const override {
        return std::make_shared<Reference>(*this, array, w, h);
    }

    std::string toString(bool top) const override {
        // Converts the reference back to string format
        std::ostringstream oss;
        if (top) oss << '=';
        if (fixed1) oss << "$";
        oss << position.getReverseColumn();
        if (fixed2) oss << "$";
        oss << position.getRow();
        return oss.str();
    }

private:
    const std::map<CPos, std::shared_ptr<cellContents>> &arr;
    CPos position;
    bool fixed1, fixed2;
};

// Expression node for addition
class Addition : public ExprNode {
public:
    Addition(std::stack<std::shared_ptr<ExprNode>> &stack) {
        right = std::move(stack.top());
        stack.pop();
        left = std::move(stack.top());
        stack.pop();
    }

    Addition(const Addition &other, const std::map<CPos, std::shared_ptr<cellContents>> &array, int w, int h)
            : left(other.left->clone(array, w, h)), right(other.right->clone(array, w, h)) {}

    CValue eval() const override {
        // Evaluates addition, handling both numbers and strings
        CValue l = left->eval();
        CValue r = right->eval();

        if (l == CValue() || r == CValue()) return CValue();

        if (std::holds_alternative<std::string>(l) || std::holds_alternative<std::string>(r)) {
            if (std::holds_alternative<std::string>(l) && std::holds_alternative<std::string>(r)) {
                return std::get<std::string>(l) + std::get<std::string>(r);
            } else if (std::holds_alternative<std::string>(l)) {
                return std::get<std::string>(l) + std::to_string(std::get<double>(r));
            } else {
                return std::get<std::string>(r) + std::to_string(std::get<double>(l));
            }
        }
        if (std::holds_alternative<double>(l) && std::holds_alternative<double>(r)) {
            return std::get<double>(l) + std::get<double>(r);
        }

        return CValue();
    }

    std::shared_ptr<ExprNode>
    clone(const std::map<CPos, std::shared_ptr<cellContents>> &array, int w, int h) const override {
        return std::make_shared<Addition>(*this, array, w, h);
    }

    std::string toString(bool top) const override {
        // Converts addition back to string format
        std::ostringstream oss;
        if (top) oss << '=';
        oss << '(' << left->toString(false) << '+' << right->toString(false) << ')';
        return oss.str();
    }

private:
    std::shared_ptr<ExprNode> left;
    std::shared_ptr<ExprNode> right;
};

class Substraction : public ExprNode {
public:
    Substraction(std::stack<std::shared_ptr<ExprNode>> &stack) {
        right = std::move(stack.top());
        stack.pop();
        left = std::move(stack.top());
        stack.pop();

    }

    Substraction(const Substraction &other, const std::map<CPos, std::shared_ptr<cellContents>> &array, int w, int h)
            : left(other.left->clone(array, w, h)), right(other.right->clone(array, w, h)) {}

    CValue eval() const override {

        CValue l = left->eval();
        CValue r = right->eval();
        if (l == CValue() || r == CValue()) return CValue();
        if (std::holds_alternative<double>(l) && std::holds_alternative<double>(r)) {
            return std::get<double>(l) - std::get<double>(r);
        }

        return CValue();

    }

    std::shared_ptr<ExprNode>
    clone(const std::map<CPos, std::shared_ptr<cellContents>> &array, int w, int h) const override {
        return std::make_shared<Substraction>(*this, array, w, h);
    }

    std::string toString(bool top) const override {
        std::ostringstream oss;
        if (top) oss << '=';
        oss << '(' << left->toString(false) << '-' << right->toString(false) << ')';
        return oss.str();
    }

private:
    std::shared_ptr<ExprNode> left;
    std::shared_ptr<ExprNode> right;
};

class Multiplication : public ExprNode {
public:
    Multiplication(std::stack<std::shared_ptr<ExprNode>> &stack) {
        right = std::move(stack.top());
        stack.pop();
        left = std::move(stack.top());
        stack.pop();

    }

    Multiplication(const Multiplication &other, const std::map<CPos, std::shared_ptr<cellContents>> &array, int w,int h)
            : left(other.left->clone(array, w, h)), right(other.right->clone(array, w, h)) {}

    CValue eval() const override {

        CValue l = left->eval();
        CValue r = right->eval();
        if (l == CValue() || r == CValue()) return CValue();

        if (std::holds_alternative<double>(l) && std::holds_alternative<double>(r)) {
            return std::get<double>(l) * std::get<double>(r);
        }

        return CValue();

    }

    std::shared_ptr<ExprNode>
    clone(const std::map<CPos, std::shared_ptr<cellContents>> &array, int w, int h) const override {
        return std::make_shared<Multiplication>(*this, array, w, h);
    }

    std::string toString(bool top) const override {
        std::ostringstream oss;
        if (top) oss << '=';
        oss << '(' << left->toString(false) << '*' << right->toString(false) << ')';
        return oss.str();
    }

private:
    std::shared_ptr<ExprNode> left;
    std::shared_ptr<ExprNode> right;
};

class Division : public ExprNode {
public:
    Division(std::stack<std::shared_ptr<ExprNode>> &stack) {
        right = std::move(stack.top());
        stack.pop();
        left = std::move(stack.top());
        stack.pop();

    }

    Division(const Division &other, const std::map<CPos, std::shared_ptr<cellContents>> &array, int w, int h)
            : left(other.left->clone(array, w, h)), right(other.right->clone(array, w, h)) {}

    CValue eval() const override {

        CValue l = left->eval();
        CValue r = right->eval();
        if (l == CValue() || r == CValue()) return CValue();
        if (std::holds_alternative<double>(l) && std::holds_alternative<double>(r)) {
            if (std::get<double>(r) == 0) return CValue();
            return std::get<double>(l) / std::get<double>(r);
        }

        return CValue();

    }

    std::shared_ptr<ExprNode>
    clone(const std::map<CPos, std::shared_ptr<cellContents>> &array, int w, int h) const override {
        return std::make_shared<Division>(*this, array, w, h);
    }

    std::string toString(bool top) const override {
        std::ostringstream oss;
        if (top) oss << '=';
        oss << '(' << left->toString(false) << '/' << right->toString(false) << ')';
        return oss.str();
    }

private:
    std::shared_ptr<ExprNode> left;
    std::shared_ptr<ExprNode> right;
};

class Power : public ExprNode {
public:
    Power(std::stack<std::shared_ptr<ExprNode>> &stack) {
        right = std::move(stack.top());
        stack.pop();
        left = std::move(stack.top());
        stack.pop();

    }

    Power(const Power &other, const std::map<CPos, std::shared_ptr<cellContents>> &array, int w, int h)
            : left(other.left->clone(array, w, h)), right(other.right->clone(array, w, h)) {}

    CValue eval() const override {

        CValue l = left->eval();
        CValue r = right->eval();
        if (l == CValue() || r == CValue()) return CValue();
        if (std::holds_alternative<double>(l) && std::holds_alternative<double>(r)) {
            return std::pow(std::get<double>(l), std::get<double>(r));
        }

        return CValue();

    }

    std::string toString(bool top) const override {
        std::ostringstream oss;
        if (top) oss << '=';
        oss << '(' << left->toString(false) << '^' << right->toString(false) << ')';
        return oss.str();
    }

    std::shared_ptr<ExprNode>
    clone(const std::map<CPos, std::shared_ptr<cellContents>> &array, int w, int h) const override {
        return std::make_shared<Power>(*this, array, w, h);
    }

private:
    std::shared_ptr<ExprNode> left;
    std::shared_ptr<ExprNode> right;

};

class Negation : public ExprNode {
public:
    Negation(std::stack<std::shared_ptr<ExprNode>> &stack) {
        single = std::move(stack.top());
        stack.pop();


    }

    Negation(const Negation &other, const std::map<CPos, std::shared_ptr<cellContents>> &array, int w, int h)
            : single(other.single->clone(array, w, h)) {}


    CValue eval() const override {
        CValue s = single->eval();
        if (s == CValue()) return CValue();
        if (std::holds_alternative<double>(s)) {
            return std::get<double>(s) * -1;
        }
        return CValue();
    }

    std::shared_ptr<ExprNode>
    clone(const std::map<CPos, std::shared_ptr<cellContents>> &array, int w, int h) const override {
        return std::make_shared<Negation>(*this, array, w, h);
    }

    std::string toString(bool top) const override {
        std::ostringstream oss;
        if (top) oss << '=';
        oss << '(' << '-' << single->toString(false) << ')';
        return oss.str();
    }


private:
    std::shared_ptr<ExprNode> single;
};

class Equal : public ExprNode {
public:
    Equal(std::stack<std::shared_ptr<ExprNode>> &stack) {
        right = std::move(stack.top());
        stack.pop();
        left = std::move(stack.top());
        stack.pop();

    }

    Equal(const Equal &other, const std::map<CPos, std::shared_ptr<cellContents>> &array, int w, int h)
            : left(other.left->clone(array, w, h)), right(other.right->clone(array, w, h)) {}


    CValue eval() const override {

        CValue l = left->eval();
        CValue r = right->eval();
        if (l == CValue() || r == CValue()) return CValue();
        if (std::holds_alternative<std::string>(l) && std::holds_alternative<std::string>(r)) {
            if (std::get<std::string>(l) == std::get<std::string>(r)) return 1.0;
            return 0.0;
        }

        if (std::holds_alternative<double>(l) && std::holds_alternative<double>(r)) {
            if (std::get<double>(l) == std::get<double>(r)) return 1.0;
            return 0.0;

        }

        return CValue();

    }

    std::shared_ptr<ExprNode>
    clone(const std::map<CPos, std::shared_ptr<cellContents>> &array, int w, int h) const override {
        return std::make_shared<Equal>(*this, array, w, h);
    }

    std::string toString(bool top) const override {
        std::ostringstream oss;
        if (top) oss << '=';
        oss << '(' << left->toString(false) << '=' << right->toString(false) << ')';
        return oss.str();
    }

private:
    std::shared_ptr<ExprNode> left;
    std::shared_ptr<ExprNode> right;
};

class NotEqual : public ExprNode {
public:
    NotEqual(std::stack<std::shared_ptr<ExprNode>> &stack) {
        right = stack.top();
        stack.pop();
        left = stack.top();
        stack.pop();

    }

    NotEqual(const NotEqual &other, const std::map<CPos, std::shared_ptr<cellContents>> &array, int w, int h)
            : left(other.left->clone(array, w, h)), right(other.right->clone(array, w, h)) {}

    CValue eval() const override {
        CValue l = left->eval();
        CValue r = right->eval();
        if (l == CValue() || r == CValue()) return CValue();
        if (std::holds_alternative<std::string>(l) && std::holds_alternative<std::string>(r)) {
            if (std::get<std::string>(l) == std::get<std::string>(r)) return 0.0;
            return 1.0;
        }
        if (std::holds_alternative<double>(l) && std::holds_alternative<double>(r)) {
            if (std::get<double>(l) == std::get<double>(r)) return 0.0;
            return 1.0;

        }
        return CValue();
    }

    std::shared_ptr<ExprNode>
    clone(const std::map<CPos, std::shared_ptr<cellContents>> &array, int w, int h) const override {
        return std::make_shared<NotEqual>(*this, array, w, h);
    }

    std::string toString(bool top) const override {
        std::ostringstream oss;
        if (top) oss << '=';
        oss << '(' << left->toString(false) << "<>" << right->toString(false) << ')';
        return oss.str();
    }


private:
    std::shared_ptr<ExprNode> left;
    std::shared_ptr<ExprNode> right;
};

class LessThan : public ExprNode {
public:
    LessThan(std::stack<std::shared_ptr<ExprNode>> &stack) {
        right = std::move(stack.top());
        stack.pop();
        left = std::move(stack.top());
        stack.pop();

    }

    LessThan(const LessThan &other, const std::map<CPos, std::shared_ptr<cellContents>> &array, int w, int h)
            : left(other.left->clone(array, w, h)), right(other.right->clone(array, w, h)) {}


    CValue eval() const override {
        CValue l = left->eval();
        CValue r = right->eval();
        if (l == CValue() || r == CValue()) return CValue();
        if (std::holds_alternative<std::string>(l) && std::holds_alternative<std::string>(r)) {
            if (std::get<std::string>(l) < std::get<std::string>(r)) return 1.0;
            return 0.0;
        }
        if (std::holds_alternative<double>(l) && std::holds_alternative<double>(r)) {
            if (std::get<double>(l) < std::get<double>(r)) return 1.0;
            return 0.0;

        }
        return CValue();
    }

    std::shared_ptr<ExprNode>
    clone(const std::map<CPos, std::shared_ptr<cellContents>> &array, int w, int h) const override {
        return std::make_shared<LessThan>(*this, array, w, h);
    }

    std::string toString(bool top) const override {
        std::ostringstream oss;
        if (top) oss << '=';
        oss << '(' << left->toString(false) << '<' << right->toString(false) << ')';
        return oss.str();
    }

private:
    std::shared_ptr<ExprNode> left;
    std::shared_ptr<ExprNode> right;
};

class LessEqual : public ExprNode {
public:
    LessEqual(std::stack<std::shared_ptr<ExprNode>> &stack) {
        right = std::move(stack.top());
        stack.pop();
        left = std::move(stack.top());
        stack.pop();

    }

    LessEqual(const LessEqual &other, const std::map<CPos, std::shared_ptr<cellContents>> &array, int w, int h)
            : left(other.left->clone(array, w, h)), right(other.right->clone(array, w, h)) {}

    CValue eval() const override {
        CValue l = left->eval();
        CValue r = right->eval();
        if (l == CValue() || r == CValue()) return CValue();
        if (std::holds_alternative<std::string>(l) && std::holds_alternative<std::string>(r)) {
            if (std::get<std::string>(l) <= std::get<std::string>(r)) return 1.0;
            return 0.0;
        }
        if (std::holds_alternative<double>(l) && std::holds_alternative<double>(r)) {
            if (std::get<double>(l) <= std::get<double>(r)) return 1.0;
            return 0.0;

        }
        return CValue();
    }

    std::shared_ptr<ExprNode>
    clone(const std::map<CPos, std::shared_ptr<cellContents>> &array, int w, int h) const override {
        return std::make_shared<LessEqual>(*this, array, w, h);
    }

    std::string toString(bool top) const override {
        std::ostringstream oss;
        if (top) oss << '=';
        oss << '(' << left->toString(false) << "<=" << right->toString(false) << ')';
        return oss.str();
    }

private:
    std::shared_ptr<ExprNode> left;
    std::shared_ptr<ExprNode> right;
};

class GreaterThan : public ExprNode {
public:
    GreaterThan(std::stack<std::shared_ptr<ExprNode>> &stack) {
        right = std::move(stack.top());
        stack.pop();
        left = std::move(stack.top());
        stack.pop();

    }

    GreaterThan(const GreaterThan &other, const std::map<CPos, std::shared_ptr<cellContents>> &array, int w, int h)
            : left(other.left->clone(array, w, h)), right(other.right->clone(array, w, h)) {}


    CValue eval() const override {
        CValue l = left->eval();
        CValue r = right->eval();
        if (l == CValue() || r == CValue()) return CValue();
        if (std::holds_alternative<std::string>(l) && std::holds_alternative<std::string>(r)) {
            if (std::get<std::string>(l) > std::get<std::string>(r)) return 1.0;
            return 0.0;
        }
        if (std::holds_alternative<double>(l) && std::holds_alternative<double>(r)) {
            if (std::get<double>(l) > std::get<double>(r)) return 1.0;
            return 0.0;

        }
        return CValue();
    }


    std::shared_ptr<ExprNode>
    clone(const std::map<CPos, std::shared_ptr<cellContents>> &array, int w, int h) const override {
        return std::make_shared<GreaterThan>(*this, array, w, h);
    }

    std::string toString(bool top) const override {
        std::ostringstream oss;
        if (top) oss << '=';
        oss << '(' << left->toString(false) << '>' << right->toString(false) << ')';
        return oss.str();
    }

private:
    std::shared_ptr<ExprNode> left;
    std::shared_ptr<ExprNode> right;
};

class GreaterEqual : public ExprNode {
public:
    GreaterEqual(std::stack<std::shared_ptr<ExprNode>> &stack) {
        right = std::move(stack.top());
        stack.pop();
        left = std::move(stack.top());
        stack.pop();

    }

    GreaterEqual(const GreaterEqual &other, const std::map<CPos, std::shared_ptr<cellContents>> &array, int w, int h)
            : left(other.left->clone(array, w, h)), right(other.right->clone(array, w, h)) {}


    CValue eval() const override {
        CValue l = left->eval();
        CValue r = right->eval();
        if (l == CValue() || r == CValue()) return CValue();
        if (std::holds_alternative<std::string>(l) && std::holds_alternative<std::string>(r)) {
            if (std::get<std::string>(l) >= std::get<std::string>(r)) return 1.0;
            return 0.0;
        }
        if (std::holds_alternative<double>(l) && std::holds_alternative<double>(r)) {
            if (std::get<double>(l) >= std::get<double>(r)) return 1.0;
            return 0.0;

        }
        return CValue();
    }

    std::shared_ptr<ExprNode>
    clone(const std::map<CPos, std::shared_ptr<cellContents>> &array, int w, int h) const override {
        return std::make_shared<GreaterEqual>(*this, array, w, h);
    }

    std::string toString(bool top) const override {
        std::ostringstream oss;
        if (top) oss << '=';
        oss << '(' << left->toString(false) << ">=" << right->toString(false) << ')';
        return oss.str();
    }

private:
    std::shared_ptr<ExprNode> left;
    std::shared_ptr<ExprNode> right;
};


MyExprBuilder::MyExprBuilder(const std::map<CPos, std::shared_ptr<cellContents>> &array) : arr(array) {}

MyExprBuilder::MyExprBuilder(const MyExprBuilder &other, const std::map<CPos, std::shared_ptr<cellContents>> &array,
                             int w, int h)
        : arr(array) {
    if (!other.stack.empty()) {
        stack.push(other.stack.top()->clone(arr, w, h));
    }

}

//the derived classes take from the stack themselves
void MyExprBuilder::opAdd() {
    if (stack.size() < 2) throw std::invalid_argument("Not enough on stack");
    stack.push(std::make_shared<Addition>(stack));
}

void MyExprBuilder::opSub() {
    if (stack.size() < 2) throw std::invalid_argument("Not enough on stack");
    stack.push(std::make_shared<Substraction>(stack));
}

void MyExprBuilder::opMul() {
    if (stack.size() < 2) throw std::invalid_argument("Not enough on stack");
    stack.push(std::make_shared<Multiplication>(stack));
}

void MyExprBuilder::opDiv() {
    if (stack.size() < 2) throw std::invalid_argument("Not enough on stack");
    stack.push(std::make_shared<Division>(stack));
}

void MyExprBuilder::opPow() {
    if (stack.size() < 2) throw std::invalid_argument("Not enough on stack");
    stack.push(std::make_shared<Power>(stack));
}


void MyExprBuilder::opNeg() {
    if (stack.size() < 1) throw std::invalid_argument("Not enough on stack");
    stack.push(std::make_shared<Negation>(stack));
}

void MyExprBuilder::opEq() {
    if (stack.size() < 2) throw std::invalid_argument("Not enough on stack");
    stack.push(std::make_shared<Equal>(stack));
}

void MyExprBuilder::opNe() {
    if (stack.size() < 2) throw std::invalid_argument("Not enough on stack");
    stack.push(std::make_shared<NotEqual>(stack));
}

void MyExprBuilder::opLt() {
    if (stack.size() < 2) throw std::invalid_argument("Not enough on stack");
    stack.push(std::make_shared<LessThan>(stack));
}

void MyExprBuilder::opLe() {
    if (stack.size() < 2) throw std::invalid_argument("Not enough on stack");
    stack.push(std::make_shared<LessEqual>(stack));
}

void MyExprBuilder::opGt() {
    if (stack.size() < 2) throw std::invalid_argument("Not enough on stack");
    stack.push(std::make_shared<GreaterThan>(stack));
}

void MyExprBuilder::opGe() {
    if (stack.size() < 2) throw std::invalid_argument("Not enough on stack");
    stack.push(std::make_shared<GreaterEqual>(stack));
}

void MyExprBuilder::valNumber(double val) {
    stack.push(std::make_shared<Number>(val));
}

void MyExprBuilder::valString(std::string val) {
    stack.push(std::make_shared<String>(std::move(val)));

}

// arr to be able to copy correctly
void MyExprBuilder::valReference(std::string val) {
    stack.push(std::make_shared<Reference>(std::move(val), arr));
}

void MyExprBuilder::valRange(std::string val) {
//not finished
}

void MyExprBuilder::funcCall(std::string fnName, int paramCount) {
//not finished
}

std::shared_ptr<ExprNode> MyExprBuilder::getRoot() const {
    if (stack.empty()) {
        throw std::invalid_argument("stack empty");
    }
    if (stack.size() > 1) {
        throw std::invalid_argument("incorrect input, elements > 1");
    }
    return std::move(stack.top());
}

// Helper function to check if a string is a number
bool is_number(const std::string &s) {
    char *end = nullptr;
    double val = strtod(s.c_str(), &end);
    return end != s.c_str() && *end == '\0' && val != HUGE_VAL;
}

// Determines if the input is an expression, string, or number
cellContents::cellContents(std::string input, const std::map<CPos, std::shared_ptr<cellContents>> &array) : expression(
        array) {
    if (input[0] == '=') {
        parseExpression(input, expression);
        state = true;
        return;
    }
    if (input.empty()) {
        val = "";
        state = false;
        return;
    }

    state = false;
    if (is_number(input)) {
        val = std::stod(input);
        return;
    } else {
        val = input;
    }
}

cellContents::cellContents(const cellContents &other, const std::map<CPos, std::shared_ptr<cellContents>> &array, int w, int h)
        : expression(other.expression, array, w, h) {
    state = other.state;
    val = other.val;
}

CValue cellContents::getResult() const {
    if (state == false) {
        return val;
    } else {
        return expression.getRoot()->eval();
    }
}

class CSpreadsheet {
public:
    static unsigned capabilities() {
        return SPREADSHEET_CYCLIC_DEPS | SPREADSHEET_FUNCTIONS | SPREADSHEET_FILE_IO | SPREADSHEET_SPEED |
               SPREADSHEET_PARSER;
    }

    CSpreadsheet() = default;

    // Assignment operator
    CSpreadsheet &operator=(const CSpreadsheet &other) {
        array.clear();
        for (const auto &cell: other.array) {
            array[cell.first] = std::make_shared<cellContents>(*cell.second, array, 0, 0);
        }
        return *this;
    }

    CSpreadsheet(const CSpreadsheet &other) {
        for (auto cell: other.array) {
            array[cell.first] = std::make_shared<cellContents>(*cell.second, array, 0, 0);
        }
    }

    CSpreadsheet(CSpreadsheet &&other) noexcept: array(std::move(other.array)) {
        other.array.clear();
    }

    // Loads the spreadsheet from a stream
    bool load(std::istream &is) {
        bool bracket = false;
        int state = 0;
        array.clear();

        std::string position = "";
        std::string length = "";
        std::string expr = "";

        while (is.peek() != std::istream::traits_type::eof()) {
            char cur;
            is.get(cur);
            if (cur == '(' && state == 0) {
                if (bracket) return false;
                bracket = true;
                state = 1;
            } else if (cur == ')' && state == 3) {
                if (!bracket) return false;
                bracket = false;
                array[CPos(position)] = std::make_shared<cellContents>(expr, array);
                state = 0;
                length = "";
                position = "";
                expr = "";

            } else if (cur == ';') {
                if (state > 3) return false;
                state++;
                if (state == 3) {
                    for (int i = 0; i < std::stoi(length); ++i) {
                        is.get(cur);
                        expr += cur;
                    }
                }

            } else if (cur == ' ' && state == 0) {
                if (bracket) return false;
            } else {
                if (state == 1) {
                    position += cur;
                } else if (state == 2) {
                    length += cur;
                } else return false;
            }
        }
        return true;
    }

    // Saves the spreadsheet to a stream
    bool save(std::ostream &os) const {
        if (!os) {
            return false;
        }

        for (const auto &cell: array) {
            os << "(" << cell.first.getReverseColumn() << cell.first.getRow() << ';';
            if (cell.second->state) {
                std::string temp = cell.second->expression.getRoot()->toString(true);
                int size = temp.size();
                os << size << ';';
                os << temp;
            } else {
                if (std::holds_alternative<double>(cell.second->val)) {
                    double temp = std::get<double>(cell.second->val);
                    std::string size = std::to_string(temp);
                    int sizer = size.size();
                    os << sizer << ';';
                    os << size;
                } else if (std::holds_alternative<std::string>(cell.second->val)) {
                    std::string temp = std::get<std::string>(cell.second->val);
                    int size = temp.size();
                    os << size << ';';
                    os << temp;
                } else {
                    os << '0' << ';';
                }
            }
            os << ") ";
        }
        if (!os) {
            return false;
        }
        return true;
    }

    // Sets the contents of a cell
    bool setCell(CPos pos, std::string contents) {
        if (contents.empty()) return false;
        try {
            array[pos] = std::make_shared<cellContents>(contents, array);
        }
        catch (...) {
            return false;
        }
        return true;
    }

    // Gets the value of a cell
    CValue getValue(CPos pos) {
        auto somepos = array.find(pos);
        if (somepos == array.end()) {
            return CValue();
        }
        counter = 0;
        try {
            return somepos->second->getResult();
        }
        catch (...) {
            return CValue();
        }
    }

    // Copies a rectangle of cells from source to destination
    void copyRect(CPos dst, CPos src, int w = 1, int h = 1) {
        if (w == 0 || h == 0) return;
        int xstart = src.getColumn();
        int xend = src.getColumn() + w;
        int ystart = src.getRow();
        int yend = src.getRow() + h;
        int xmove = dst.getColumn() - src.getColumn();
        int ymove = dst.getRow() - src.getRow();

        std::map<CPos, std::shared_ptr<cellContents>> tempArray;

        // Copies cells from source rectangle
        for (const auto &cell: array) {
            if (!(cell.first.getColumn() >= xstart && cell.first.getColumn() < xend
                  && cell.first.getRow() >= ystart && cell.first.getRow() < yend)) {
                continue;
            }
            tempArray[CPos(cell.first.getColumn() + xmove, cell.first.getRow() + ymove)] =
                    std::make_shared<cellContents>(*cell.second, array, xmove, ymove);
        }

        xstart = dst.getColumn();
        xend = dst.getColumn() + w;
        ystart = dst.getRow();
        yend = dst.getRow() + h;

        // Deletes destination rectangle cells
        for (auto cell = array.begin(); cell != array.end();) {
            if (cell->first.getColumn() >= xstart && cell->first.getColumn() < xend &&
                cell->first.getRow() >= ystart && cell->first.getRow() < yend) {
                if (tempArray.find(cell->first) == tempArray.end()) {
                    cell = array.erase(cell);
                } else {
                    ++cell;
                }
            } else {
                ++cell;
            }
        }

        // Inserts copied cells into the array
        for (const auto &cell: tempArray) {
            array[cell.first] = std::make_shared<cellContents>(*cell.second, array, 0, 0);
        }
    }

private:
    std::map<CPos, std::shared_ptr<cellContents>> array;  // Map of cell positions to contents
};

bool valueMatch(const CValue &r, const CValue &s) {
    // Compares two CValues for equality
    if (r.index() != s.index())
        return false;
    if (r.index() == 0)
        return true;
    if (r.index() == 2)
        return std::get<std::string>(r) == std::get<std::string>(s);
    if (std::isnan(std::get<double>(r)) && std::isnan(std::get<double>(s)))
        return true;
    if (std::isinf(std::get<double>(r)) && std::isinf(std::get<double>(s)))
        return (std::get<double>(r) < 0 && std::get<double>(s) < 0)
               || (std::get<double>(r) > 0 && std::get<double>(s) > 0);
    return fabs(std::get<double>(r) - std::get<double>(s)) <= 1e8 * DBL_EPSILON * fabs(std::get<double>(r));
}

int main() {
    CSpreadsheet x0, x1;
    std::ostringstream oss;
    std::istringstream iss;
    std::string data;


    assert (x0.setCell(CPos("A1"), "10"));
    assert (x0.setCell(CPos("A2"), "20.5"));
    assert (x0.setCell(CPos("A3"), "3e1"));
    assert (x0.setCell(CPos("A4"), "=40"));
    assert (x0.setCell(CPos("A5"), "=5e+1"));
    assert (x0.setCell(CPos("A6"), "raw text with any characters, including a quote \" or a newline\n"));
    assert (x0.setCell(CPos("A7"),
                       "=\"quoted string, quotes must be doubled: \"\". Moreover, backslashes are needed for C++.\""));
    assert (valueMatch(x0.getValue(CPos("A1")), CValue(10.0)));
    assert (valueMatch(x0.getValue(CPos("A2")), CValue(20.5)));
    assert (valueMatch(x0.getValue(CPos("A3")), CValue(30.0)));
    assert (valueMatch(x0.getValue(CPos("A4")), CValue(40.0)));
    assert (valueMatch(x0.getValue(CPos("A5")), CValue(50.0)));
    assert (valueMatch(x0.getValue(CPos("A6")),
                       CValue("raw text with any characters, including a quote \" or a newline\n")));
    assert (valueMatch(x0.getValue(CPos("A7")),
                       CValue("quoted string, quotes must be doubled: \". Moreover, backslashes are needed for C++.")));
    assert (valueMatch(x0.getValue(CPos("A8")), CValue()));
    assert (valueMatch(x0.getValue(CPos("AAAA9999")), CValue()));
    assert (x0.setCell(CPos("B1"), "=A1+A2*A3"));
    assert (x0.setCell(CPos("B2"), "= -A1 ^ 2 - A2 / 2   "));
    assert (x0.setCell(CPos("B3"), "= 2 ^ $A$1"));
    assert (x0.setCell(CPos("B4"), "=($A1+A$2)^2"));
    assert (x0.setCell(CPos("B5"), "=B1+B2+B3+B4"));
    assert (x0.setCell(CPos("B6"), "=B1+B2+B3+B4+B5"));
    assert (valueMatch(x0.getValue(CPos("B1")), CValue(625.0)));
    assert (valueMatch(x0.getValue(CPos("B2")), CValue(-110.25)));
    assert (valueMatch(x0.getValue(CPos("B3")), CValue(1024.0)));
    assert (valueMatch(x0.getValue(CPos("B4")), CValue(930.25)));
    assert (valueMatch(x0.getValue(CPos("B5")), CValue(2469.0)));
    assert (valueMatch(x0.getValue(CPos("B6")), CValue(4938.0)));
    assert (x0.setCell(CPos("A1"), "12"));
    assert (valueMatch(x0.getValue(CPos("B1")), CValue(627.0)));
    assert (valueMatch(x0.getValue(CPos("B2")), CValue(-154.25)));
    assert (valueMatch(x0.getValue(CPos("B3")), CValue(4096.0)));
    assert (valueMatch(x0.getValue(CPos("B4")), CValue(1056.25)));
    assert (valueMatch(x0.getValue(CPos("B5")), CValue(5625.0)));
    assert (valueMatch(x0.getValue(CPos("B6")), CValue(11250.0)));
    x1 = x0;
    assert (x0.setCell(CPos("A2"), "100"));
    assert (x1.setCell(CPos("A2"), "=A3+A5+A4"));
    assert (valueMatch(x0.getValue(CPos("B1")), CValue(3012.0)));
    assert (valueMatch(x0.getValue(CPos("B2")), CValue(-194.0)));
    assert (valueMatch(x0.getValue(CPos("B3")), CValue(4096.0)));
    assert (valueMatch(x0.getValue(CPos("B4")), CValue(12544.0)));
    assert (valueMatch(x0.getValue(CPos("B5")), CValue(19458.0)));
    assert (valueMatch(x0.getValue(CPos("B6")), CValue(38916.0)));
    assert (valueMatch(x1.getValue(CPos("B1")), CValue(3612.0)));
    assert (valueMatch(x1.getValue(CPos("B2")), CValue(-204.0)));
    assert (valueMatch(x1.getValue(CPos("B3")), CValue(4096.0)));
    assert (valueMatch(x1.getValue(CPos("B4")), CValue(17424.0)));
    assert (valueMatch(x1.getValue(CPos("B5")), CValue(24928.0)));
    assert (valueMatch(x1.getValue(CPos("B6")), CValue(49856.0)));
    oss.clear();
    oss.str("");
    assert (x0.save(oss));
    data = oss.str();
    iss.clear();
    iss.str(data);
    assert (x1.load(iss));
    assert (valueMatch(x1.getValue(CPos("B1")), CValue(3012.0)));
    assert (valueMatch(x1.getValue(CPos("B2")), CValue(-194.0)));
    assert (valueMatch(x1.getValue(CPos("B3")), CValue(4096.0)));
    assert (valueMatch(x1.getValue(CPos("B4")), CValue(12544.0)));
    assert (valueMatch(x1.getValue(CPos("B5")), CValue(19458.0)));
    assert (valueMatch(x1.getValue(CPos("B6")), CValue(38916.0)));
    assert (x0.setCell(CPos("A3"), "4e1"));
    assert (valueMatch(x1.getValue(CPos("B1")), CValue(3012.0)));
    assert (valueMatch(x1.getValue(CPos("B2")), CValue(-194.0)));
    assert (valueMatch(x1.getValue(CPos("B3")), CValue(4096.0)));
    assert (valueMatch(x1.getValue(CPos("B4")), CValue(12544.0)));
    assert (valueMatch(x1.getValue(CPos("B5")), CValue(19458.0)));
    assert (valueMatch(x1.getValue(CPos("B6")), CValue(38916.0)));
    oss.clear();
    oss.str("");
    assert (x0.save(oss));
    data = oss.str();
    for (size_t i = 0; i < std::min<size_t>(data.length(), 10); i++)
        data[i] ^= 0x5a;
    iss.clear();
    iss.str(data);
    assert (!x1.load(iss));
    assert (x0.setCell(CPos("D0"), "10"));
    assert (x0.setCell(CPos("D1"), "20"));
    assert (x0.setCell(CPos("D2"), "30"));
    assert (x0.setCell(CPos("D3"), "40"));
    assert (x0.setCell(CPos("D4"), "50"));
    assert (x0.setCell(CPos("E0"), "60"));
    assert (x0.setCell(CPos("E1"), "70"));
    assert (x0.setCell(CPos("E2"), "80"));
    assert (x0.setCell(CPos("E3"), "90"));
    assert (x0.setCell(CPos("E4"), "100"));
    assert (x0.setCell(CPos("F10"), "=D0+5"));
    assert (x0.setCell(CPos("F11"), "=$D0+5"));
    assert (x0.setCell(CPos("F12"), "=D$0+5"));
    assert (x0.setCell(CPos("F13"), "=$D$0+5"));
    x0.copyRect(CPos("G11"), CPos("F10"), 1, 4);
    assert (valueMatch(x0.getValue(CPos("F10")), CValue(15.0)));
    assert (valueMatch(x0.getValue(CPos("F11")), CValue(15.0)));
    assert (valueMatch(x0.getValue(CPos("F12")), CValue(15.0)));
    assert (valueMatch(x0.getValue(CPos("F13")), CValue(15.0)));
    assert (valueMatch(x0.getValue(CPos("F14")), CValue()));
    assert (valueMatch(x0.getValue(CPos("G10")), CValue()));
    assert (valueMatch(x0.getValue(CPos("G11")), CValue(75.0)));
    assert (valueMatch(x0.getValue(CPos("G12")), CValue(25.0)));
    assert (valueMatch(x0.getValue(CPos("G13")), CValue(65.0)));
    assert (valueMatch(x0.getValue(CPos("G14")), CValue(15.0)));
    x0.copyRect(CPos("G11"), CPos("F10"), 2, 4);
    assert (valueMatch(x0.getValue(CPos("F10")), CValue(15.0)));
    assert (valueMatch(x0.getValue(CPos("F11")), CValue(15.0)));
    assert (valueMatch(x0.getValue(CPos("F12")), CValue(15.0)));
    assert (valueMatch(x0.getValue(CPos("F13")), CValue(15.0)));
    assert (valueMatch(x0.getValue(CPos("F14")), CValue()));
    assert (valueMatch(x0.getValue(CPos("G10")), CValue()));
    assert (valueMatch(x0.getValue(CPos("G11")), CValue(75.0)));
    assert (valueMatch(x0.getValue(CPos("G12")), CValue(25.0)));
    assert (valueMatch(x0.getValue(CPos("G13")), CValue(65.0)));
    assert (valueMatch(x0.getValue(CPos("G14")), CValue(15.0)));
    assert (valueMatch(x0.getValue(CPos("H10")), CValue()));
    assert (valueMatch(x0.getValue(CPos("H11")), CValue()));
    assert (valueMatch(x0.getValue(CPos("H12")), CValue()));
    assert (valueMatch(x0.getValue(CPos("H13")), CValue(35.0)));
    assert (valueMatch(x0.getValue(CPos("H14")), CValue()));
    assert (x0.setCell(CPos("F0"), "-27"));
    assert (valueMatch(x0.getValue(CPos("H14")), CValue(-22.0)));
    x0.copyRect(CPos("H12"), CPos("H13"), 1, 2);
    assert (valueMatch(x0.getValue(CPos("H12")), CValue(25.0)));
    assert (valueMatch(x0.getValue(CPos("H13")), CValue(-22.0)));
    assert (valueMatch(x0.getValue(CPos("H14")), CValue(-22.0)));
    std::cout << "TESTS SUCCESSFUL" << std::endl;
    return EXIT_SUCCESS;
}
