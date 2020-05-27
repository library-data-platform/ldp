#include "camelcase.h"

/**
 * \brief Appends an underscore (optionally) and a specified character
 * to a string.
 *
 * \param[in] underscore If true, specifies that an underscore should
 * be appended.
 * \param[in] ch The character to be appended.
 * \param[out] str The string that will be appended to.
 */
static void append_underscore_char(bool underscore, char ch, string* str)
{
    if (underscore)
        *str += '_';
    *str += ch;
}

/**
 * \brief Examines a sequence of three characters, decodes the middle
 * character, and appends the decoded output to a string.
 *
 * \param[in] c1 The first character.
 * \param[in] c2 The second character, which will be decoded.
 * \param[in] c3 The third character.
 * \param[out] str The string that will be appended to.
 */
static void decode_triple(char c1, char c2, char c3, string* str)
{
    bool c1u = isupper(c1);
    bool c2u = isupper(c2);
    bool c3u = isupper(c3);
    if (c2 == '\0')
        return;
    if ( (c1 == '\0' && c2 != '\0' && c3 != '\0') ||
            (c1 == '\0' && c2 != '\0' && c3 == '\0') ) {
        append_underscore_char(false, tolower(c2), str);
        return;
    }
    if (c1 != '\0' && c2 != '\0' && c3 == '\0') {
        if ( !c1u && c2u ) {
            append_underscore_char(true, tolower(c2), str);
            return;
        }
        if (c1u && c2u) {
            append_underscore_char(false, tolower(c2), str);
            return;
        }
        if ( (!c1u && !c2u) ||
                (c1u && !c2u) ) {
            append_underscore_char(false, c2, str);
            return;
        }
        return;
    }
    // Check triples having no zeros.
    if ( (!c1u && !c2u && !c3u) ||
            (c1u && !c2u && !c3u) ||
            (!c1u && !c2u && c3u) ||
            (c1u && !c2u && c3u) ) {
            append_underscore_char(false, c2, str);
            return;
    }
    if ( (!c1u && c2u && !c3u) ||
            (c1u && c2u && !c3u) ||
            (!c1u && c2u && c3u) ) {
        append_underscore_char(true, tolower(c2), str);
        return;
    }
    if ( c1u && c2u && c3u ) {
        append_underscore_char(false, tolower(c2), str);
        return;
    }
    // All cases should have been checked by this point.
}

/**
 * \brief Decodes camel case into lowercase words separated by
 * underscores.
 *
 * A sequence of uppercase letters is interpreted as a word, except
 * that the last uppercase letter of a sequence is considered the
 * start of a new word if it is followed by a lowercase letter.
 *
 * \param[in] str The string to be decoded.
 * \param[out] decoded The string where the decoded text will be
 * written, replacing any existing text.
 */
void decode_camel_case(const char* str, string* decoded)
{
    decoded->clear();
    // c1, c2, and c3 are a sliding window of char triples.
    char c1 = '\0';
    char c2 = '\0';
    char c3 = '\0';
    const char* p = str;
    while (*p != '\0') {
        c1 = c2;
        c2 = c3;
        c3 = *p;
        decode_triple(c1, c2, c3, decoded);
        p++;
    }
    // Decode last character.
    c1 = c2;
    c2 = c3;
    c3 = '\0';
    decode_triple(c1, c2, c3, decoded);
}

