#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <limits.h>

typedef struct {
    int (*func)(int, int);
    int precedence;
    const char *name;
} Operator;

typedef struct TreeNode {
    union {
        struct {
            int value;
        } operand;
        Operator operator;
    } data;
    int type; // 0: 操作数, 1: 操作符
    struct TreeNode *left;
    struct TreeNode *right;
} TreeNode;

// 操作符相关函数
Operator findOperator(const char *opStr);

// 表达式树相关函数
TreeNode* createOperandNode(int value);
TreeNode* createOperatorNode(Operator op);
void freeExpressionTree(TreeNode *node);

// 解析器相关函数
TreeNode* parseOperand(char **input);
Operator parseOperator(char **input);
void skipWhitespace(char **input);
TreeNode* parseFactor(char **input);
TreeNode* parseExpression(char **input, int precedence);
TreeNode* buildExpressionTree(char *input);

// 求值器相关函数
int evaluateExpressionTree(TreeNode *node);


// 计算表达式树的结果
int evaluateExpressionTree(TreeNode *node) {
    if (node == NULL) return 0;
    if (node->type == 0) return node->data.operand.value;

    int leftValue = evaluateExpressionTree(node->left);
    int rightValue = evaluateExpressionTree(node->right);

    // 调用操作符函数并返回结果
    return (*(node->data.operator.func))(leftValue, rightValue);
}
// 操作符函数定义
int add(int a, int b) { return a + b; }
int sub(int a, int b) { return a - b; }
int mul(int a, int b) { return a * b; }
int div_func(int a, int b) {
    if (b != 0) return a / b;
    else {
        printf("除数不能为0\n");
        exit(EXIT_FAILURE);
    }
}

// 预定义的操作符数组
static const Operator operators[] = {
        { add, 1, "add" },
        { sub, 1, "sub" },
        { mul, 2, "mul" },
        { div_func, 2, "div" },
};

static const int numOperators = sizeof(operators) / sizeof(operators[0]);

// 根据操作符字符串查找对应的函数指针和优先级
Operator findOperator(const char *opStr) {
    for (int i = 0; i < numOperators; i++) {
        if (strcmp(opStr, operators[i].name) == 0) {
            return operators[i];
        }
    }
    // 无效操作符
    Operator invalidOp = { NULL, 0, "unknown" };
    printf("无效操作符: %s\n", opStr);
    return invalidOp;
}
// 解析操作数
TreeNode* parseOperand(char **input) {
    int value = 0;
    int sign = 1;

    // 处理负数
    if (**input == '-') {
        sign = -1;
        (*input)++;
    }

    while (**input && isdigit((unsigned char)**input)) {
        int digit = (**input - '0');
        if (value > (INT_MAX - digit) / 10) {
            printf("数值超出范围\n");
            return NULL;
        }
        value = value * 10 + digit;
        (*input)++;
    }

    if (sign == -1) {
        value = -value;
    }

    // 检查下一个字符是否有效
    if (**input && !isspace((unsigned char)**input) && **input != ')' && !isalpha((unsigned char)**input)) {
        printf("无效字符在数字后\n");
        return NULL;
    }

    return createOperandNode(value);
}

Operator parseOperator(char **input) {
    char opBuffer[20] = {0};
    int i = 0;

    // 跳过空白字符
    skipWhitespace(input);

    // 防止括号部分直接输出无效操作符
    if (!isalpha((unsigned char)**input)) {
        if(**input=='('||**input==')') printf("括号操作符\n");
        else printf("无效字符，非操作符: '%c'\n", **input);
        return (Operator){ NULL, 0, "unknown" };
    }

    // 读取操作符字符串
    while (**input && isalpha((unsigned char)**input)) {
        opBuffer[i++] = *(*input)++;
    }

    Operator op = findOperator(opBuffer);
    if (op.precedence == 0) {
        printf("无效操作符: %s\n", opBuffer);
    } else {
        printf("parsed operator: %s\n", opBuffer);
    }
    return op;
}

// 跳过空白字符
void skipWhitespace(char **input) {
    while (**input && isspace((unsigned char)**input)) (*input)++;
}

// 解析因子（数字或括号内的表达式）
TreeNode* parseFactor(char **input) {
    skipWhitespace(input);

    if (**input == '(') {
        printf("检测到左括号\n");
        (*input)++; // 跳过左括号
        TreeNode* node = parseExpression(input, 0); // 递归解析括号内的表达式
        skipWhitespace(input);

        if (**input == ')') {
            printf("检测到右括号\n");
            (*input)++; // 跳过右括号
            return node;

        } else {
            printf("缺少右括号\n");
            freeExpressionTree(node);
            return NULL;
        }
    } else if (isdigit((unsigned char)**input) || **input == '-') {
        return parseOperand(input); // 处理数字和负号
    } else {
        printf("无效内容: '%c'\n", **input);
        return NULL;
    }
}

// 解析表达式（根据优先级）
TreeNode* parseExpression(char **input, int precedence) {
    TreeNode* left = parseFactor(input);
    if (left == NULL) {
        return NULL;
    }

    while (**input) {
        skipWhitespace(input);
        char *savedInput = *input; // 保存当前指针位置以便回溯

        Operator op = parseOperator(input);
        if (op.precedence == 0) {
            *input = savedInput; // 无效操作符，回溯
            break;
        }
        if (op.precedence <= precedence) {
            *input = savedInput; // 操作符优先级不足，回溯
            break;
        }

        TreeNode* right = parseExpression(input, op.precedence);
        if (right == NULL) {
            freeExpressionTree(left);
            return NULL;
        }

        TreeNode* newNode = createOperatorNode(op);
        if (newNode == NULL) {
            freeExpressionTree(left);
            freeExpressionTree(right);
            return NULL;
        }

        newNode->left = left;
        newNode->right = right;
        left = newNode; // 更新左子树
    }

    return left;
}


TreeNode* buildExpressionTree(char *input) {
    char *p = input;
    TreeNode* root = parseExpression(&p, 0);
    if (root == NULL) {
        printf("无法构建表达式树\n");
    }
    return root;
}

// 创建操作数节点
TreeNode* createOperandNode(int value) {
    TreeNode* node = (TreeNode*)malloc(sizeof(TreeNode));
    if (node == NULL) {
        perror("malloc failed");
        return NULL;
    }
    node->data.operand.value = value;
    node->type = 0;
    node->left = NULL;
    node->right = NULL;
    printf("num: %d\n", value);
    return node;
}

// 创建操作符节点
TreeNode* createOperatorNode(Operator op) {
    TreeNode* node = (TreeNode*)malloc(sizeof(TreeNode));
    if (node == NULL) {
        perror("malloc failed");
        return NULL;
    }
    node->data.operator = op;
    node->type = 1;
    node->left = NULL;
    node->right = NULL;
    printf("op: %s\n", op.name);
    return node;
}

// 释放表达式树内存
void freeExpressionTree(TreeNode *node) {
    if (node == NULL) return;
    freeExpressionTree(node->left);
    freeExpressionTree(node->right);
    free(node);
}

int main() {
    while (1) {
        char input[100];
        printf("请输入表达式字符串（如1 add 2）：\n");
        fgets(input, sizeof(input), stdin);
        input[strcspn(input, "\n")] = '\0'; // 去除换行符

        char *inputCopy = strdup(input);
        if (inputCopy == NULL) {
            perror("strdup failed");
            return EXIT_FAILURE;
        }

        TreeNode* root = buildExpressionTree(inputCopy);
        int result = evaluateExpressionTree(root);
        printf("结果是：%d\n\n", result);

        freeExpressionTree(root);
        free(inputCopy);
    }
}
