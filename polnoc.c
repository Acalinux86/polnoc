#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

#define RPNI_INIT_CAP 5
#define RPNI_TRACE 0

char *rpni_read_file_into_memory(const char *rpni_file_path, size_t *rpni_file_size)
{
    FILE *fp = fopen(rpni_file_path, "rb");
    if (fp == NULL) {
        fprintf(stderr, "[ERROR] Could not read `%s`: `%s`\n", rpni_file_path, strerror(errno));
        return NULL;
    }

    int ret = fseek(fp, 0, SEEK_END);
    if (ret < 0) {
        fprintf(stderr, "[ERROR] Could not seek to end of `%s`: `%s`\n", rpni_file_path, strerror(errno));
        fclose(fp);
        return NULL;
    }

    size_t size = ftell(fp);
    if (size <= 0) {
        fprintf(stderr, "[ERROR] File `%s` Empty\n", rpni_file_path);
        fclose(fp);
        return NULL;
    }

    rewind(fp); // Rewind the file pointer to the beginning

    char *buffer = (char *)malloc(size + 1);
    if (buffer == NULL) {
        fprintf(stderr, "[ERROR] Memory Allocation Failed\n");
        fclose(fp);
        return NULL;
    }

    size_t bytes = fread(buffer, sizeof(*buffer), size, fp);
    if (bytes != (size_t)size) {
        fprintf(stderr, "fread() failed: Expected %ld bytes got %zu bytes\n", size, bytes);
        free(buffer);
        fclose(fp);
        return NULL;
    }
    buffer[bytes] = '\0'; // Null Terminate

    *rpni_file_size = size;
    fclose(fp); // close file pointer
    return buffer;
}

const char *rpni_shift_args(int *argc, char ***argv)
{
    const char *result = **argv;
    (*argc)--;
    (*argv)++;
    return result;
}

typedef struct RPNI_String {
    char   *c_str;
    size_t len;
    int row;
    int col;
} RPNI_String;

typedef struct RPNI_RawList {
    RPNI_String *items;
    size_t count;
    size_t capacity;
} RPNI_RawList;

typedef struct RPNI_Lexer {
    int col;
    int row;
    int cursor;
    const char *content_path;
    const char *content;
    size_t content_len;
} RPNI_Lexer;

typedef enum RPNI_TokenType {
    RPNI_TOKEN_NUMBER,
    RPNI_TOKEN_PLUS,
    RPNI_TOKEN_MINUS,
    RPNI_TOKEN_DIV,
    RPNI_TOKEN_MULT,
    RPNI_TOKEN_FUNC,
} RPNI_TokenType;

const char *rpni_token_type_as_cstr(RPNI_TokenType type)
{
    switch (type) {
    case RPNI_TOKEN_NUMBER: return "RPNI_TOKEN_NUMBER";
    case RPNI_TOKEN_PLUS:   return "RPNI_TOKEN_PLUS";
    case RPNI_TOKEN_MINUS:  return "RPNI_TOKEN_MINUS";
    case RPNI_TOKEN_DIV:    return "RPNI_TOKEN_DIV";
    case RPNI_TOKEN_MULT:   return "RPNI_TOKEN_MULT";
    case RPNI_TOKEN_FUNC:   return "RPNI_TOKEN_FUNC";
    default:
        fprintf(stderr, "[PANIC] Unreachable TOKEN TYPE\n");
        return NULL;
    }
}

typedef union RPNI_TokenData {
    double number;
    char   *string;
} RPNI_TokenData;

typedef struct RPNI_Token {
    RPNI_TokenType type;
    RPNI_TokenData data;
    int row;
    int col;
} RPNI_Token;

struct RPNI_Tokens {
    RPNI_Token *items;
    size_t count;
    size_t capacity;
};

typedef struct RPNI_Tokens RPNI_Stack;
typedef struct RPNI_Tokens RPNI_Tokens;

#define rpni_array_push(array, item)                                    \
    do {                                                                \
        if ((array)->count >= (array)->capacity) {                      \
            (array)->capacity =                                         \
                (array)->capacity == 0 ?                                \
                RPNI_INIT_CAP :                                         \
                (array)->capacity * 2;                                  \
            (array)->items =                                            \
                realloc((array)->items,                                 \
                        (array)->capacity*(sizeof(*(array)->items)));   \
            if ((array)->items == NULL) {                               \
                fprintf(stderr, "[ERROR] Memory Reallocation for Array Failed\n"); \
                exit(EXIT_FAILURE);                                     \
            }                                                           \
        }                                                               \
        (array)->items[(array)->count++] = item;                        \
    } while (0)

RPNI_Token rpni_stack_pop(RPNI_Stack *stack)
{
    if (stack->count <= 0) {
        fprintf(stderr, "[ERROR] Attempting to Pop From Empty Stack\n");
        exit(EXIT_FAILURE);
    }
    return stack->items[--stack->count];
}

bool rpni_is_opcode(char x)
{
    return x == '+' || x == '*' || x == '/' || x == '-';
}

char *rpni_strdup(const char *src, const size_t src_len)
{
    char *dest = (char *)malloc(sizeof(char)*(src_len + 1));
    if (dest == NULL) {
        fprintf(stderr, "[ERROR] Memory Allocation For String Duplication failed\n");
        return NULL;
    }
    memcpy(dest, src, sizeof(char)*src_len);
    dest[src_len] = '\0';
    return dest;
}

bool rpni_tokenize_source_string(RPNI_Lexer *l, RPNI_RawList *list)
{
    if (!list)     return false;
    if (!l->content)   return false;

    const size_t OUT = 1;
    const size_t IN  = 0;
    size_t state = OUT;

    char buf[256] = {0};
    size_t cursor = 0;
    size_t n = 0;

    while (n <= l->content_len) {
        char c = l->content[n];
        if (isspace(c)) {
            if (cursor > 0) {
                rpni_array_push(list, ((RPNI_String){rpni_strdup(buf, cursor), cursor, l->row, l->cursor - cursor}));
            }
            cursor = 0;
            state = OUT;
            if (c == '\n') {
                l->row += 1;
                l->col = 1;
            }
        } else if (state == OUT) {
            if (rpni_is_opcode(c)) {
                rpni_array_push(list, ((RPNI_String){rpni_strdup(&c, 1), 1, l->row, l->cursor - 1}));
            } else {
                buf[cursor++] = c;
                state = IN;
            }
        } else if (state == IN) {
            if (rpni_is_opcode(c)) {
                rpni_array_push(list, ((RPNI_String){rpni_strdup(buf, cursor), cursor, l->row, l->cursor - cursor}));
                cursor = 0;

                rpni_array_push(list, ((RPNI_String){rpni_strdup(&c, 1), 1, l->row, l->cursor - 1}));
                state = OUT;
            } else {
                buf[cursor++] = c;
            }
        }
        n++;
        l->cursor++;
    }
    return true;
}

bool rpni_dump_raw_list(const RPNI_RawList *list)
{
    if (!list) return false;

    for (size_t i = 0; i < list->count; ++i) {
        printf("[INFO] Token: %s, Size: %zu\n", list->items[i].c_str, list->items[i].len);
    }
    return true;
}

double rpni_parse_number(char *number_string)
{
    char *endptr, *str;
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

bool rpni_is_string_number(RPNI_String *str)
{
    for (size_t i = 0; i < str->len; ++i) {
        char c = str->c_str[i];
        if (!(isdigit(c) || c == '.')) {
            return false;
        }
    }
    return true;
}

bool rpni_tokenize_raw_list(const RPNI_Lexer *l, const RPNI_RawList *list, RPNI_Tokens *tokens)
{
    if (!list)   return false;
    if (!tokens) return false;
    if (!l) return false;

    for (size_t i = 0; i < list->count; ++i) {
        RPNI_Token token = {0};
        RPNI_String *string = &list->items[i];
        if (isdigit(string->c_str[0])) { // if digit occurred , parse number
            if (!rpni_is_string_number(string)) {
                fprintf(stderr, "%s:%d:%d:Error Occurred While Parsing Number: %s\n", l->content_path, string->row, string->col, string->c_str);
                return false;
            }
            token.data.number = rpni_parse_number(string->c_str);
            token.type = RPNI_TOKEN_NUMBER;
            token.row = string->row;
            token.col = string->col;
            rpni_array_push(tokens, token);
        } else if (rpni_is_opcode(string->c_str[0])) {
            // parse opcode
            char opcode = string->c_str[0];
            switch (opcode) {
            case '+': {
                token.data.string = string->c_str;
                token.type = RPNI_TOKEN_PLUS;
                token.row = string->row;
                token.col = string->col;
                rpni_array_push(tokens, token);
            } break;

            case '/': {
                token.data.string = string->c_str;
                token.type = RPNI_TOKEN_DIV;
                rpni_array_push(tokens, token);
            } break;

            case '-': {
                token.data.string = string->c_str;
                token.type = RPNI_TOKEN_MINUS;
                token.row = string->row;
                token.col = string->col;
                rpni_array_push(tokens, token);
            } break;

            case '*': {
                token.data.string = string->c_str;
                token.type = RPNI_TOKEN_MULT;
                token.row = string->row;
                token.col = string->col;
                rpni_array_push(tokens, token);
            } break;

            default:
                fprintf(stderr, "%s:%d:%d:ERROR Expected Opcode, Got: %s\n", l->content_path, string->row, string->col, string->c_str);
                return false;
            }
        } else if (isalnum(string->c_str[0])) {
            token.data.string = string->c_str;
            token.type = RPNI_TOKEN_FUNC;
            token.row = string->row;
            token.col = string->col;
            rpni_array_push(tokens, token);
        } else {
            fprintf(stderr, "%s:%d:%d:ERROR Unrecognized Token: %s\n", l->content_path, string->row, string->col, string->c_str);
            return false;
        }
        // free(list->items[i].c_str);
    }

    return true;
}

bool rpni_dump_tokens(const RPNI_Tokens *tokens);

bool rpni_eval_exprs(const RPNI_Lexer *l, const RPNI_Tokens *tokens, RPNI_Stack *stack)
{
    if (!stack)  return false;
    if (!tokens) return false;

    for (size_t i = 0; i < tokens->count; ++i) {
        RPNI_Token token = tokens->items[i];
        switch (token.type) {
        case RPNI_TOKEN_FUNC: {
            rpni_array_push(stack, token);
            RPNI_Token token = rpni_stack_pop(stack);
            if (strcmp(token.data.string, "print") == 0) {
                if (!rpni_dump_tokens((RPNI_Tokens*)stack)) return false;
            } else {
                fprintf(stderr, "%s:%d:%d:ERROR Unrecognized Token `%s`\n", l->content_path, token.row, token.col, token.data.string);
                return false;
            }
        } break;

        case RPNI_TOKEN_NUMBER: {
            rpni_array_push(stack, token);
        } break;

        case RPNI_TOKEN_PLUS: {
            double right = rpni_stack_pop(stack).data.number;
            double left  = rpni_stack_pop(stack).data.number;

            RPNI_Token token = {0};
            token.type = RPNI_TOKEN_NUMBER;
            token.data.number = left + right;
            rpni_array_push(stack, token);
        } break;

        case RPNI_TOKEN_MINUS: {
            double right = rpni_stack_pop(stack).data.number;
            double left  = rpni_stack_pop(stack).data.number;

            RPNI_Token token = {0};
            token.type = RPNI_TOKEN_NUMBER;
            token.data.number = left - right;
            rpni_array_push(stack, token);
        } break;

        case RPNI_TOKEN_DIV: {
            double right = rpni_stack_pop(stack).data.number;
            double left  = rpni_stack_pop(stack).data.number;

            if (right == 0) {
                fprintf(stderr, "[PANIC] Division By %lf\n", right);
                return false;
            }

            RPNI_Token token = {0};
            token.type = RPNI_TOKEN_NUMBER;
            token.data.number = left / right;
            rpni_array_push(stack, token);
        } break;

        case RPNI_TOKEN_MULT: {
            double right = rpni_stack_pop(stack).data.number;
            double left  = rpni_stack_pop(stack).data.number;

            RPNI_Token token = {0};
            token.type = RPNI_TOKEN_NUMBER;
            token.data.number = left * right;
            rpni_array_push(stack, token);
        } break;

        default:
            fprintf(stderr, "[PANIC] Unreachable TOKEN TYPE\n");
            return false;
        }
    }

    return true;
}

bool rpni_dump_tokens(const RPNI_Tokens *tokens)
{
    if (!tokens) return false;
    for (size_t i = 0; i < tokens->count; ++i) {
        RPNI_Token *token = &tokens->items[i];
        switch (token->type) {
        case RPNI_TOKEN_NUMBER: {
            printf("[INFO] Type: %s, Token: %lf\n", rpni_token_type_as_cstr(token->type), token->data.number);
        } break;

        case RPNI_TOKEN_FUNC:
        case RPNI_TOKEN_DIV:
        case RPNI_TOKEN_MULT:
        case RPNI_TOKEN_MINUS:
        case RPNI_TOKEN_PLUS:
            printf("[INFO] Type: %s, Token: %s\n", rpni_token_type_as_cstr(token->type), token->data.string);
            break;
        default:
            fprintf(stderr, "[PANIC] Unreachable TOKEN TYPE\n");
            return false;
        }
    }
    return true;
}

void rpni_init_cursor(RPNI_Lexer *lexer, const char *path)
{
    lexer->row = 1;
    lexer->col = 1;
    lexer->cursor = 1;
    lexer->content_path = path;
    lexer->content = rpni_read_file_into_memory(lexer->content_path, &lexer->content_len);
    if (lexer->content == NULL) return ;
}

int main(int argc, char **argv)
{
    const char *program = rpni_shift_args(&argc, &argv);
    if (argc < 1) {
        fprintf(stderr, "[USAGE] %s <input_file>\n", program);
        return 1;
    }

    const char *source = rpni_shift_args(&argc, &argv);

    RPNI_RawList list = {0};
    RPNI_Lexer lexer = {0};
    rpni_init_cursor(&lexer, source);
    if (!rpni_tokenize_source_string(&lexer, &list)) {
        return 1;
    }

#if RPNI_TRACE
    if (!rpni_dump_raw_list(&list)) {
        // Clean up
        free(list.items);
        return 1;
    }
#endif

    RPNI_Tokens tokens = {0};
    if (!rpni_tokenize_raw_list(&lexer, &list, &tokens)) {
        // Clean up
        free(list.items);
        return 1;
    }

#if RPNI_TRACE
    if (!rpni_dump_tokens(&tokens)) {
        // Clean up
        free(list.items);
        free(tokens.items);
        return 1;
    }
#endif

    RPNI_Stack stack = {0};
    // Reverse Polish Notation Algorithm
    if (!rpni_eval_exprs(&lexer, &tokens, &stack)) {
        // Clean up
        free(stack.items);
        free(list.items);
        free(tokens.items);
        return 1;
    }

#if RPNI_TRACE
    if (!rpni_dump_tokens((RPNI_Tokens*)&stack)) {
        // Clean up
        free(stack.items);
        free(list.items);
        free(tokens.items);
        return 1;
    }
#endif

    // Clean up
    free(stack.items);
    free(list.items);
    free(tokens.items);
    return 0;
}
