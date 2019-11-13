#include "camelcase.h"

static void writeChars(bool writeBreak, char write, string* newstr)
{
    if (writeBreak)
        (*newstr) += '_';
    (*newstr) += write;
}

/**
 * \brief Examines a sequence of three characters (c1, c2, c3) and decodes c2.
 *
 * The decoded output is appended to newstr.
 */
static void decodeTriple(char c1, char c2, char c3, string* newstr)
{
    bool c1u = isupper(c1);
    bool c2u = isupper(c2);
    bool c3u = isupper(c3);
    if (c2 == '\0')
        return;
    if ( (c1 == '\0' && c2 != '\0' && c3 != '\0') ||
            (c1 == '\0' && c2 != '\0' && c3 == '\0') ) {
        writeChars(false, tolower(c2), newstr);
        return;
    }
    if (c1 != '\0' && c2 != '\0' && c3 == '\0') {
        if ( !c1u && c2u ) {
            writeChars(true, tolower(c2), newstr);
            return;
        }
        if (c1u && c2u) {
            writeChars(false, tolower(c2), newstr);
            return;
        }
        if ( (!c1u && !c2u) ||
                (c1u && !c2u) ) {
            writeChars(false, c2, newstr);
            return;
        }
        return;
    }
    // Check triples having no zeros.
    if ( (!c1u && !c2u && !c3u) ||
            (c1u && !c2u && !c3u) ||
            (!c1u && !c2u && c3u) ||
            (c1u && !c2u && c3u) ) {
            writeChars(false, c2, newstr);
            return;
    }
    if ( (!c1u && c2u && !c3u) ||
            (c1u && c2u && !c3u) ||
            (!c1u && c2u && c3u) ) {
        writeChars(true, tolower(c2), newstr);
        return;
    }
    if ( c1u && c2u && c3u ) {
        writeChars(false, tolower(c2), newstr);
        return;
    }
    // All cases should have been checked by this point.
}

/**
 * \brief Parses camel case into lowercase words separated by underscores.
 *
 * A sequence of uppercase letters is interpreted as a word, except that
 * the last uppercase letter of a sequence is considered the start of a
 * new word if it is followed by a lowercase letter.
 */
void decodeCamelCase(const char* str, string* newstr)
{
    newstr->clear();
    // c1, c2, and c3 are a sliding window of char triples.
    char c1 = '\0';
    char c2 = '\0';
    char c3 = '\0';
    const char* p = str;
    while (*p != '\0') {
        c1 = c2;
        c2 = c3;
        c3 = *p;
        decodeTriple(c1, c2, c3, newstr);
        p++;
    }
    // Decode last character.
    c1 = c2;
    c2 = c3;
    c3 = '\0';
    decodeTriple(c1, c2, c3, newstr);
}

