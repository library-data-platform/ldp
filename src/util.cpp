#include <cstring>

#include "util.h"

bool is_uuid(const char* str)
{
    if (strlen(str) != 36)
        return false;
    for (int x = 0; x < 36; x++) {
        char c = str[x];
        if (x == 8 || x == 13 || x == 18 || x == 23) {
            if (c != '-')
                return false;
        } else {
            switch (c) {
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
            case 'A':
            case 'B':
            case 'C':
            case 'D':
            case 'E':
            case 'F':
            case 'a':
            case 'b':
            case 'c':
            case 'd':
            case 'e':
            case 'f':
                break;
            default:
                return false;
            }
        }
    }
    return true;
}

void vacuum_sql(const ldp_options& opt, string* sql)
{
    if (opt.parallel_vacuum) {
        *sql = "VACUUM ";
    } else {
        *sql = "VACUUM (PARALLEL 0) ";
    }
}

void print_banner_line(FILE* stream, char ch, int width)
{
    for (int x = 0; x < width; x++)
        fputc(ch, stream);
    fputc('\n', stream);
}

source_state::source_state(data_source source)
{
    this->source = source;
}

source_state::~source_state()
{
}

