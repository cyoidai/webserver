
#include "utils.h"
#include <string.h>

void strreplace(char *str, char match, char replace)
{
    int length = strlen(str);
    for (int i = 0; i < length; i++)
        if (str[i] == match)
            str[i] = replace;
}

void strupper(char *str)
{
    for (int i = 0; str[i] != '\0'; i++)
        if (str[i] >= 'a' && str[i] <= 'z')
            str[i] -= 32;
}

void strsort(char *arr)
{
    // selection sort
    int length = strlen(arr);
    for (int i = 0; i < length - 1; i++)
    {
        int min = i;
        for (int j = i + 1; j < length; j++)
            if (arr[j] < arr[min])
                min = j;
        if (min != i)
        {
            char tmp = arr[i];
            arr[i] = arr[min];
            arr[min] = tmp;
        }
    }
}
