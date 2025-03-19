
#include "wwf.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "utils.h"

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
int wordListLength;
struct WordListNode* masterWord = NULL;
struct GameListNode* gameList = NULL;

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
    gameList = NULL;
    masterWord = NULL;
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
bool compareCounts(const int candidate[], const int choices[])
{
    for (int i = 0; i < 26; i++)
        if (candidate[i] > choices[i])
            return false;
    return true;
}

bool isDone()
{
    struct GameListNode *node = gameList;
    while (node != NULL)
    {
        if (!node->isFound)
            return false;
        node = node->next;
    }
    return true;
}

void displayWorld()
{
    char word[31];
    strcpy(word, masterWord->word);
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

void displayWorldHTML(char *html, size_t size)
{
    if (masterWord == NULL)
    {
        strncpy(html,
            "<!DOCTYPE html>\n"
            "<html>\n"
            "  <body>\n"
            "    <h1>Words Without Friends</h1>\n"
            "    <p>No game is in progress. <a href=\"/words\">Start a new one</a>?</p>\n"
            "  </body>\n"
            "</html>",
            size - 1);
        return;
    }
    if (isDone())
    {
        strncpy(html,
            "<!DOCTYPE html>\n"
            "<html>\n"
            "  <body>\n"
            "    <h1>Words Without Friends</h1>\n"
            "    <p>Congratulations! You found all the words! <a href=\"/words\">Play again</a>?</p>\n"
            "  </body>\n"
            "</html>", size - 1);
        return;
    }
    strncpy(html,
        "<!DOCTYPE html>\n"
        "<html>\n"
        "  <body>\n"
        "    <h1>Words Without Friends</h1>\n", size - 1);
    char word[31];
    strcpy(word, masterWord->word);
    strsort(word);
    strncat(html, "    <p>", size - strlen(html) - 1);
    char buf[3];
    sprintf(buf, "%c", word[0]);
    strncat(html, buf, size - strlen(html) - 1);
    int length = strlen(word);
    for (int i = 1; i < length; i++)
    {
        sprintf(buf, " %c", word[i]);
        strncat(html, buf, size - strlen(html) - 1);
    }
    strncat(html, "</p>\n    <p>", size - strlen(html) - 1);

    struct GameListNode *node = gameList;
    while (node != NULL)
    {
        if (node->isFound)
            strncat(html, node->word, size - strlen(html) - 1);
        else
        {
            length = strlen(node->word);
            strncat(html, "_", size - strlen(html) - 1);
            for (int i = 1; i < length; i++)
                strncat(html, " _", size - strlen(html) - 1);
        }
        strncat(html, "<br>", size - strlen(html) - 1);
        node = node->next;
    }
    strncat(html, "</p>\n", size - strlen(html) - 1);
    strncat(html,
        "    <form action=\"/words\" method=\"get\">\n"
        "      <input type=\"text\" name=\"guess\">\n"
        "      <input type=\"submit\" value=\"Submit\">"
        "    </form>\n"
        "  </body>\n"
        "</html>", size - strlen(html) - 1);
}

void acceptInput_stdin()
{
    printf("Enter a guess: ");
    char buffer[32];
    fgets(buffer, sizeof(buffer), stdin);
    strupper(buffer);
    strreplace(buffer, '\n', '\0');
    strreplace(buffer, '\r', '\0');
    acceptInput(buffer);
}

void acceptInput(const char *guess)
{
    struct GameListNode *node = gameList;
    if (CHEAT_MODE && strlen(guess) == 0)
        while (node != NULL)
        {
            if (node->isFound == false)
            {
                node->isFound = true;
                break;
            }
            node = node->next;
        }
    else
        while (node != NULL)
        {
            if (strcmp(guess, node->word) == 0)
                node->isFound = true;
            node = node->next;
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
            if (CHEAT_MODE)
                printf("%s\n", gameNode->word);
        }
        wordNode = wordNode->next;
        free(candidate);
    }
    free(choices);
}

void gameNew()
{
    cleanupGameListNodes();
    masterWord = getRandomWord(wordListLength);
    findWords(masterWord);
}

int initialize()
{
    srand(time(NULL));

    FILE* fp = fopen("words_without_friends/2of12.txt", "r");
    if (fp == NULL)
        return 1;
    int length = 0;
    char buffer[100];
    while (fgets(buffer, sizeof buffer, fp) != NULL)
    {
        strupper(buffer);
        strreplace(buffer, '\n', '\0');
        strreplace(buffer, '\r', '\0');
        struct WordListNode* node = (struct WordListNode *)malloc(sizeof(struct WordListNode));
        if (node == NULL)
            return 1;
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
    wordListLength = length;
    return 0;
}

int teardown()
{
    cleanupGameListNodes();
    cleanupWordListNodes();
    printf("All done.\n");
    return 0;
}

// void gameLoop()
// {
//     while (!isDone())
//     {
//         displayWorld();
//         acceptInput();
//     }
//     displayWorld();
// }

// int main(int argc, char **argv)
// {
//     if (argc > 1 && strcmp(argv[1], "-c") == 0)
//         CHEAT_MODE = true;
//     int length = initialization();
//     masterWord = getRandomWord(length);
//     findWords(masterWord);
//     gameLoop();
//     teardown();
//     return 0;
// }
