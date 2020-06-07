#include <string.h>
#include <ctype.h>
#include "bool.h"
#include "utilities.h"


bool is_positive_number(char *s)
{
    if (s[0] == '-' || s[0] == '0')
    {
        return false;
    }

    int i;
    for (i = 0; i < strlen(s); i++)
    {
        if (!isdigit(s[i]))
            return false;
    }
    return true;
}