#ifndef POLNOC_LEXER_H
#define POLNOC_LEXER_H

#include "./dyn_array.h"

#define PLC_MAX_BUF_LEN 256

typedef enum {
    PLC_TOKEN_NUMBER,
    PLC_TOKEN_PLUS,
    PLC_TOKEN_MINUS,
    PLC_TOKEN_DIV,
    PLC_TOKEN_MULT,
    PLC_TOKEN_FUNC,
} Plc_TokenType;

typedef struct {
    char *contents;
    size_t count;
    size_t capacity;
} Plc_String;

typedef union {
    Plc_String string;
    double number;
} Plc_TokenData;

typedef struct {
    Plc_TokenData data;
    Plc_TokenType type;
} Plc_Token;

typedef struct {
    Plc_Token *contents;
    size_t count;
    size_t capacity;
} Plc_Tokens;

typedef struct {
    const char *content;
    size_t content_len;
    size_t cursor;
} Plc_Lexer;

const char *plc_token_type_as_cstr(Plc_TokenType type);
Plc_Lexer plc_lexer_init(const char *source, size_t size);
char plc_lexer_peek(const Plc_Lexer *lexer);
char plc_lexer_advance(Plc_Lexer *lexer);
bool plc_lexer_end(const Plc_Lexer *lexer);
bool plc_lexer_tokenize(Plc_Lexer *l, Plc_Tokens *tokens);

#endif // POLNOC_LEXER_H
