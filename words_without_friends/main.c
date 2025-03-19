
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

bool CHEAT_MODE = false;

struct WordListNode
{
    struct WordListNode* next;
    char word[31];
};

struct GameListNode
{
    struct GameListNode* next;
    char word[31];
    bool isFound;
};

struct WordListNode* wordList;
struct GameListNode* gameList;

void cleanupWordListNodes()
{
    struct WordListNode *current = wordList;
    struct WordListNode *next;
    while (current != NULL)
    {
        next = current->next;
        free(current);
        current = next;
    }
}

void cleanupGameListNodes()
{
    struct GameListNode *current = gameList;
    struct GameListNode *next;
    while (current != NULL)
    {
        next = current->next;
        free(current);
        current = next;
    }
}

void strreplace(char str[], char match, char replace)
{
    int length = strlen(str);
    for (int i = 0; i < length; i++)
        if (str[i] == match)
            str[i] = replace;
}

void strupper(char str[])
{
    for (int i = 0; str[i] != '\0'; i++)
        if (str[i] >= 'a' && str[i] <= 'z')
            str[i] -= 32;
}

/**
 * @brief Returns the number of occurrences of each letter in a string.
 * @param str The given string.
 * @return Array of length 26 where index 0 is the number of A/a and index 25 is
 *         the number of Z/z within `s`.
 */
int* getLetterDistribution(const char str[])
{
    int* distr = (int *)calloc(26, sizeof(int));
    for (int i = 0; str[i] != '\0'; i++)
    {
        if (str[i] >= 'A' && str[i] <= 'Z')
            distr[str[i] - 65]++;
        else if (str[i] >= 'a' && str[i] <= 'z')
            distr[str[i] - 97]++;
    }
    return distr;
}

/**
 * @brief Compares two letter distributions depending on whether `candidate` can
 *        be made using `choices`.
 * @param candidate letter distribution being compared.
 * @param choices   letter distribution `candidate` is being compared to.
 * @return `true` if `candidate` can be made using `choices`, `false` if
 *         `candidate` cannot be made using `choices`.
 */
_Bool compareCounts(const int candidate[], const int choices[])
{
    for (int i = 0; i < 26; i++)
        if (candidate[i] > choices[i])
            return false;
    return true;
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

void displayWorld(struct WordListNode* master)
{
    char word[31];
    strcpy(word, master->word);
    strsort(word);
    int length = strlen(word);
    for (int i = 0; i < length; i++)
        printf("%c ", word[i]);
    printf("\n");

    printf("-----------------------------\n");

    struct GameListNode* node = gameList;
    while (node != NULL)
    {
        if (node->isFound)
            printf("FOUND: %s\n", node->word);
        else
        {
            length = strlen(node->word);
            for (int i = 0; i < length; i++)
                printf("_ ");
            printf("\n");
        }
        node = node->next;
    }
}

_Bool isDone()
{
    struct GameListNode* node = gameList;
    while (node != NULL)
    {
        if (!node->isFound)
            return false;
        node = node->next;
    }
    return true;
}

void acceptInput()
{
    printf("Enter a guess: ");
    char buffer[32];
    fgets(buffer, sizeof(buffer), stdin);
    strupper(buffer);
    strreplace(buffer, '\n', '\0');
    strreplace(buffer, '\r', '\0');

    struct GameListNode* node = gameList;
    if (CHEAT_MODE)
    {
        while (node != NULL)
        {
            if (node->isFound == false)
            {
                node->isFound = true;
                break;
            }
            node = node->next;
        }
    }
    else
    {
        while (node != NULL)
        {
            if (strcmp(buffer, node->word) == 0)
            {
                node->isFound = true;
            }
            node = node->next;
        }
    }
}

/**
 * @brief Picks a random word from `wordList`. The "random" word must be more
 *        than 6 characters long.
 * @param length length of `wordList`
 * @return the random word's node.
 */
struct WordListNode* getRandomWord(int length)
{
    int pick = rand() % (length + 1);
    struct WordListNode* n = wordList;
    for (int i = 0; i < pick; i++)
        n = n->next;

    while (strlen(n->word) <= 6)
    {
        n = n->next;
        if (n == NULL)
            n = wordList;
    }
    return n;
}

/**
 * @brief Finds words that can be made up using `choicesNode` and adds them to
 * `gameList`.
 * @param choicesNode chosen word's node.
 */
void findWords(const struct WordListNode* choicesNode)
{
    int* choices = getLetterDistribution(choicesNode->word);
    struct WordListNode* wordNode = wordList;
    while (wordNode != NULL)
    {
        int *candidate = getLetterDistribution(wordNode->word);
        if (compareCounts(candidate, choices))
        {
            struct GameListNode* gameNode = (struct GameListNode *)malloc(sizeof(struct GameListNode));
            memcpy(gameNode->word, wordNode->word, strlen(wordNode->word) + 1);
            gameNode->next = NULL;
            gameNode->isFound = false;
            if (gameList == NULL)
                gameList = gameNode;
            else
            {
                struct GameListNode* n = gameList;
                while (n->next != NULL)
                    n = n->next;
                n->next = gameNode;
            }
        }
        wordNode = wordNode->next;
        free(candidate);
    }
    free(choices);
}

/**
 * @brief Reads `2of2.txt` and adds every line to `wordList`.
 * @return length of `wordList`.
 */
int initialization()
{
    srand(time(NULL));

    FILE* fp = fopen("2of12.txt", "r");
    if (fp == NULL)
    {
        printf("Failed to open file\n");
        return 0;
    }
    int length = 0;
    char buffer[100];
    while (fgets(buffer, sizeof buffer, fp) != NULL)
    {
        strupper(buffer);
        strreplace(buffer, '\n', '\0');
        strreplace(buffer, '\r', '\0');

        struct WordListNode* node = (struct WordListNode *)malloc(sizeof(struct WordListNode));
        memcpy(node->word, buffer, strlen(buffer) + 1);
        node->next = NULL;
        if (wordList == NULL)
            wordList = node;
        else
        {
            struct WordListNode* n = wordList;
            while (n->next != NULL)
                n = n->next;
            n->next = node;
        }
        length++;
    }
    fclose(fp);
    return length;
}

int teardown()
{
    cleanupGameListNodes();
    cleanupWordListNodes();
    printf("All done.\n");
    return 0;
}

void gameLoop(struct WordListNode *word)
{
    while (!isDone())
    {
        displayWorld(word);
        acceptInput();
    }
    displayWorld(word);
}

int main(int argc, char **argv)
{
    if (argc > 1 && strcmp(argv[1], "-c") == 0)
        CHEAT_MODE = true;
    int length = initialization();
    struct WordListNode *word = getRandomWord(length);
    findWords(word);
    gameLoop(word);
    teardown();
    return 0;
}
