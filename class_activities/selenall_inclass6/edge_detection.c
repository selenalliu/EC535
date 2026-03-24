/*  In-class exercise 6 - Ethan Liang (ethanl66) and Selena Liu (selenall)
Q4: If this function runs on an embedded Linux system without GPU acceleration, what are some ways you could speed it up without using extra libraries? 
    To speed up the edge detection function on an embedded Linux system without GPU acceleration, we could:
    * 1. Optimize memory access patterns to improve cache performance (implement blocking)
    * 2. Use loop unrolling (reduce loop overhead and increase instruction-level parallelism)
    * 3. Minimize branching by using conditional moves 
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define WIDTH 640
#define HEIGHT 480


void detectEdge(unsigned char image[HEIGHT][WIDTH], unsigned char output[HEIGHT][WIDTH], int width, int height) {
    // Simple edge detection algorithm
    for (int y = 0; y < height - 1; y++) {
        for (int x = 0; x < width - 1; x++) {
            if (x == 0 || y == 0 || x == width - 1 || y == height - 1) {
                output[y][x] = 0; // Set border pixels to 0
                continue;
            }
            int gx = abs(image[y][x+1] - image[y][x-1]);
            int gy = abs(image[y+1][x] - image[y-1][x]);
            int g = gx + gy;
            output[y][x] = g > 255 ? 255 : g;
        }
    }
}

int main() {
    FILE *fp = fopen("img.txt", "r");
    if (fp == NULL) {
        perror("Error opening file");
        return 1;
    }

    unsigned char image[HEIGHT][WIDTH];  // Stores grayscale image (0–255)
    char line[16000];                    // Buffer for each row
    unsigned char output[HEIGHT][WIDTH]; // Output image for edge detection

    for (int row = 0; row < HEIGHT; row++) {
        if (fgets(line, sizeof(line), fp) == NULL) {
            fprintf(stderr, "Unexpected end of file at row %d\n", row);
            fclose(fp);
            return 1;
        }

        char *token = strtok(line, " ");
        for (int col = 0; col < WIDTH; col++) {
            if (token == NULL) {
                fprintf(stderr, "Missing data at row %d, column %d\n", row, col);
                fclose(fp);
                return 1;
            }
            image[row][col] = (unsigned char)atoi(token);
            token = strtok(NULL, " ");
        }
    }

    fclose(fp);

    // Example: Print a small section to verify
    printf("Top-left 3x3 block:\n");
    for (int y = 0; y < 3; y++) {
        for (int x = 0; x < 3; x++) {
            printf("%3d ", image[y][x]);
        }
        printf("\n");
    }

    printf("\n");
    detectEdge(image, output, WIDTH, HEIGHT);
    // for (int y = 0; y < HEIGHT; y++) {
    //     for (int x = 0; x < WIDTH; x++) {
    //         printf("%3d ", output[y][x]);
    //     }
    //     printf("\n");
    // }

    return 0;
}
