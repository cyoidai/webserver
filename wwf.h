
#include <stddef.h>

/**
 * @brief Initialize words without friends. Only needs to be called once at
 * program start. Reads the dictionary file and adds words to the word list.
 * @return 0 on success, 1 on failure.
 */
int initialize();

/**
 * @brief Starts a new words without friends game. Will cleanup if there's
 * already an existing game in-progress.
 */
void gameNew();

/**
 * @brief Creates an HTML-formatted string representation of the current game.
 * @param html Pointer to put the string
 * @param size size of the string
 */
void displayWorldHTML(char *html, size_t size);

void displayWorld();

void acceptInput(const char *guess);
