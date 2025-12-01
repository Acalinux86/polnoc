#include <errno.h>

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

bool plc_parse_tokens(const Plc_Tokens *tokens)
{
    for (size_t i = 0; i < )
    return true;
}
