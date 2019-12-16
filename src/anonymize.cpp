#include <cstring>

#include "anonymize.h"

bool possiblePersonalData(const string& field)
{
    const char* f = field.c_str();
    return (
            (strcmp(f, "/id") != 0) &&
            (strcmp(f, "/active") != 0) &&
            (strcmp(f, "/type") != 0) &&
            (strcmp(f, "/patronGroup") != 0) &&
            (strcmp(f, "/enrollmentDate") != 0) &&
            (strcmp(f, "/expirationDate") != 0) &&
            (strcmp(f, "/meta") != 0) &&
            (strcmp(f, "/proxyFor") != 0) &&
            (strcmp(f, "/createdDate") != 0) &&
            (strcmp(f, "/updatedDate") != 0) &&
            (strcmp(f, "/metadata") != 0) &&
            (strcmp(f, "/tags") != 0)
           );
}


