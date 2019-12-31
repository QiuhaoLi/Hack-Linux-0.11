#ifndef _STRING_FIX_H_
#define _STRING_FIX_H_

/*
 * Sometimes inline optimization is tricky and evil...
 */
int strCmp(const char* s1, const char* s2)
{
    while(*s1 && (*s1 == *s2))
    {
        s1++;
        s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

#endif
