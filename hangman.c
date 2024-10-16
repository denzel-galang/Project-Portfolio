/*
* Coded by Denzel Galang
* 
* This program is an implementation of hangman. The user chooses a 
* difficulty level, then a random word based on that difficulty level
* is chosen from a text file. The hangman sprites to show the number
* of guesses remaining are also loaded in by being read from a text
* file.
* 
* File I/O is primarily being used in this program. The word bank of
* words to choose from is stored in a file called allWords.txt while
* an array of text file names ("attempt1.txt", "attempt2.txt", and so
* on) is used to load in the hangman art. Arrays are also used to store
* used letters, correct letters, and the current guess of the player,
* so the game logic heavily relies on keeping track of which letters
* have already been correctly played to allow the player to progress.
*/

#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define NUM_WORDS 356
#define NUM_ALPHABET 26
#define MAX_WORD_LENGTH 1000

char* getWord(int difficultyLevel);
void stringToUpper(char* str);
void gameLoop(char* correctWord, int difficultyLevel, int* winCondition);
void printDifficulty(int difficultyLevel);
void printUsedLetters(char* usedLetters);
void displayGameState(int* numGuesses, int difficultyLevel,
    char* correctWord, char* usedLetters,
    int* lifelinesRemaining, int* winCondition, char* correctLetters);
int printBlanks(char* word, char* usedLetters);
int getNumLifelines(int difficultyLevel);
int checkCurrentChar(char letter, char* word);
int checkGuessLetter(char guess, char* correctStr, char* usedLetters);

int main(void) {
    int winCondition = 0;
    int difficultyInput = 0;
    char confirmDifficulty = ' ';
    printf("Welcome to hangman!\n");
    printf("Choose a difficulty to see its info:\n");
    printf("\t1) Easy\t\t 3) Hard \n");
    printf("\t2) Medium\t 4) Expert\n");

    // user selects then confirms difficulty level
    while (confirmDifficulty == ' ') {
        while (difficultyInput == 0) {
            printf("Enter 1, 2, 3, or 4: ");
            while (scanf("%d", &difficultyInput) != 1) {
                printf("Invalid input. Please enter 1, 2, 3, or 4: ");
                while (getchar() != '\n'); // clear input buffer
            }
            printf("\n");

            switch (difficultyInput) {
            case 1:
                printf("You selected EASY: Perfect for beginners. Short words and all lifelines are available.\n");
                break;
            case 2:
                printf("You selected MEDIUM: For those more experienced. Decently sized words and phrases will be used. All lifelines are available.\n");
                break;
            case 3:
                printf("You selected HARD: A challenge. Longer phrases will be used, and you only have one lifeline to choose from. Use it wisely!\n");
                break;
            case 4:
                printf("You selected EXPERT: The hardest. Complex sentences will be used, and no lifelines are offered. Good luck with this one!\n");
                break;
            default:
                printf("Please enter a number from 1-4.\n");
                difficultyInput = 0;
            }
        }

        printf("Confirm difficulty? (Y/N): ");
        if (scanf(" %c", &confirmDifficulty) != 1) {
            printf("Confirmation couldn't be read.\n");
            difficultyInput = 0;
            confirmDifficulty = ' ';
        }
        if (confirmDifficulty == 'Y' || confirmDifficulty == 'y') {
            break;
        }
        else {
            difficultyInput = 0;
            confirmDifficulty = ' ';
        }
    }
    system("cls");

    char* gameWord = getWord(difficultyInput); // getting a random word from the word bank
    gameLoop(gameWord, difficultyInput, &winCondition); // enter the game loop to begin the game

    if (winCondition) {
        printf("Congrats! You guessed the word!\n");
    }
    else {
        printf("You lost!\n");
    }

    free(gameWord);
    return 0;
}

char* getWord(int difficultyLevel) {
    srand(time(NULL));
    int wordIndex;

    // choose a random word depending the difficulty level
        // the difficulty levels are separated by how far down they are in the text file.
        // so for instance, lines 1-62 each contain a word under the easy category.
        // then, lines 64-111 each contain a word for the medium category, and so on.
    switch (difficultyLevel) {
    case 1: // easy words
        wordIndex = (rand() % 63) + 1;
        break;
    case 2: // medium words
        wordIndex = (rand() % (111 - 64 + 1)) + 64;
        break;
    case 3: // hard words
        wordIndex = (rand() % (326 - 112 + 1)) + 112;
        break;
    case 4: // expert words
        wordIndex = (rand() % (356 - 327 + 1)) + 327;
        break;
    }

    FILE* allWords = fopen("allWords.txt", "r");
    if (allWords == NULL) {
        fprintf(stderr, "An error occurred trying to open allWords.txt\n");
        return NULL;
    }
    fseek(allWords, 0, SEEK_SET); // move the file pointer to the beginning of the file

    // moving to the randomly chosen word's line number in the text file
    int currentLine = 0;
    while (currentLine < wordIndex) {
        if (fgetc(allWords) == '\n') {
            currentLine++;
        }
    }

    // using fscanf to copy the word from the text file to a char array variable
    char chosenWord[MAX_WORD_LENGTH];
    if (fscanf(allWords, "%[^\n]", chosenWord) != 1) {
        fprintf(stderr, "Error reading word from file\n");
        fclose(allWords);
        return NULL;
    }
    chosenWord[sizeof(chosenWord) - 1] = '\0'; // null terminating the string

    int wordSize = strlen(chosenWord);

    // allocating the appropriate amount of memory for the word
    char* finalWord = malloc(sizeof(char) * (wordSize + 1));
    if (finalWord == NULL) {
        fprintf(stderr, "Memory allocation failed!\n");
        fclose(allWords);
        return NULL;
    }
    strcpy(finalWord, chosenWord);

    fclose(allWords);
    return finalWord;
}

void printDifficulty(int difficultyLevel) {
    switch (difficultyLevel) {
    case 1:
        printf("You are now playing EASY mode |\n");
        printf("------------------------------+\n");
        break;
    case 2:
        printf("You are now playing MEDIUM mode |\n");
        printf("--------------------------------+\n");
        break;
    case 3:
        printf("You are now playing HARD mode |\n");
        printf("------------------------------+\n");
        break;
    case 4:
        printf("You are now playing EXPERT mode |\n");
        printf("--------------------------------+\n");
    }
    printf("\n");
}

void printHangman(int numGuesses) {
    const char* spriteFileArr[] = {
        "attempt1.txt",
        "attempt2.txt",
        "attempt3.txt",
        "attempt4.txt",
        "attempt5.txt",
        "attempt6.txt",
        "attempt7.txt",
        "attempt8.txt"
    };
    const char* spriteFile = spriteFileArr[numGuesses]; // loading in the correct file to use based on the attempts used so far

    FILE* hangman = fopen(spriteFile, "r");
    if (hangman == NULL) {
        fprintf(stderr, "An error occurred trying to print the hangman from %s.\n", spriteFile);
        return;
    }

    // printing the appropriate hangman sprite based on the current attempt number
    char ch;
    while ((ch = fgetc(hangman)) != EOF) {
        putchar(ch);
    }
    printf("\n");

    fclose(hangman);
}

int endGame(int winCondition, int numGuesses) {
    if (winCondition || numGuesses == 7) {
        return 1;
    }
    else {
        return 0;
    }
}

void displayGameState(int* numGuesses, int difficultyLevel,
    char* correctWord, char* usedLetters,
    int* lifelinesRemaining, int* winCondition, char* correctLetters) {

    printDifficulty(difficultyLevel);
    printHangman(*numGuesses);
    int numBlanks = printBlanks(correctWord, correctLetters) - 1;
    printUsedLetters(usedLetters);

    int choice = 0;
    printf("Choose an option:\n");
    printf("\t1) Guess a letter\n");
    printf("\t2) Guess the word/phrase\n");
    printf("\t3) Use a lifeline\n");
    printf("Enter your choice: ");
    if (scanf("%d", &choice) != 1) {
        printf("Invalid input. Please enter 1, 2, or 3: ");
        while (getchar() != '\n');
        return;
    }
    printf("\n");

    char letterGuess;
    char guessStr[1000];

    switch (choice) {
    case 1:
        printf("What letter would you like to guess? ");
        if (scanf(" %c", &letterGuess) != 1) {
            printf("Invalid input. Please enter a letter: ");
            while (getchar() != '\n');
            return;
        }

        letterGuess = toupper(letterGuess); // disables case-sensitivity

        for (int i = 0; i < NUM_ALPHABET; i++) {

            // check if the letter is already not a part of usedLetters
            if (letterGuess == usedLetters[i]) {
                printf("You already guessed that letter!\n");
                return;
            }
        }

        if (checkGuessLetter(letterGuess, correctWord, usedLetters, correctLetters)) {
            int index = 0;
            char* tempPtr = correctLetters;

            // move to the end of the correctLetters array
            while (*tempPtr != ' ') {
                tempPtr++;
                index++;
            }
            correctLetters[index] = letterGuess; // add the guessed letter to the array of correct letters

            // player automatically wins if there are no more blank letters to guess
            if (numBlanks == 0) {
                *winCondition = 1;
            }

            printf("Correct!\n");
        }
        else {
            printf("Wrong!\n");
            (*numGuesses)++;
        }
        break;
    case 2:
        printf("Enter the word/phrase: ");
        if (scanf(" %[^\n]", guessStr) != 1) {
            printf("Invalid input!");
            while (getchar() != '\n');
            return;
        }

        stringToUpper(guessStr);

        // compare the guessed word with the actual word
        if (strcmp(guessStr, correctWord) == 0) {
            *winCondition = 1; // player automatically wins if they match
        }
        else {
            printf("Wrong!\n");
            (*numGuesses)++;
            return;
        }
        break;
    case 3:
        if ((*lifelinesRemaining) > 0) {
            if (*lifelinesRemaining == 1) {
                printf("You have 1 lifeline remaining!\n");
            }
            else {
                printf("You have %d lifelines remaining!\n", (*lifelinesRemaining));
            }

            // user inputs a lifeline to use thru a menu
            int lifelineInput = 0;
            printf("What lifeline would you like to use? ");
            printf("Choose a lifeline:\n");
            printf("\t1) Reveal a letter\n");
            printf("\t2) Have another guess\n");
            printf("Enter an option: ");
            if (scanf("%d", &lifelineInput) != 1) {
                printf("Not a valid input!\n");
                while (getchar() != '\n');
                return;
            }

            switch (lifelineInput) {
            case 1: // revealing a letter

                // find a correct letter not already in the array of correct letters
                for (int i = 0; i < strlen(correctWord) + 1; i++) {
                    if (!existsInArr(correctWord, correctLetters[i])) {
                        int index = 0;
                        char* tempPtr = correctLetters;
                        while (*tempPtr != ' ') {
                            tempPtr++;
                            index++;
                        }
                        correctLetters[index] = correctWord[i]; // add the correct letter to the array of correct letters
                        break;
                    }
                }
                break;
            case 2: // having another guess (simply just decrement the number of guesses used)
                if (*numGuesses > 0) {
                    printf("The hangman lost a body part! You now have another guess.\n");
                    (*numGuesses)--;
                }
                else {
                    printf("You need to have at least one incorrect guess first.\n");
                    return;
                }
                break;
            }

            (*lifelinesRemaining)--; // decrement the lifelines as one has been just used
        }
        else {
            if (difficultyLevel == 4) {
                printf("There are no lifelines in expert difficulty!\n");
            }
            else {
                printf("You have no lifelines remaining!\n");
            }
        }
        break;
    default:
        printf("Please enter 1, 2, or 3.\n");
        break;
    }
}

// iterate thru the string char by char to see if the char exists within it
int existsInArr(char* arr, char value) {
    for (int i = 0; i < strlen(arr) + 1; i++) {
        if (arr[i] == value) {
            return 1;
        }
    }
    return 0;
}

// converts the user's guess to uppercase to not be case-sensitive input
void stringToUpper(char* str) {
    while (*str) {
        *str = toupper((unsigned char)*str);
        str++;
    }
}

int getNumLifelines(int difficultyLevel) {
    switch (difficultyLevel) {
    case 1:
        return 3;
    case 2:
        return 2;
    case 3:
        return 1;
    case 4:
        return 0;
    }
}

// checks if the usedLetter is ALSO part of the word
int checkCurrentChar(char letter, char* word) {
    for (int i = 0; i < strlen(word) + 1; i++) {
        if (letter == word[i]) {
            return 1;
        }
    }
    return 0;
}

// will either print an underscore (to represent a blank letter) or the correctly guessed letters so far
int printBlanks(char* word, char* correctLetters) {
    int numBlanks = 0;
    printf("   ");

    // traverse the entire word or phrase char by char
    while (*word != '\0') {
        if (*word != ' ') {

            // if the current letter of the word is also a correctly guessed letter, print the letter
            if (checkCurrentChar(*word, correctLetters)) {
                printf("%c ", *word);
            }
            else { // otherwise, print it as an underscore
                printf("_ ");
                numBlanks++;
            }
        }
        else {
            printf("  ");
        }
        word++;
    }
    printf("\n\n");
    return numBlanks;
}

void printUsedLetters(char* usedLetters) {
    printf("\n");
    printf("Letters already used: ");
    for (int i = 0; i < NUM_ALPHABET; i++) {
        printf("%c ", usedLetters[i]);
    }
    printf("\n");
}

int checkGuessLetter(char guess, char* correctStr, char* usedLetters) {
    for (int i = 0; i < strlen(correctStr) + 1; i++) {
        if (guess == correctStr[i]) {
            return 1;
        }
    }

    // add the incorrectly guessed letter to the usedLetters array
    int i = 0;
    while (usedLetters[i] != ' ') {
        i++;
    }
    usedLetters[i] = guess;
    return 0;
}

void gameLoop(char* correctWord, int difficultyLevel, int* winCondition) {
    int numGuesses = 0;
    int lifelinesRemaining = getNumLifelines(difficultyLevel);
    char usedLetters[NUM_ALPHABET], correctLetters[NUM_ALPHABET];

    for (int i = 0; i < NUM_ALPHABET; i++) {
        usedLetters[i] = ' ';
        correctLetters[i] = ' ';
    }

    while (1) {
        displayGameState(&numGuesses, difficultyLevel, correctWord,
            usedLetters, &lifelinesRemaining, winCondition, 
            correctLetters);
        Sleep(1000);

        // check if the game has met either the winning or losing condition to end the game
        if (endGame(*winCondition, numGuesses)) {
            break;
        }
        system("cls");
    }

    // reveal the correct word if the game has been lost
    if (!(*winCondition)) {
        system("cls");
        printDifficulty(difficultyLevel);
        printHangman(7);
        printf("\n\nThe word was %s\n", correctWord);
    }
}