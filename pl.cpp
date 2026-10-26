#include <iostream>
#include <fstream>
#include <string>
#include <set>
#include <map>
#include <vector>
#include <stack>
#include <cctype>

// Перечисление для типов токенов
enum TokenType
{
    KEYWORD,
    IDENTIFIER,
    INTEGER_LITERAL,
    REAL_LITERAL,
    STRING_LITERAL,
    OPERATOR,
    SEPARATOR,
    END_OF_FILE
};

// Структура токена
struct Token
{
    TokenType type;
    std::string value;
    int line;
    int column;
};

// Типы данных
enum DataType
{
    TYPE_INT,
    TYPE_REAL,
    TYPE_STRING,
    TYPE_NONE // Для выражений без типа (например, метки)
};

// Преобразование типа в строку для сообщений об ошибках
std::string data_type_to_string(DataType type)
{
    switch (type)
    {
        case TYPE_INT: return "int";
        case TYPE_REAL: return "real";
        case TYPE_STRING: return "string";
        case TYPE_NONE: return "none";
        default: return "unknown";
    }
}

// Элемент ПОЛИЗ
struct PolizItem
{
    enum ItemType { OPERAND, OPERATOR, LABEL, JUMP };
    ItemType type;
    std::string value; // Имя переменной, оператор, метка или адрес
    DataType dataType; // Тип данных для операнда

    PolizItem(ItemType t, const std::string& v, DataType dt = TYPE_NONE)
        : type(t), value(v), dataType(dt) {}
};

// Класс для управления ПОЛИЗ
class Poliz
{
public:
    void add(const PolizItem& item)
    {
        items.push_back(item);
    }

    void addLabel(const std::string& label)
    {
        add(PolizItem(PolizItem::LABEL, label, TYPE_NONE));
    }

    void addJump(const std::string& label, bool conditional = false)
    {
        add(PolizItem(PolizItem::JUMP, conditional ? "JZ " + label : "JMP " + label, TYPE_NONE));
    }

    void addOperator(const std::string& op)
    {
        add(PolizItem(PolizItem::OPERATOR, op, TYPE_NONE));
    }

    void addOperand(const std::string& value, DataType type)
    {
        add(PolizItem(PolizItem::OPERAND, value, type));
    }

    std::string generateLabel()
    {
        return "L" + std::to_string(labelCounter++);
    }

    void print() const
    {
        for (const auto& item : items)
        {
            switch (item.type)
            {
                case PolizItem::LABEL:
                    std::cout << item.value << ":" << std::endl;
                    break;
                case PolizItem::JUMP:
                case PolizItem::OPERATOR:
                    std::cout << item.value << std::endl;
                    break;
                case PolizItem::OPERAND:
                    std::cout << item.value << " (" << data_type_to_string(item.dataType) << ")" << std::endl;
                    break;
            }
        }
    }

private:
    std::vector<PolizItem> items;
    int labelCounter = 0;
};

// Таблица символов для хранения переменных и меток
class SymbolTable
{
public:
    void addVariable(const std::string& name, DataType type, int line, int column)
    {
        if (variables.find(name) != variables.end() || labels.find(name) != labels.end())
        {
            throw std::runtime_error("Семантическая ошибка: имя '" + name +
                                     "' уже объявлено в строке " + std::to_string(line) +
                                     ", столбце " + std::to_string(column));
        }
        variables[name] = type;
    }

    void addLabel(const std::string& name, int line, int column)
    {
        if (labels.find(name) != labels.end() || variables.find(name) != variables.end())
        {
            throw std::runtime_error("Семантическая ошибка: метка '" + name +
                                     "' уже объявлена в строке " + std::to_string(line) +
                                     ", столбце " + std::to_string(column));
        }
        labels[name] = line;
    }

    bool isVariableDeclared(const std::string& name) const
    {
        return variables.find(name) != variables.end();
    }

    bool isLabelDeclared(const std::string& name) const
    {
        return labels.find(name) != labels.end();
    }

    DataType getVariableType(const std::string& name) const
    {
        auto it = variables.find(name);
        if (it == variables.end())
            throw std::runtime_error("Семантическая ошибка: переменная '" + name + "' не объявлена");
        return it->second;
    }

private:
    std::map<std::string, DataType> variables; // имя переменной -> тип
    std::map<std::string, int> labels;         // имя метки -> строка
};

// Класс лексера
class Lexer
{
public:
    Lexer(const std::string& filename);
    Token get_next_token();
    void next();
    char peek();
    void skip_whitespace();
    void skip_comment();
    Token read_identifier_or_keyword();
    Token read_number();
    Token read_string();
    Token read_operator();
    Token read_separator();
private:
    std::ifstream file;
    char current_char;
    int line;
    int column;
};

Lexer::Lexer(const std::string& filename)
    : file(filename), current_char('\0'), line(1), column(0)
{
    if (!file.is_open())
    {
        throw std::runtime_error("Не удалось открыть файл: " + filename);
    }
    file >> std::noskipws;
    next();
}

void Lexer::next()
{
    if (file.get(current_char))
    {
        if (current_char == '\n')
        {
            line++;
            column = 0;
        }
        else
        {
            column++;
        }
    }
    else
    {
        current_char = EOF;
    }
}

char Lexer::peek()
{
    char next_char = file.peek();
    return (next_char == EOF) ? EOF : next_char;
}

void Lexer::skip_whitespace()
{
    while (std::isspace(current_char) && current_char != EOF)
    {
        next();
    }
}

void Lexer::skip_comment()
{
    int start_line = line, start_column = column;
    if (current_char == '/' && peek() == '/')
    {
        while (current_char != EOF && current_char != '\n')
        {
            next();
        }
        if (current_char == '\n')
        {
            next();
        }
        return;
    }
    else if (current_char == '/' && peek() == '*')
    {
        next();
        next();
        while (current_char != EOF)
        {
            if (current_char == '*' && peek() == '/')
            {
                next();
                next();
                return;
            }
            next();
        }
        throw std::runtime_error("Незакрытый комментарий на строке " + std::to_string(start_line) +
                                 ", столбце " + std::to_string(start_column));
    }
}

Token Lexer::read_identifier_or_keyword()
{
    std::string value;
    int start_line = line, start_column = column;
    while (std::isalnum(current_char) || current_char == '_')
    {
        value += current_char;
        next();
    }
    static const std::set<std::string> keywords =
    {
        "program", "int", "string", "if", "else", "while", "read", "write", "break", "real", "for", "goto"
    };
    if (keywords.count(value) > 0)
    {
        return {KEYWORD, value, start_line, start_column};
    }
    return {IDENTIFIER, value, start_line, start_column};
}

Token Lexer::read_number()
{
    std::string value;
    int start_line = line, start_column = column;
    bool is_real = false;

    if (current_char == '+' || current_char == '-')
    {
        value += current_char;
        next();
    }

    while (std::isdigit(current_char))
    {
        value += current_char;
        next();
    }

    if (current_char == '.')
    {
        is_real = true;
        value += current_char;
        next();
        while (std::isdigit(current_char))
        {
            value += current_char;
            next();
        }
    }

    if (current_char == 'e' || current_char == 'E')
    {
        is_real = true;
        value += current_char;
        next();
        if (current_char == '+' || current_char == '-')
        {
            value += current_char;
            next();
        }
        while (std::isdigit(current_char))
        {
            value += current_char;
            next();
        }
    }

    TokenType type = is_real ? REAL_LITERAL : INTEGER_LITERAL;
    return {type, value, start_line, start_column};
}

Token Lexer::read_string()
{
    std::string value;
    int start_line = line, start_column = column;
    next();
    while (current_char != '"' && current_char != EOF)
    {
        value += current_char;
        next();
    }
    if (current_char == '"')
    {
        next();
        return {STRING_LITERAL, value, start_line, start_column};
    }
    throw std::runtime_error("Незакрытая строковая константа на строке " + std::to_string(start_line) +
                             ", столбце " + std::to_string(start_column));
}

Token Lexer::read_operator()
{
    std::string value;
    int start_line = line, start_column = column;
    value += current_char;
    next();

    if ((value == "+" && current_char == '+') ||
        (value == "-" && current_char == '-') ||
        (value == "&" && current_char == '&') ||
        (value == "|" && current_char == '|') ||
        (value == "<" && current_char == '=') ||
        (value == ">" && current_char == '=') ||
        (value == "=" && current_char == '=') ||
        (value == "!" && current_char == '='))
    {
        value += current_char;
        next();
    }

    return {OPERATOR, value, start_line, start_column};
}

Token Lexer::read_separator()
{
    std::string value(1, current_char);
    int start_line = line, start_column = column;
    next();
    return {SEPARATOR, value, start_line, start_column};
}

bool is_operator(char c)
{
    static const std::string operators = "+-*/%<>=!&|";
    return operators.find(c) != std::string::npos;
}

bool is_separator(char c)
{
    static const std::string separators = "(){}[],;.:";
    return separators.find(c) != std::string::npos;
}

Token Lexer::get_next_token()
{
    while (current_char != EOF)
    {
        if (std::isspace(current_char))
        {
            skip_whitespace();
            continue;
        }
        if (current_char == '/' && (peek() == '*' || peek() == '/'))
        {
            skip_comment();
            continue;
        }
        if (std::isalpha(current_char) || current_char == '_')
        {
            return read_identifier_or_keyword();
        }
        if (is_operator(current_char))
        {
            return read_operator();
        }
        if (std::isdigit(current_char) || current_char == '+' || current_char == '-' || current_char == '.')
        {
            return read_number();
        }
        if (current_char == '"')
        {
            return read_string();
        }
        if (is_separator(current_char))
        {
            return read_separator();
        }
        throw std::runtime_error("Неожиданный символ '" + std::string(1, current_char) +
                                 "' на строке " + std::to_string(line) + ", столбце " + std::to_string(column));
    }
    return {END_OF_FILE, "", line, column};
}

std::string token_type_to_string(TokenType type)
{
    switch (type)
    {
        case KEYWORD: return "KEYWORD";
        case IDENTIFIER: return "IDENTIFIER";
        case INTEGER_LITERAL: return "INTEGER_LITERAL";
        case REAL_LITERAL: return "REAL_LITERAL";
        case STRING_LITERAL: return "STRING_LITERAL";
        case OPERATOR: return "OPERATOR";
        case SEPARATOR: return "SEPARATOR";
        case END_OF_FILE: return "END_OF_FILE";
        default: return "UNKNOWN";
    }
}

// Класс парсера
class Parser
{
public:
    Parser(Lexer& lexer);
    void parseProgram();
    void printPoliz() const { poliz.print(); }
private:
    Lexer& lexer;
    Token currentToken;
    SymbolTable symbolTable;
    Poliz poliz;
    int loopDepth; // Для проверки break
    std::stack<std::string> breakLabels; // Метки для выхода из циклов

    void eat(TokenType type, const std::string& value = "");
    void parseDeclarations();
    void parseDeclaration();
    void parseVariableList(const std::string& type);
    void parseVariable(const std::string& type);
    void parseStatements();
    void parseStatement();
    void parseIfStatement();
    void parseWhileStatement();
    void parseForStatement();
    void parseGotoStatement();
    void parseIOStatement();
    void parseCompoundStatement();
    DataType parseLogicalExpression();
    DataType parseExpression();
    DataType parseTerm();
    int getOperatorPrecedence(const std::string& op);
    DataType checkBinaryOperation(const std::string& op, DataType left, DataType right, int line);
};

Parser::Parser(Lexer& lexer)
    : lexer(lexer), currentToken(lexer.get_next_token()), loopDepth(0) {}

void Parser::eat(TokenType type, const std::string& value)
{
    if (currentToken.type == type && (value.empty() || currentToken.value == value))
    {
        currentToken = lexer.get_next_token();
    }
    else
    {
        std::string expected = value.empty() ? token_type_to_string(type) : "'" + value + "'";
        throw std::runtime_error("Синтаксическая ошибка: ожидалось " + expected +
                                 ", но получено " + token_type_to_string(currentToken.type) +
                                 " со значением '" + currentToken.value + "'" +
                                 " на строке " + std::to_string(currentToken.line) +
                                 ", столбце " + std::to_string(currentToken.column));
    }
}

void Parser::parseProgram()
{
    eat(KEYWORD, "program");
    parseDeclarations();
    parseStatements();
    eat(END_OF_FILE);
}

void Parser::parseDeclarations()
{
    while (currentToken.type == KEYWORD &&
           (currentToken.value == "int" || currentToken.value == "string" || currentToken.value == "real"))
    {
        parseDeclaration();
    }
}

void Parser::parseDeclaration()
{
    std::string typeStr = currentToken.value;
    DataType type = (typeStr == "int") ? TYPE_INT : (typeStr == "real") ? TYPE_REAL : TYPE_STRING;
    eat(KEYWORD);
    parseVariableList(typeStr);
    eat(SEPARATOR, ";");
}

void Parser::parseVariableList(const std::string& typeStr)
{
    parseVariable(typeStr);
    while (currentToken.type == SEPARATOR && currentToken.value == ",")
    {
        eat(SEPARATOR, ",");
        parseVariable(typeStr);
    }
}

void Parser::parseVariable(const std::string& typeStr)
{
    Token ident = currentToken;
    DataType type = (typeStr == "int") ? TYPE_INT : (typeStr == "real") ? TYPE_REAL : TYPE_STRING;
    eat(IDENTIFIER);
    symbolTable.addVariable(ident.value, type, ident.line, ident.column);
    if (currentToken.type == OPERATOR && currentToken.value == "=")
    {
        poliz.addOperand(ident.value, type);
        eat(OPERATOR, "=");
        DataType exprType = parseExpression();
        if (type != exprType)
        {
            throw std::runtime_error("Семантическая ошибка: несовместимый тип при присваивании переменной '" +
                                     ident.value + "' (ожидался " + data_type_to_string(type) +
                                     ", получен " + data_type_to_string(exprType) +
                                     ") на строке " + std::to_string(ident.line));
        }
        poliz.addOperator("=");
    }
}

void Parser::parseStatements()
{
    while (currentToken.type != END_OF_FILE)
    {
        parseStatement();
    }
}

void Parser::parseStatement()
{
    if (currentToken.type == KEYWORD)
    {
        if (currentToken.value == "if")
        {
            parseIfStatement();
        }
        else if (currentToken.value == "while")
        {
            parseWhileStatement();
        }
        else if (currentToken.value == "for")
        {
            parseForStatement();
        }
        else if (currentToken.value == "goto")
        {
            parseGotoStatement();
        }
        else if (currentToken.value == "read" || currentToken.value == "write")
        {
            parseIOStatement();
        }
        else if (currentToken.value == "break")
        {
            if (loopDepth == 0)
            {
                throw std::runtime_error("Семантическая ошибка: оператор 'break' вне цикла на строке " +
                                         std::to_string(currentToken.line));
            }
            eat(KEYWORD, "break");
            poliz.addJump(breakLabels.top());
            eat(SEPARATOR, ";");
        }
        else
        {
            throw std::runtime_error("Синтаксическая ошибка: неожиданное ключевое слово '" + currentToken.value +
                                     "' на строке " + std::to_string(currentToken.line));
        }
    }
    else if (currentToken.type == SEPARATOR && currentToken.value == "{")
    {
        parseCompoundStatement();
    }
    else if (currentToken.type == IDENTIFIER)
    {
        Token ident = currentToken;
        eat(IDENTIFIER);
        if (currentToken.type == SEPARATOR && currentToken.value == ":")
        {
            eat(SEPARATOR, ":");
            symbolTable.addLabel(ident.value, ident.line, ident.column);
            poliz.addLabel(ident.value);
        }
        else if (currentToken.type == OPERATOR && currentToken.value == "=")
        {
            if (!symbolTable.isVariableDeclared(ident.value))
            {
                throw std::runtime_error("Семантическая ошибка: переменная '" + ident.value +
                                         "' не объявлена на строке " + std::to_string(ident.line));
            }
            DataType varType = symbolTable.getVariableType(ident.value);
            poliz.addOperand(ident.value, varType);
            eat(OPERATOR, "=");
            DataType exprType = parseExpression();
            if (varType != exprType)
            {
                throw std::runtime_error("Семантическая ошибка: несовместимый тип при присваивании переменной '" +
                                         ident.value + "' (ожидался " + data_type_to_string(varType) +
                                         ", получен " + data_type_to_string(exprType) +
                                         ") на строке " + std::to_string(ident.line));
            }
            poliz.addOperator("=");
            eat(SEPARATOR, ";");
        }
        else
        {
            throw std::runtime_error("Синтаксическая ошибка: ожидался '=' или ':' после идентификатора на строке " +
                                     std::to_string(currentToken.line));
        }
    }
    else
    {
        throw std::runtime_error("Синтаксическая ошибка: неожиданный токен " + token_type_to_string(currentToken.type) +
                                 " на строке " + std::to_string(currentToken.line));
    }
}

void Parser::parseIfStatement()
{
    eat(KEYWORD, "if");
    eat(SEPARATOR, "(");
    DataType condType = parseLogicalExpression();
    if (condType != TYPE_INT)
    {
        throw std::runtime_error("Семантическая ошибка: условие в 'if' должно быть типа int на строке " +
                                 std::to_string(currentToken.line));
    }
    std::string endLabel = poliz.generateLabel();
    poliz.addJump(endLabel, true); // JZ endLabel
    eat(SEPARATOR, ")");
    parseStatement();
    if (currentToken.type == KEYWORD && currentToken.value == "else")
    {
        std::string elseLabel = poliz.generateLabel();
        poliz.addJump(elseLabel); // JMP elseLabel
        poliz.addLabel(endLabel);
        eat(KEYWORD, "else");
        parseStatement();
        poliz.addLabel(elseLabel);
    }
    else
    {
        poliz.addLabel(endLabel);
    }
}

void Parser::parseWhileStatement()
{
    std::string startLabel = poliz.generateLabel();
    std::string endLabel = poliz.generateLabel();
    breakLabels.push(endLabel);
    loopDepth++;
    poliz.addLabel(startLabel);
    eat(KEYWORD, "while");
    eat(SEPARATOR, "(");
    DataType condType = parseLogicalExpression();
    if (condType != TYPE_INT)
    {
        throw std::runtime_error("Семантическая ошибка: условие в 'while' должно быть типа int на строке " +
                                 std::to_string(currentToken.line));
    }
    poliz.addJump(endLabel, true); // JZ endLabel
    eat(SEPARATOR, ")");
    parseStatement();
    poliz.addJump(startLabel); // JMP startLabel
    poliz.addLabel(endLabel);
    loopDepth--;
    breakLabels.pop();
}

void Parser::parseForStatement()
{
    std::string startLabel = poliz.generateLabel();
    std::string bodyLabel = poliz.generateLabel();
    std::string endLabel = poliz.generateLabel();
    breakLabels.push(endLabel);
    loopDepth++;
    eat(KEYWORD, "for");
    eat(SEPARATOR, "(");
    if (currentToken.type == KEYWORD &&
        (currentToken.value == "int" || currentToken.value == "real" || currentToken.value == "string"))
    {
        parseDeclaration();
    }
    else if (currentToken.type == IDENTIFIER)
    {
        Token ident = currentToken;
        eat(IDENTIFIER);
        if (!symbolTable.isVariableDeclared(ident.value))
        {
            throw std::runtime_error("Семантическая ошибка: переменная '" + ident.value +
                                     "' не объявлена на строке " + std::to_string(ident.line));
        }
        DataType varType = symbolTable.getVariableType(ident.value);
        poliz.addOperand(ident.value, varType);
        eat(OPERATOR, "=");
        DataType exprType = parseExpression();
        if (varType != exprType)
        {
            throw std::runtime_error("Семантическая ошибка: несовместимый тип при присваивании переменной '" +
                                     ident.value + "' в цикле for (ожидался " + data_type_to_string(varType) +
                                     ", получен " + data_type_to_string(exprType) +
                                     ") на строке " + std::to_string(ident.line));
        }
        poliz.addOperator("=");
        eat(SEPARATOR, ";");
    }
    else
    {
        eat(SEPARATOR, ";");
    }
    poliz.addLabel(startLabel);
    DataType condType = parseLogicalExpression();
    if (condType != TYPE_INT)
    {
        throw std::runtime_error("Семантическая ошибка: условие в 'for' должно быть типа int на строке " +
                                 std::to_string(currentToken.line));
    }
    poliz.addJump(endLabel, true); // JZ endLabel
    poliz.addJump(bodyLabel); // JMP bodyLabel
    eat(SEPARATOR, ";");
    if (currentToken.type == IDENTIFIER)
    {
        Token ident = currentToken;
        eat(IDENTIFIER);
        if (!symbolTable.isVariableDeclared(ident.value))
        {
            throw std::runtime_error("Семантическая ошибка: переменная '" + ident.value +
                                     "' не объявлена на строке " + std::to_string(ident.line));
        }
        DataType varType = symbolTable.getVariableType(ident.value);
        poliz.addOperand(ident.value, varType);
        eat(OPERATOR, "=");
        DataType exprType = parseExpression();
        if (varType != exprType)
        {
            throw std::runtime_error("Семантическая ошибка: несовместимый тип при присваивании переменной '" +
                                     ident.value + "' в цикле for (ожидался " + data_type_to_string(varType) +
                                     ", получен " + data_type_to_string(exprType) +
                                     ") на строке " + std::to_string(ident.line));
        }
        poliz.addOperator("=");
    }
    eat(SEPARATOR, ")");
    poliz.addJump(startLabel); // JMP startLabel
    poliz.addLabel(bodyLabel);
    parseStatement();
    poliz.addJump(startLabel); // JMP startLabel
    poliz.addLabel(endLabel);
    loopDepth--;
    breakLabels.pop();
}

void Parser::parseGotoStatement()
{
    eat(KEYWORD, "goto");
    Token label = currentToken;
    eat(IDENTIFIER);
    if (!symbolTable.isLabelDeclared(label.value))
    {
        throw std::runtime_error("Семантическая ошибка: метка '" + label.value +
                                 "' не объявлена на строке " + std::to_string(label.line));
    }
    poliz.addJump(label.value);
    eat(SEPARATOR, ";");
}

void Parser::parseIOStatement()
{
    if (currentToken.value == "read")
    {
        eat(KEYWORD, "read");
        eat(SEPARATOR, "(");
        Token ident = currentToken;
        eat(IDENTIFIER);
        if (!symbolTable.isVariableDeclared(ident.value))
        {
            throw std::runtime_error("Семантическая ошибка: переменная '" + ident.value +
                                     "' не объявлена на строке " + std::to_string(ident.line));
        }
        DataType varType = symbolTable.getVariableType(ident.value);
        poliz.addOperand(ident.value, varType);
        poliz.addOperator("READ");
        eat(SEPARATOR, ")");
        eat(SEPARATOR, ";");
    }
    else if (currentToken.value == "write")
    {
        eat(KEYWORD, "write");
        eat(SEPARATOR, "(");
        DataType exprType = parseExpression();
        poliz.addOperator("WRITE");
        eat(SEPARATOR, ")");
        eat(SEPARATOR, ";");
    }
}

void Parser::parseCompoundStatement()
{
    eat(SEPARATOR, "{");
    while (currentToken.type != SEPARATOR || currentToken.value != "}")
    {
        parseStatement();
    }
    eat(SEPARATOR, "}");
}

int Parser::getOperatorPrecedence(const std::string& op)
{
    if (op == "*" || op == "/") return 3;
    if (op == "+" || op == "-") return 2;
    if (op == "<" || op == ">" || op == "<=" || op == ">=" || op == "==" || op == "!=") return 1;
    if (op == "&&" || op == "||") return 0;
    return -1;
}

DataType Parser::checkBinaryOperation(const std::string& op, DataType left, DataType right, int line)
{
    if (op == "+" || op == "-" || op == "*" || op == "/")
    {
        if ((left == TYPE_INT || left == TYPE_REAL) && (right == TYPE_INT || right == TYPE_REAL))
        {
            return (left == TYPE_REAL || right == TYPE_REAL) ? TYPE_REAL : TYPE_INT;
        }
        throw std::runtime_error("Семантическая ошибка: оператор '" + op +
                                 "' требует типов int или real, получены " +
                                 data_type_to_string(left) + " и " + data_type_to_string(right) +
                                 " на строке " + std::to_string(line));
    }
    if (op == "<" || op == ">" || op == "<=" || op == ">=" || op == "==" || op == "!=")
    {
        if (left == right)
        {
            return TYPE_INT; // Сравнения возвращают булев тип (int)
        }
        throw std::runtime_error("Семантическая ошибка: оператор '" + op +
                                 "' требует одинаковых типов, получены " +
                                 data_type_to_string(left) + " и " + data_type_to_string(right) +
                                 " на строке " + std::to_string(line));
    }
    if (op == "&&" || op == "||")
    {
        if (left == TYPE_INT && right == TYPE_INT)
        {
            return TYPE_INT;
        }
        throw std::runtime_error("Семантическая ошибка: оператор '" + op +
                                 "' требует типа int, получены " +
                                 data_type_to_string(left) + " и " + data_type_to_string(right) +
                                 " на строке " + std::to_string(line));
    }
    throw std::runtime_error("Семантическая ошибка: неизвестный оператор '" + op +
                             "' на строке " + std::to_string(line));
}

DataType Parser::parseLogicalExpression()
{
    DataType leftType = parseExpression();
    std::stack<std::string> operators;
    std::stack<DataType> types;

    while (currentToken.type == OPERATOR &&
           (currentToken.value == "&&" || currentToken.value == "||"))
    {
        std::string op = currentToken.value;
        eat(OPERATOR);
        DataType rightType = parseExpression();
        leftType = checkBinaryOperation(op, leftType, rightType, currentToken.line);
        operators.push(op);
        types.push(leftType);
    }

    while (!operators.empty())
    {
        poliz.addOperator(operators.top());
        operators.pop();
        types.pop();
    }

    return leftType;
}

DataType Parser::parseExpression()
{
    std::stack<std::string> operatorStack;
    std::stack<DataType> typeStack;
    DataType leftType = parseTerm();

    while (currentToken.type == OPERATOR &&
           (currentToken.value == "+" || currentToken.value == "-" ||
            currentToken.value == "<" || currentToken.value == ">" ||
            currentToken.value == "<=" || currentToken.value == ">=" ||
            currentToken.value == "==" || currentToken.value == "!="))
    {
        std::string op = currentToken.value;
        int prec = getOperatorPrecedence(op);
        while (!operatorStack.empty() && getOperatorPrecedence(operatorStack.top()) >= prec)
        {
            std::string topOp = operatorStack.top();
            operatorStack.pop();
            poliz.addOperator(topOp);
            typeStack.pop();
        }
        operatorStack.push(op);
        eat(OPERATOR);
        DataType rightType = parseTerm();
        leftType = checkBinaryOperation(op, leftType, rightType, currentToken.line);
        typeStack.push(leftType);
    }

    while (!operatorStack.empty())
    {
        poliz.addOperator(operatorStack.top());
        operatorStack.pop();
        typeStack.pop();
    }

    return leftType;
}

DataType Parser::parseTerm()
{
    DataType type;
    if (currentToken.type == IDENTIFIER)
    {
        Token ident = currentToken;
        eat(IDENTIFIER);
        if (!symbolTable.isVariableDeclared(ident.value))
        {
            throw std::runtime_error("Семантическая ошибка: переменная '" + ident.value +
                                     "' не объявлена на строке " + std::to_string(ident.line));
        }
        type = symbolTable.getVariableType(ident.value);
        poliz.addOperand(ident.value, type);
    }
    else if (currentToken.type == INTEGER_LITERAL)
    {
        poliz.addOperand(currentToken.value, TYPE_INT);
        type = TYPE_INT;
        eat(INTEGER_LITERAL);
    }
    else if (currentToken.type == REAL_LITERAL)
    {
        poliz.addOperand(currentToken.value, TYPE_REAL);
        type = TYPE_REAL;
        eat(REAL_LITERAL);
    }
    else if (currentToken.type == STRING_LITERAL)
    {
        poliz.addOperand(currentToken.value, TYPE_STRING);
        type = TYPE_STRING;
        eat(STRING_LITERAL);
    }
    else if (currentToken.type == SEPARATOR && currentToken.value == "(")
    {
        eat(SEPARATOR, "(");
        type = parseLogicalExpression();
        eat(SEPARATOR, ")");
    }
    else
    {
        throw std::runtime_error("Синтаксическая ошибка: ожидалось выражение на строке " +
                                 std::to_string(currentToken.line));
    }

    while (currentToken.type == OPERATOR &&
           (currentToken.value == "*" || currentToken.value == "/"))
    {
        std::string op = currentToken.value;
        eat(OPERATOR);
        poliz.addOperator(op);
        DataType rightType;
        if (currentToken.type == IDENTIFIER)
        {
            Token ident = currentToken;
            eat(IDENTIFIER);
            if (!symbolTable.isVariableDeclared(ident.value))
            {
                throw std::runtime_error("Семантическая ошибка: переменная '" + ident.value +
                                         "' не объявлена на строке " + std::to_string(ident.line));
            }
            rightType = symbolTable.getVariableType(ident.value);
            poliz.addOperand(ident.value, rightType);
        }
        else if (currentToken.type == INTEGER_LITERAL)
        {
            poliz.addOperand(currentToken.value, TYPE_INT);
            rightType = TYPE_INT;
            eat(INTEGER_LITERAL);
        }
        else if (currentToken.type == REAL_LITERAL)
        {
            poliz.addOperand(currentToken.value, TYPE_REAL);
            rightType = TYPE_REAL;
            eat(REAL_LITERAL);
        }
        else if (currentToken.type == SEPARATOR && currentToken.value == "(")
        {
            eat(SEPARATOR, "(");
            rightType = parseLogicalExpression();
            eat(SEPARATOR, ")");
        }
        else
        {
            throw std::runtime_error("Синтаксическая ошибка: ожидался терм после оператора на строке " +
                                     std::to_string(currentToken.line));
        }
        type = checkBinaryOperation(op, type, rightType, currentToken.line);
    }

    return type;
}

int main()
{
    try
    {
        Lexer lexer("input.txt");
        Parser parser(lexer);
        parser.parseProgram();
        std::cout << "Синтаксический анализ успешно завершён" << std::endl;
        std::cout << "ПОЛИЗ:" << std::endl;
        parser.printPoliz();
    }
    catch (const std::exception& e)
    {
        std::cerr << "Ошибка: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}