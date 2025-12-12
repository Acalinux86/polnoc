#include <errno.h>

#include "./polnoc_lexer.h"
#include "./polnoc_parser.h"

double plc_parse_number(const char *number_string)
{
    char *endptr;
    const char *str;
    double number = 0;

    errno = 0;
    str = number_string;
    number = strtold(str, &endptr);
    if (errno == ERANGE) {
        perror("strtold");
        exit(EXIT_FAILURE);
    }

    if (endptr == str) {
        fprintf(stderr, "No digits were found\n");
        exit(EXIT_FAILURE);
    }
    return number;
}

Plc_Token plc_parser_pop_token(Plc_Tokens *stack)
{
    if (stack->count <= 0) {
        fprintf(stderr, "[ERROR] Attempting to Pop From Empty Stack\n");
        exit(EXIT_FAILURE);
    }
    return stack->contents[--stack->count];
}

// Compare strings
#define plc_cmp(str, str_lit) (strcmp((str)->contents, str_lit) == 0)

bool plc_parse_tokens(const Plc_Tokens *tokens, Plc_Tokens *stack)
{
    memset(stack, 0, sizeof(*stack));
    for (size_t i = 0; i < tokens->count; ++i) {
        Plc_Token *token = &tokens->contents[i];
        switch (token->type) {
        case PLC_TOKEN_NUMBER: {
            dyn_array_append(stack, *token);
        } break;

        case PLC_TOKEN_PLUS: {
            double right = plc_parser_pop_token(stack).data.number;
            double left  = plc_parser_pop_token(stack).data.number;

            Plc_Token t   = {0};
            t.type        = PLC_TOKEN_NUMBER;
            t.data.number = left + right;
            dyn_array_append(stack, t);
        } break;

        case PLC_TOKEN_MINUS: {
            double right = plc_parser_pop_token(stack).data.number;
            double left  = plc_parser_pop_token(stack).data.number;

            Plc_Token t   = {0};
            t.type        = PLC_TOKEN_NUMBER;
            t.data.number = left - right;
            dyn_array_append(stack, t);
        } break;

        case PLC_TOKEN_DIV: {
            double right = plc_parser_pop_token(stack).data.number;
            double left  = plc_parser_pop_token(stack).data.number;
            if (right == 0.000000) {
                fprintf(stderr, "ERROR: Division by `%lf`", right);
                return false;
            }

            Plc_Token t   = {0};
            t.type        = PLC_TOKEN_NUMBER;
            t.data.number = left / right;
            dyn_array_append(stack, t);
        } break;

        case PLC_TOKEN_MULT: {
            double right = plc_parser_pop_token(stack).data.number;
            double left  = plc_parser_pop_token(stack).data.number;

            Plc_Token t   = {0};
            t.type        = PLC_TOKEN_NUMBER;
            t.data.number = left * right;
            dyn_array_append(stack, t);
        } break;

        case PLC_TOKEN_STRING: {
            assert(stack->count > 0);
            Plc_Token top = stack->contents[0];
            if (plc_cmp(&token->data.string, "print")) {
                fprintf(stdout, "INFO: Value: %.2lf, Type: %s\n", top.data.number, plc_token_type_as_cstr(top.type));
            } else if (plc_cmp(&token->data.string, "dup")) {
                dyn_array_append(stack, top);
            } else {
                fprintf(stdout, "ERROR: Unknown token `%s`\n", token->data.string.contents);
                continue;
            }
        } break;

        default:
            assert(0 && "unreachable token type");
        }
    }
    return true;
}
