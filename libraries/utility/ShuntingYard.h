// Copyright 2011 - 2014 Brian Marshall. All rights reserved.
//
// Use of this source code is governed by the BSD 2-Clause License that can be
// found in the LICENSE file.

#include <ctype.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

template <typename TYPE>
class ShuntingYard
{
    struct Stack {
        const void *value;
        Stack *next;
    };

    void stack_push(Stack **stack, const void *value) {
        Stack *item = (Stack *)malloc(sizeof(Stack));
        item->value = value;
        item->next = *stack;
        *stack = item;
    }

    const void *stack_pop(Stack **stack) {
        Stack *item = *stack;
        const void *value = item->value;
        *stack = item->next;
        free(item);
        return value;
    }

    const void *stack_top(const Stack *stack) {
        return stack->value;
    }

    typedef enum {
        OK,
        ERROR_SYNTAX,
        ERROR_OPEN_PARENTHESIS,
        ERROR_CLOSE_PARENTHESIS,
        ERROR_UNRECOGNIZED,
        ERROR_NO_INPUT,
        ERROR_UNDEFINED_FUNCTION,
        ERROR_FUNCTION_ARGUMENTS,
        ERROR_UNDEFINED_CONSTANT
    } Status;

    typedef enum {
        TOKEN_NONE,
        TOKEN_UNKNOWN,
        TOKEN_OPEN_PARENTHESIS,
        TOKEN_CLOSE_PARENTHESIS,
        TOKEN_OPERATOR,
        TOKEN_NUMBER,
        TOKEN_IDENTIFIER
    } TokenType;

    typedef struct {
        TokenType type;
        char *value;
    } Token;

    typedef enum {
        OPERATOR_OTHER,
        OPERATOR_UNARY,
        OPERATOR_BINARY
    } OperatorArity;

    typedef enum {
        OPERATOR_NONE,
        OPERATOR_LEFT,
        OPERATOR_RIGHT
    } OperatorAssociativity;

    typedef struct {
        char symbol;
        int precedence;
        OperatorArity arity;
        OperatorAssociativity associativity;
    } Operator;

    static const Token NO_TOKEN = { TOKEN_NONE, NULL };

    static const Operator OPERATORS[] = {
        { '!', 1, OPERATOR_UNARY,  OPERATOR_LEFT },
        { '^', 2, OPERATOR_BINARY, OPERATOR_RIGHT },
        { '+', 3, OPERATOR_UNARY,  OPERATOR_RIGHT },
        { '-', 3, OPERATOR_UNARY,  OPERATOR_RIGHT },
        { '*', 4, OPERATOR_BINARY, OPERATOR_LEFT },
        { '/', 4, OPERATOR_BINARY, OPERATOR_LEFT },
        { '%', 4, OPERATOR_BINARY, OPERATOR_LEFT },
        { '+', 5, OPERATOR_BINARY, OPERATOR_LEFT },
        { '-', 5, OPERATOR_BINARY, OPERATOR_LEFT },
        { '(', 6, OPERATOR_OTHER,  OPERATOR_NONE }
    };

    Status push_operator(const Operator *_operator, Stack **operandList, Stack **operatorList)
    {
        if (!_operator)
            return ERROR_SYNTAX;

        Status status = OK;
        while (*operatorList && status == OK)
        {
            const Operator *stack_operator = stack_top(*operatorList);
            if (_operator->arity == OPERATOR_UNARY ||
                _operator->precedence < stack_operator->precedence ||
                (_operator->associativity == OPERATOR_RIGHT &&
                    _operator->precedence == stack_operator->precedence))
                break;

            status = apply_operator(stack_pop(operatorList), operandList);
        };

        stack_push(operatorList, _operator);
        return status;
    }

    Status push_multiplication(Stack **operandList, Stack **operatorList)
    {
        return push_operator(get_operator('*', OPERATOR_BINARY), operandList,
            operatorList);
    }

    void push_value(TYPE x, Stack **operandList)
    {
        TYPE *pointer = malloc(sizeof *pointer);
        *pointer = x;
        stack_push(operandList, pointer);
    }

    TYPE pop_value(Stack **operandList)
    {
        const TYPE *pointer = stack_pop(operandList);
        TYPE x = *pointer;
        free((void *)pointer);
        return x;
    }

    Status push_number(const char *value, Stack **operandList)
    {
        char *end_pointer = NULL;
        TYPE x = strtod(value, &end_pointer);

        // If not all of the value is converted, the rest is invalid.
        if (value + strlen(value) != end_pointer)
            return ERROR_SYNTAX;

        push_value(x, operandList);
        return OK;
    }

    Status push_constant(const char *value, Stack **operandList)
    {
        TYPE x = 0.0;
        if (strcasecmp(value, "e") == 0)
            x = M_E;
        else if (strcasecmp(value, "pi") == 0)
            x = M_PI;
        else if (strcasecmp(value, "tau") == 0)
            x = M_PI * 2;
        else
            return ERROR_UNDEFINED_CONSTANT;

        push_value(x, operandList);
        return OK;
    }

    Status apply_operator(const Operator *_operator, Stack **operandList)
    {
        if (!_operator || !*operandList)
            return ERROR_SYNTAX;
        if (_operator->arity == OPERATOR_UNARY)
            return apply_unary_operator(_operator, operandList);

        TYPE y = pop_value(operandList);
        if (!*operandList)
            return ERROR_SYNTAX;
        TYPE x = pop_value(operandList);
        Status status = OK;
        switch (_operator->symbol)
        {
        case '^':
            x = pow(x, y);
            break;
        case '*':
            x = x * y;
            break;
        case '/':
            x = x / y;
            break;
        case '%':
            x = fmod(x, y);
            break;
        case '+':
            x = x + y;
            break;
        case '-':
            x = x - y;
            break;
        default:
            return ERROR_UNRECOGNIZED;
        };

        push_value(x, operandList);
        return status;
    }

    Status apply_unary_operator(const Operator *_operator, Stack **operandList)
    {
        TYPE x = pop_value(operandList);
        switch (_operator->symbol)
        {
        case '+':
            break;
        case '-':
            x = -x;
            break;
        case '!':
            x = tgamma(x + 1);
            break;
        default:
            return ERROR_UNRECOGNIZED;
        }

        push_value(x, operandList);
        return OK;
    }

    Status apply_function(const char *function, Stack **operandList)
    {
        if (!*operandList)
            return ERROR_FUNCTION_ARGUMENTS;

        TYPE x = pop_value(operandList);
        if (strcasecmp(function, "abs") == 0)
            x = fabs(x);
        else if (strcasecmp(function, "sqrt") == 0)
            x = sqrt(x);
        else if (strcasecmp(function, "ln") == 0)
            x = log(x);
        else if (strcasecmp(function, "lb") == 0)
            x = log2(x);
        else if (strcasecmp(function, "lg") == 0 ||
            strcasecmp(function, "log") == 0)
            x = log10(x);
        else if (strcasecmp(function, "cos") == 0)
            x = cos(x);
        else if (strcasecmp(function, "sin") == 0)
            x = sin(x);
        else if (strcasecmp(function, "tan") == 0)
            x = tan(x);
        else
            return ERROR_UNDEFINED_FUNCTION;

        push_value(x, operandList);
        return OK;
    }

    OperatorArity get_arity(char symbol, const Token *previous)
    {
        if (symbol == '!' || previous->type == TOKEN_NONE ||
            previous->type == TOKEN_OPEN_PARENTHESIS ||
            (previous->type == TOKEN_OPERATOR && *previous->value != '!'))
            return OPERATOR_UNARY;
        return OPERATOR_BINARY;
    }

    const Operator *get_operator(char symbol, OperatorArity arity)
    {
        for (size_t i = 0; i < sizeof OPERATORS / sizeof OPERATORS[0]; i++)
        {
            if (OPERATORS[i].symbol == symbol && OPERATORS[i].arity == arity)
                return &OPERATORS[i];
        }

        return NULL;
    }

    Status parse(const Token *tokenList, Stack **operandList, Stack **operatorList, Stack **functionList)
    {
        Status status = OK;
        for (const Token *token = tokenList, *previous = &NO_TOKEN, *next = token + 1; token->type != TOKEN_NONE; previous = token, token = next++)
        {
            switch (token->type)
            {
            case TOKEN_OPEN_PARENTHESIS:
                // Implicit multiplication: "(2)(2)".
                if (previous->type == TOKEN_CLOSE_PARENTHESIS)
                    status = push_multiplication(operandList, operatorList);

                stack_push(operatorList, get_operator('(', OPERATOR_OTHER));
                break;

            case TOKEN_CLOSE_PARENTHESIS:
            {
                // Apply operatorList until the previous open parenthesis is found.
                bool found_parenthesis = false;
                while (*operatorList && status == OK && !found_parenthesis)
                {
                    const Operator *_operator = stack_pop(operatorList);
                    if (_operator->symbol == '(')
                        found_parenthesis = true;
                    else
                        status = apply_operator(_operator, operandList);
                }

                if (!found_parenthesis)
                    status = ERROR_CLOSE_PARENTHESIS;
                else if (*functionList)
                    status = apply_function(stack_pop(functionList), operandList);
                break;
            }

            case TOKEN_OPERATOR:
                status = push_operator(
                    get_operator(*token->value,
                        get_arity(*token->value, previous)),
                    operandList, operatorList);
                break;

            case TOKEN_NUMBER:
                if (previous->type == TOKEN_CLOSE_PARENTHESIS ||
                    previous->type == TOKEN_NUMBER ||
                    previous->type == TOKEN_IDENTIFIER)
                    status = ERROR_SYNTAX;
                else
                {
                    status = push_number(token->value, operandList);

                    // Implicit multiplication: "2(2)" or "2a".
                    if (next->type == TOKEN_OPEN_PARENTHESIS ||
                        next->type == TOKEN_IDENTIFIER)
                        status = push_multiplication(operandList, operatorList);
                }

                break;

            case TOKEN_IDENTIFIER:
                // The identifier could be either a constant or function.
                status = push_constant(token->value, operandList);
                if (status == ERROR_UNDEFINED_CONSTANT &&
                    next->type == TOKEN_OPEN_PARENTHESIS)
                {
                    stack_push(functionList, token->value);
                    status = OK;
                }
                else if (next->type == TOKEN_OPEN_PARENTHESIS ||
                    next->type == TOKEN_IDENTIFIER)
                {
                    // Implicit multiplication: "a(2)" or "a b".
                    status = push_multiplication(operandList, operatorList);
                }
                break;

            default:
                status = ERROR_UNRECOGNIZED;

            }
            if (status != OK)
                return status;
        }

        // Apply all remaining operatorList.
        while (*operatorList && status == OK)
        {
            const Operator *_operator = stack_pop(operatorList);
            if (_operator->symbol == '(')
                status = ERROR_OPEN_PARENTHESIS;
            else
                status = apply_operator(_operator, operandList);
        }

        return status;
    }

    Token *tokenize(const char *expression)
    {
        int length = 0;
        Token *tokenList = malloc(sizeof *tokenList);
        const char *c = expression;
        while (*c)
        {
            Token token = { TOKEN_UNKNOWN, NULL };
            if (*c == '(')
                token.type = TOKEN_OPEN_PARENTHESIS;
            else if (*c == ')')
                token.type = TOKEN_CLOSE_PARENTHESIS;
            else if (strchr("!^*/%+-", *c))
            {
                token.type = TOKEN_OPERATOR;
                token.value = strndup(c, 1);
            }
            else if (sscanf(c, "%m[0-9.]", &token.value))
                token.type = TOKEN_NUMBER;
            else if (sscanf(c, "%m[A-Za-z]", &token.value))
                token.type = TOKEN_IDENTIFIER;

            if (!isspace(*c))
            {
                tokenList = realloc(tokenList, sizeof *tokenList * (++length + 1));
                tokenList[length - 1] = token;
            }

            c += token.value ? strlen(token.value) : 1;
        };

        tokenList[length] = NO_TOKEN;
        return tokenList;
    }

    Status shunting_yard(const char *expression, TYPE *result)
    {
        Token *tokenList = tokenize(expression);
        Stack *operandList = NULL, *operatorList = NULL, *functionList = NULL;
        Status status = parse(tokenList, &operandList, &operatorList, &functionList);
        if (operandList)
            *result = round(pop_value(&operandList) * 10e14) / 10e14;
        else if (status == OK)
            status = ERROR_NO_INPUT;

        for (Token *token = tokenList; token->type != TOKEN_NONE; token++)
            free(token->value);
        free(tokenList);
        while (operandList)
            pop_value(&operandList);
        while (operatorList)
            stack_pop(&operatorList);
        while (functionList)
            stack_pop(&functionList);
        return status;
    }
};
