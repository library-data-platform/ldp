#include <cassert>
#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <sys/stat.h>

#include "../include/util.h"

namespace etymon {

File::File(const string& pathname, const char* mode)
{
    file = fopen(pathname.c_str(), mode);
    if (file == NULL)
        throw runtime_error("Error opening file: " + pathname + ": " +
                string(strerror(errno)));
}

File::~File()
{
    if (file)
        fclose(file);
}

CommandArgs::CommandArgs(int argc, char* const argv[])
{
    this->argc = argc - 1;
    this->argv = (char**) malloc(sizeof(char*) * this->argc);
    this->argv[0] = argv[0];
    this->command = argc >= 2 ? argv[1] : "";
    for (int x = 2; x < argc; x++)
        this->argv[x - 1] = argv[x];
}

CommandArgs::~CommandArgs()
{
    free(argv);
}

bool fileExists(const string& filename) {
    struct stat st;   
    if (stat(filename.c_str(), &st) == 0)
        return true;
    else
        return false;
}

void join(string* s1, const string& s2)
{
    if (s1->empty()) {
        *s1 = s2;
        return;
    }
    if (s2.empty()) {
        return;
    }
    assert(s1->length() > 0);
    assert(s2.length() > 0);
    int count = 0;
    if ((*s1)[s1->length() - 1] == '/')
        count++;
    if (s2[0] == '/')
        count++;
    assert(0 <= count && count <= 2);
    switch (count) {
        case 0:
            *s1 += '/';
            *s1 += s2;
            break;
        case 1:
            *s1 += s2;
            break;
        case 2:
            *s1 += (s2.c_str()) + 1;
            break;
    }
}

/**
 * \brief Trims leading and trailing white space from a string.
 *
 * \param[in,out] s The string to be trimmed.
 */
void trim(string* s)
{
    const char* p = s->c_str();
    const char* q = p + s->length();
    do {
        if (*p == '\0') {
            s->clear();
            return;
        }
        if (!isspace(*p))
            break;
        p++;
    } while (true);
    do {
        if (p == q) {
            s->clear();
            return;
        }
        if (!isspace(*(q - 1)))
            break;
        q--;
    } while (true);
    size_t pqlen = q - p;
    char* tmp = (char*) malloc(pqlen + 1);
    memcpy(tmp, p, pqlen);
    tmp[pqlen] = '\0';
    *s = tmp;
    free(tmp);
}

void split(const string& str, char delim, vector<string>* v)
{
    v->clear();
    string s;
    const char* mark = str.c_str();
    const char* p = mark;
    while (*p != '\0') {
        if (*p == delim) {
            s.assign(mark, p - mark);
            v->push_back(s);
            mark = p + 1;
        }
        p++;
    }
    s.assign(mark, p - mark);
    v->push_back(s);
}

void prefixLines(string* str, const char* prefix)
{
    trim(str);
    vector<string> v;
    split(*str, '\n', &v);
    str->clear();
    for (const string& s : v) {
        (*str) += prefix;
        (*str) += s;
        (*str) += '\n';
    }
}

}

