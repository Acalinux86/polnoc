#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

#include "./polnoc_lexer.h"
// #include "./polnoc_parser.h"
#include "./dyn_array.h"

#define PLC_MAX_BUF_LEN 256

const char *plc_token_type_as_cstr(const Plc_TokenType type)
{
    switch (type) {
    case PLC_TOKEN_NUMBER: return "PLC_TOKEN_NUMBER";
    case PLC_TOKEN_PLUS:   return "PLC_TOKEN_PLUS";
    case PLC_TOKEN_MINUS:  return "PLC_TOKEN_MINUS";
    case PLC_TOKEN_DIV:    return "PLC_TOKEN_DIV";
    case PLC_TOKEN_MULT:   return "PLC_TOKEN_MULT";
    case PLC_TOKEN_FUNC:   return "PLC_TOKEN_FUNC";
    default:
        fprintf(stderr, "[PANIC] Unreachable TOKEN TYPE\n");
        return NULL;
    }
}

static inline bool plc_is_opcode(char x)
{
    return x == '+' || x == '*' || x == '/' || x == '-';
}

Plc_Lexer plc_lexer_init(const char *source, size_t size)
{
    Plc_Lexer lexer   = {0};
    lexer.cursor      = 0;
    lexer.content     = source;
    lexer.content_len = size;
    return lexer;
}

char plc_lexer_peek(const Plc_Lexer *lexer)
{
    return lexer->content[lexer->cursor];
}

// Compare strings
#define plc_cmp(str, str_lit) (strcmp((str)->contents, str_lit) == 0)

char plc_lexer_advance(Plc_Lexer *lexer)
{
    return lexer->content[lexer->cursor++];
}

bool plc_lexer_end(const Plc_Lexer *lexer)
{
    return lexer->cursor == lexer->content_len;
}

bool plc_lexer_tokenize(Plc_Lexer *l, Plc_Tokens *tokens)
{
    if (!tokens)     return false;
    if (!l->content) return false;

    char c;
    while (!plc_lexer_end(l)) {
        Plc_Token token = {0};
        c = plc_lexer_peek(l);
        if (isalpha(c)) {
            dyn_array_append(&token.data.string, c);
            plc_lexer_advance(l);

            while (!isspace(plc_lexer_peek(l)) && plc_lexer_peek(l) != '\0') {
                c = plc_lexer_peek(l);
                dyn_array_append(&token.data.string, c);
                plc_lexer_advance(l);
            }
            dyn_array_append(&token.data.string, '\0');

            if (plc_cmp(&token.data.string, "print")) {
                token.type = PLC_TOKEN_FUNC;
                dyn_array_append(tokens, token);
            } else {
                fprintf(stderr, "ERROR: Unknown Token: `%s`\n", token.data.string.contents);
                dyn_array_delete(&token.data.string);
                return false;
            }
        } else if (isspace(c)) {
            plc_lexer_advance(l);
            dyn_array_delete(&token.data.string);
        } else {
            if (plc_is_opcode(c)) {
                plc_lexer_advance(l);
                if (c == '+') {
                    dyn_array_append(&token.data.string, c);
                    dyn_array_append(&token.data.string, '\0');
                    token.type = PLC_TOKEN_PLUS;
                    dyn_array_append(tokens, token);
                } else if (c == '-') {
                    dyn_array_append(&token.data.string, c);
                    dyn_array_append(&token.data.string, '\0');
                    token.type = PLC_TOKEN_MINUS;
                    dyn_array_append(tokens, token);
                } else if (c == '*') {
                    dyn_array_append(&token.data.string, c);
                    dyn_array_append(&token.data.string, '\0');
                    token.type = PLC_TOKEN_MULT;
                    dyn_array_append(tokens, token);
                } else if (c == '/') {
                    dyn_array_append(&token.data.string, c);
                    dyn_array_append(&token.data.string, '\0');
                    token.type = PLC_TOKEN_DIV;
                    dyn_array_append(tokens, token);
                } else {
                    // Unknown Opcode
                    return false;
                }
            } else if (isdigit(c)) {
                // Consume Rest of Number
                dyn_array_append(&token.data.string, c);
                plc_lexer_advance(l);

                while (isdigit(plc_lexer_peek(l)) || plc_lexer_peek(l) == '.') {
                    c = plc_lexer_peek(l);
                    dyn_array_append(&token.data.string, c);
                    plc_lexer_advance(l);
                }
                dyn_array_append(&token.data.string, '\0');

                const char *str_num = token.data.string.contents;
                double number = plc_parse_number(str_num);
                dyn_array_delete(&token.data.string);

                token.type = PLC_TOKEN_NUMBER;
                token.data.number = number;
                dyn_array_append(tokens, token);
            }
        }
    }
    return true;
}
