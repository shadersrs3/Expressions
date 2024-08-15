#include <cstdio>
#include <cctype>
#include <string>

enum TokenType {
    TOKEN_TYPE_NULL,
    TOKEN_TYPE_INTEGER,
    TOKEN_TYPE_ADD,
    TOKEN_TYPE_MINUS,
    TOKEN_TYPE_MUL,
    TOKEN_TYPE_LPAREN,
    TOKEN_TYPE_RPAREN,
};

struct Token {
    std::string name;
    TokenType type;
};

typedef int Character;

static bool beginsWithName(Character c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static bool beginsWithDigit(Character c) {
    return c >= '0' && c <= '9';
}

static bool isIdentifier(Character c) {
    return beginsWithName(c) || beginsWithDigit(c);
}

struct Scanner {
private:
    std::string buffer;
    size_t currentCharacterIndex;
    size_t nextCharacterIndex;
public:
    Character getCharacter();
    void setBuffer(const std::string& buf);
    void incrementPosition(int amount = 1);
    Token peekToken();
    void nextToken();
};

Character Scanner::getCharacter() {
    if (currentCharacterIndex >= buffer.length())
        return 0;
    return buffer[currentCharacterIndex];
}

void Scanner::incrementPosition(int amount) {
    currentCharacterIndex += amount;
}

void Scanner::setBuffer(const std::string& buf) {
    currentCharacterIndex = 0;
    buffer = buf;
}

Token Scanner::peekToken() {
    Token t;
    Character c;

    size_t previousCharacterIndex = currentCharacterIndex;
    c = getCharacter();
    if (c == 0) {
        t.type = TOKEN_TYPE_NULL;
        return t;
    }

    if (!std::isprint(c)) {
        printf("Bad lexical analysis stream (no such printable character)\n");
        t.type = TOKEN_TYPE_NULL;
        return t;
    }

    while (std::isspace(c)) {
        incrementPosition();
        c = getCharacter();
    }

    if (beginsWithDigit(c)) {
        t.type = TOKEN_TYPE_INTEGER;
        while (beginsWithDigit(c)) {
            t.name += c;
            incrementPosition();
            c = getCharacter();
        }

        if (isIdentifier(c) || c == '.') {
            printf("skipping trailing characters for integer\n");
            while (isIdentifier(c) || c == '.') {
                incrementPosition();
                c = getCharacter();
            }
        }
    } else {
        t.name = c;
        switch (c) {
        case '+':
            t.type = TOKEN_TYPE_ADD;
            incrementPosition();
            break;
        case '-':
            t.type = TOKEN_TYPE_MINUS;
            incrementPosition();
            break;
        case '*':
            t.type = TOKEN_TYPE_MUL;
            incrementPosition();
            break;
        case '(':
            t.type = TOKEN_TYPE_LPAREN;
            incrementPosition();
            break;
        case ')':
            t.type = TOKEN_TYPE_RPAREN;
            incrementPosition();
            break;
        default:
            printf("Unexpected lexical analysis character %c\n", c);
            t.type = TOKEN_TYPE_NULL;
        }
    }

    nextCharacterIndex = currentCharacterIndex;
    currentCharacterIndex = previousCharacterIndex;
    return t;
}

void Scanner::nextToken() {
    currentCharacterIndex = nextCharacterIndex;
}

enum TreeType {
    TREE_TYPE_NONE,
    TREE_TYPE_LITERAL,
    TREE_TYPE_UNARY_EXPRESSION,
    TREE_TYPE_BINARY_EXPRESSION,
};

struct Tree {
    TreeType treeType;
};

struct LiteralTree : Tree {
    Token token;
};

struct UnaryExpressionTree : Tree {
    TokenType operatorType;
    Tree *child;
};

struct BinaryExpressionTree : Tree {
    int operatorType;
    Tree *left;
    Tree *right;
};

LiteralTree *createLiteralTree(const Token& token) {
    LiteralTree *tree = new LiteralTree;
    tree->treeType = TREE_TYPE_LITERAL;
    tree->token = token;
    return tree;
}

UnaryExpressionTree *createUnaryExpressionTree(TokenType operatorType, Tree *child) {
    UnaryExpressionTree *expr = new UnaryExpressionTree;
    expr->treeType = TREE_TYPE_UNARY_EXPRESSION;
    expr->operatorType = operatorType;
    expr->child = child;
    return expr;
}

BinaryExpressionTree *createBinaryExpressionTree(int operatorType, Tree *left, Tree *right) {
    BinaryExpressionTree *expr = new BinaryExpressionTree;
    expr->treeType = TREE_TYPE_BINARY_EXPRESSION;
    expr->operatorType = operatorType;
    expr->left = left;
    expr->right = right;
    return expr;
}

Scanner s;

bool matchToken(const Token& t, TokenType type) {
    return t.type == type;
}

static bool matchTerm(const Token& t) {
    return t.type == TOKEN_TYPE_ADD || t.type == TOKEN_TYPE_MINUS;
}

static bool matchFactor(const Token& t) {
    return t.type == TOKEN_TYPE_MUL;
}

Tree *parseExpression();

void destroyExpressionTreeWithChildren(Tree *expr) {
    if (!expr)
        return;

    switch (expr->treeType) {
    case TREE_TYPE_BINARY_EXPRESSION:
        destroyExpressionTreeWithChildren(static_cast<BinaryExpressionTree *>(expr)->left);
        destroyExpressionTreeWithChildren(static_cast<BinaryExpressionTree *>(expr)->right);
        reinterpret_cast<BinaryExpressionTree *>(expr)->left = nullptr;
        reinterpret_cast<BinaryExpressionTree *>(expr)->right = nullptr;
        delete static_cast<BinaryExpressionTree *>(expr);
        break;
    case TREE_TYPE_LITERAL:
        delete static_cast<LiteralTree *>(expr);
        break;
    case TREE_TYPE_UNARY_EXPRESSION:
        destroyExpressionTreeWithChildren(static_cast<UnaryExpressionTree *>(expr)->child);
        reinterpret_cast<UnaryExpressionTree *>(expr)->child = nullptr;
        delete static_cast<UnaryExpressionTree *>(expr);
        break;
    default:
        printf("What tree is this?\n");
        return;
    }
}

Tree *parsePrimary() {
    Token t = s.peekToken();
    Tree *tree;

    if (matchToken(t, TOKEN_TYPE_INTEGER)) {
        s.nextToken();
        tree = createLiteralTree(t);
        return tree;
    } else if (matchToken(t, TOKEN_TYPE_LPAREN)) {
        s.nextToken();
        tree = parseExpression();
        if (t = s.peekToken(); t.type != TOKEN_TYPE_RPAREN) {
            printf("Expected right parantheses match\n");
            destroyExpressionTreeWithChildren(tree);
            return nullptr;
        }

        s.nextToken();
        return tree;
    }
    printf("Syntax error in %s\n", t.name.c_str());
    return nullptr;
}

Tree *parseMultiplicativeExpression() {
    Tree *a = parsePrimary();
    
    if (auto tok = s.peekToken(); matchFactor(tok)) {
        s.nextToken();
        a = createBinaryExpressionTree(tok.type, a, parsePrimary());
        if (tok = s.peekToken(); matchFactor(tok)) {
            s.nextToken();
            a = createBinaryExpressionTree(tok.type, a, parseMultiplicativeExpression());
        }
    }

    return a;
}

Tree *parseAdditiveExpression() {
    Tree *a = parseMultiplicativeExpression();

    if (auto tok = s.peekToken(); matchTerm(tok)) {
        s.nextToken();
        
        a = createBinaryExpressionTree(tok.type, a, parseMultiplicativeExpression());

        if (tok = s.peekToken(); matchTerm(tok)) {
            s.nextToken();
            a = createBinaryExpressionTree(tok.type, a, parseAdditiveExpression());
        }
    }
    return a;
}

Tree *parseExpression() {
    return parseAdditiveExpression();
}

std::uint64_t evaluateConstantExpressionTree(Tree *expr) {
    std::uint64_t a, b;
    std::uint64_t result = 0;

    if (!expr)
        return 0;

    switch (expr->treeType) {
    case TREE_TYPE_BINARY_EXPRESSION:
        a = evaluateConstantExpressionTree(static_cast<BinaryExpressionTree *>(expr)->left);
        b = evaluateConstantExpressionTree(static_cast<BinaryExpressionTree *>(expr)->right);
        switch (static_cast<BinaryExpressionTree *>(expr)->operatorType) {
        case TOKEN_TYPE_ADD:
            result = a + b;
            break;
        case TOKEN_TYPE_MINUS:
            result = a - b;
            break;
        case TOKEN_TYPE_MUL:
            result = a * b;
            break;
        default:
            ;
        }
        break;
    case TREE_TYPE_LITERAL:
        return std::atoll(static_cast<LiteralTree *>(expr)->token.name.c_str());
    case TREE_TYPE_UNARY_EXPRESSION:
        result = evaluateConstantExpressionTree(static_cast<UnaryExpressionTree *>(expr)->child);
        switch (static_cast<UnaryExpressionTree *>(expr)->operatorType) {
        case TOKEN_TYPE_MINUS:
            result = -result;
            break;
        default:
            ;
        }
        break;
    default:
        printf("What tree is this?\n");
        return 0;
    }
    return result;
}

struct ExpressionEvaluationTester {
    std::string buffer;
    std::uint64_t result;
};

ExpressionEvaluationTester evaluations[] = {
    { "4 + 3 * 8", 4 + 3 * 8 },
    { "(4 + 3) * 8", (4 + 3) * 8 },
    { "(4 + 3 * 8) + 8 * 8 + (4 * 4)", (4 + 3 * 8) + 8 * 8 + (4 * 4) }
};

void testExpressions() {
    for (auto& i : evaluations) {
        s.setBuffer(i.buffer);
        Tree *tree = parseExpression();
        std::uint64_t result = evaluateConstantExpressionTree(tree);
        
        printf("Test %s %s :: (my result: %ld) == (compilers result: %ld)\n", result == i.result ? "passed" : "failed", i.buffer.c_str(), (std::int64_t) result, (std::int64_t) i.result);

        destroyExpressionTreeWithChildren(tree);
    }
}

int main(int argc, char *argv[]) {
    testExpressions();
    return 0;
}