/*
* Coded by Denzel Galang
* 
* This program uses a PNG image as input to create a text file that
* replicates the level layout designed in the PNG. This text file
* will then be outputted to the game files as it will be read in
* to be a playable level.
* 
* All PNG images inputted are 23x23 pixels to replicate the game board.
* The PNG image was designed in a pixel art website, so by using a palette
* of predetermined RGB colors, I could design my own levels as pixel art 
* in the website, output that art as a 23x23 PNG, then have this program 
* analyze each pixel to map each RGB value to a specific game entity at 
* that location. For instance, a black pixel means a wall, and a red pixel
* means the player's starting location.
*/

#define _CRT_SECURE_NO_WARNINGS
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <stdint.h>

#define GRID_SIZE 23 // dimensions of the game board

typedef struct {
    int r, g, b;
} Pixel;

typedef struct {
    Pixel color;
    char gridMarker;
} ColorMap;

void imageToGrid(const char* imageName, char grid[GRID_SIZE][GRID_SIZE]);
void gridToTxt(char* fileName, char grid[GRID_SIZE][GRID_SIZE]);
char determineChar(Pixel pixel);
int matchesColor(Pixel x, Pixel y);

// all entity type markers and their corresponding RGB values
const ColorMap allMarkers[] = {
    {{0, 0, 0}, '#'},
    {{255, 255, 255}, ' '},
    {{255, 0, 0}, 'X'},
    {{0, 255, 0}, 'E'},
    {{0, 0, 255}, 'B'},
    {{0, 255, 255}, 'P'},
    {{255, 0, 255}, 't'},
    {{255, 255, 0}, 'C'},
    {{255, 128, 0}, 'T'},
    {{128, 255, 0}, 's'},
    {{128, 0, 255}, 'M'},
    {{128, 128, 128}, 'W'},
    {{128, 128, 0}, 'S'},
    {{91, 36, 1}, '!'},
    {{222, 169, 135}, '?'},
    {{6, 85, 53}, '?'},
    {{0, 51, 102}, '?'}
};

// defining the output file directory (which is in the main game's folder)
char* outputDirectory = "C:/Users/altav/source/repos/Project39/Project39/";

int main(int argc, char* argv[]) {
    char grid[GRID_SIZE][GRID_SIZE];
    char imageName[20];
    char fileName[20];

    // copy the command line arguments to the image and file name variables
    strcpy(imageName, argv[1]);
    strcpy(fileName, argv[2]);

    imageToGrid(imageName, grid);
    gridToTxt(fileName, grid);

    return 0;
}

void gridToTxt(char* fileName, char grid[GRID_SIZE][GRID_SIZE]) {
    char filePath[100];

    // concatenate the file name to the defined file path to prepare being opened
    strcpy(filePath, outputDirectory);
    strcat(filePath, fileName);

    // opening a new text file to write into
    FILE* newFile = fopen(filePath, "w");
    if (newFile == NULL) {
        fprintf(stderr, "ERROR: Could not export image to .txt file\n");
        return;
    }

    // copying the grid data to the text file
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            fputc(grid[i][j], newFile);
        }
        fputc('\n', newFile);
    }

    fclose(newFile);
}

int matchesColor(Pixel x, Pixel y) {
    return x.r == y.r && x.b == y.b && x.g == y.g; // see if all RGB values are the same between 2 pixels
}

char determineChar(Pixel pixel) {
    int numMarkers = sizeof(allMarkers) / sizeof(allMarkers[0]);

    // mapping each pixel's color from the image to its corresponding entity type
    for (int i = 0; i < numMarkers; i++) {
        if (matchesColor(pixel, allMarkers[i].color)) {
            return allMarkers[i].gridMarker;
        }
    }
    return '?'; // return an unknown marker if no matches were found
}

void imageToGrid(const char* imageName, char grid[GRID_SIZE][GRID_SIZE]) {
    int width, height, bpp;
    uint8_t* rgbImage = stbi_load(imageName, &width, &height, &bpp, 3); // stbi_load returns RGB values for each pixel on the image

    if (rgbImage == NULL) {
        printf("ERROR: The image could not be loaded.\n");
        return;
    }
    else if (width != GRID_SIZE || height != GRID_SIZE) {
        printf("ERROR: Image has incorrect dimensions. width=%d, height=%d\n", width, height);
        stbi_image_free(rgbImage);
        return;
    }

    // traverse thru the image pixel by pixel
    int rowCounter = 0;
    for (int i = 0; i < GRID_SIZE * GRID_SIZE * 3; i += 3) {
        
        // translate each pixel to a char by matching the pixel's RGB values to its corresponding entity type
        int entityChar = determineChar((Pixel) { rgbImage[i], rgbImage[i + 1], rgbImage[i + 2] });

        // handle any unknown colors in the image
        if (entityChar == '?') {
            fprintf(stderr, "ERROR: Unknown color located at row=%d column=%d", rowCounter, (i / 3) % GRID_SIZE);
            break;
        }

        grid[rowCounter][(i / 3) % GRID_SIZE] = entityChar; // set the matching character onto the grid

        // move to the next row if the end of the current row is reached
        if ((i / 3) % GRID_SIZE == GRID_SIZE - 1) {
            rowCounter++;
        }
    }
    stbi_image_free(rgbImage); // close the image when finished
}