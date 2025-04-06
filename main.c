#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <limits.h>
#include <math.h>

typedef struct {
    double (*func)(double, double);  // 改为支持double类型的函数指针
    int precedence;
    const char *name;
} Operator;

typedef struct TreeNode {
    union {
        struct {
            double value;  // 操作数值改为double类型
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
TreeNode* createOperandNode(double value);
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
double evaluateExpressionTree(TreeNode *node);



// 计算表达式树的结果
/**
 * 计算表达式树的值
 * @param node 表达式树的根节点
 * @return 计算结果(double类型)
 * @note 递归计算左右子树的值，然后应用操作符函数
 */
double evaluateExpressionTree(TreeNode *node) {
    if (node == NULL) return 0; // 空节点返回0
    if (node->type == 0) return node->data.operand.value; // 叶子节点直接返回值

    // 递归计算左右子树的值
    double leftValue = evaluateExpressionTree(node->left);
    double rightValue = evaluateExpressionTree(node->right);

    // 调用操作符函数并返回结果
    return (*(node->data.operator.func))(leftValue, rightValue);
}
// 操作符函数定义
/**
 * 加法运算
 * @param a 左操作数
 * @param b 右操作数
 * @return 两数之和
 */
double add(double a, double b) { return a + b; }

/**
 * 减法运算
 * @param a 左操作数
 * @param b 右操作数
 * @return 两数之差
 */
double sub(double a, double b) { return a - b; }

/**
 * 乘法运算
 * @param a 左操作数
 * @param b 右操作数
 * @return 两数之积
 */
double mul(double a, double b) { return a * b; }

/**
 * 除法运算(带除零检查)
 * @param a 被除数
 * @param b 除数
 * @return 两数之商
 * @note 当除数接近0时返回NaN并输出错误信息
 */
double div_func(double a, double b) {
    if (fabs(b) < 1e-10) {
        fprintf(stderr, "错误: 除数为0或接近0(%.10e)，计算中止\n", b);
        return NAN;
    }
    return a / b;
}

// 预定义的操作符数组
/**
 * 操作符定义表
 * 包含: 函数指针, 优先级, 操作符名称
 * 优先级说明: 1-最低, 2-中等, 3-最高
 */
static const Operator operators[] = {
        { add, 1, "add" },    // 加法, 优先级1
        { sub, 1, "sub" },    // 减法, 优先级1
        { mul, 2, "mul" },    // 乘法, 优先级2
        { div_func, 2, "div" }, // 除法, 优先级2
        { fmod, 2, "mod" }   // 取模, 优先级2
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
/**
 * 解析操作数(支持浮点数)
 * @param input 输入字符串指针的指针
 * @return 解析后的操作数节点，失败返回NULL
 * @note 支持负数、整数和小数部分
 */
TreeNode* parseOperand(char **input) {
    double value = 0.0;
    double sign = 1.0;
    double fraction = 0.0;
    int decimalPlaces = 0;

    // 处理负数符号
    if (**input == '-') {
        sign = -1.0;
        (*input)++;
    }

    // 解析整数部分
    while (**input && isdigit((unsigned char)**input)) {
        value = value * 10 + (**input - '0');
        (*input)++;
    }

    // 解析小数部分
    if (**input == '.') {
        (*input)++;
        while (**input && isdigit((unsigned char)**input)) {
            fraction = fraction * 10 + (**input - '0');
            decimalPlaces++;
            (*input)++;
        }
        if (decimalPlaces > 0) {
            value += fraction / pow(10, decimalPlaces);
        }
    }

    value *= sign;

    // 验证后续字符有效性
    if (**input && !isspace((unsigned char)**input) && **input != ')' && !isalpha((unsigned char)**input)) {
        fprintf(stderr, "错误: 数字后出现非法字符'%c'\n", **input);
        return NULL;
    }

    // 检查括号匹配
    if (**input == '(') {
        int parenCount = 1;
        char *temp = *input + 1;
        while (*temp && parenCount > 0) {
            if (*temp == '(') parenCount++;
            if (*temp == ')') parenCount--;
            temp++;
        }
        if (parenCount != 0) {
            fprintf(stderr, "错误: 缺少右括号\n");
            return NULL;
        }
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
/**
 * 解析表达式因子（数字或括号内的表达式）
 * @param input 输入字符串指针的指针
 * @return 解析后的表达式树节点
 */
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
    } 
    
    if (isdigit((unsigned char)**input) || **input == '-') {
        return parseOperand(input); // 处理数字和负号
    }
    
    printf("无效内容: '%c'\n", **input);
    return NULL;
}

// 解析表达式（根据优先级）
/**
 * 解析表达式(根据优先级)
 * @param input 输入字符串指针的指针
 * @param precedence 当前优先级
 * @return 解析后的表达式树节点
 * @note 使用递归下降法解析表达式，处理操作符优先级
 */
/**
 * 解析表达式（根据优先级）
 * @param input 输入字符串指针的指针
 * @param precedence 当前优先级
 * @return 解析后的表达式树节点
 */
TreeNode* parseExpression(char **input, int precedence) {
    TreeNode* left = parseFactor(input);
    if (left == NULL) {
        return NULL; // 因子解析失败
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
            freeExpressionTree(left); // 右子树解析失败，释放已分配内存
            return NULL;
        }

        TreeNode* newNode = createOperatorNode(op);
        if (newNode == NULL) {
            freeExpressionTree(left);
            freeExpressionTree(right);
            return NULL; // 内存分配失败
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
TreeNode* createOperandNode(double value) {
    TreeNode* node = (TreeNode*)malloc(sizeof(TreeNode));
    if (node == NULL) {
        perror("malloc failed");
        return NULL;
    }
    node->data.operand.value = value;
    node->type = 0;
    node->left = NULL;
    node->right = NULL;
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
    return node;
}

// 释放表达式树内存
void freeExpressionTree(TreeNode *node) {
    if (node == NULL) return;
    freeExpressionTree(node->left);
    freeExpressionTree(node->right);
    free(node);
}

#ifdef _WIN32
#include <windows.h>
#else
#include <locale.h>
#endif

int main() {
    // 设置控制台编码为UTF-8
#ifdef _WIN32
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
#else
    setlocale(LC_ALL, "en_US.UTF-8");
#endif
    
    while (1) {
        char input[100];
        printf("请输入表达式字符串（如1 add 2）：\n");
        if (fgets(input, sizeof(input), stdin) == NULL) {
            fprintf(stderr, "输入错误\n");
            continue;
        }
        input[strcspn(input, "\n")] = '\0'; // 去除换行符

        printf("\n=== 解析过程 ===\n");
        printf("原始输入: %s\n", input);

        char *inputCopy = strdup(input);
        if (inputCopy == NULL) {
            perror("strdup failed");
            continue;
        }

        TreeNode* root = buildExpressionTree(inputCopy);
        if (root == NULL) {
            fprintf(stderr, "表达式解析失败，请检查输入格式\n");
            free(inputCopy);
            continue;
        }

        printf("语法树构建成功，开始计算...\n");
        double result = evaluateExpressionTree(root);
        if (isnan(result)) {
            fprintf(stderr, "计算过程中出现错误，请检查表达式\n");
            freeExpressionTree(root);
            free(inputCopy);
            continue;
        } else {
            printf("\n=== 计算结果 ===\n");
            printf("表达式: %s\n", input);
            printf("结果: %.15g\n\n", result);
        }

        freeExpressionTree(root);
        free(inputCopy);
    }
    return 0;
}