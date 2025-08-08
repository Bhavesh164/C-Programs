#include <stdio.h>
#include <stdlib.h>
#include <ncurses.h>  // For terminal UI functions on Linux/Unix
#include <unistd.h>   // For usleep()
#include <time.h>     // For seeding the random number generator
#include <sys/time.h> // For gettimeofday() to create a responsive game loop
#include <stdbool.h>  // For bool type
#include <string.h>   // For strlen() to center text

// --- Game Configuration ---
#define WIDTH 40
#define HEIGHT 20
#define GAME_SPEED 100000 // microseconds (100000us = 100ms)

// --- Game State Variables ---
int gameOver;
int score;
int headX, headY;       // Snake head coordinates
int foodX, foodY;       // Food coordinates
int tailX[100], tailY[100]; // Arrays to store tail coordinates
int nTail;              // Current length of the tail
enum eDirection { STOP = 0, LEFT, RIGHT, UP, DOWN };
enum eDirection dir;

// --- Helper function to check if a coordinate is on the snake ---
bool isPositionOnSnake(int x, int y) {
    if (headX == x && headY == y) {
        return true;
    }
    for (int i = 0; i < nTail; i++) {
        if (tailX[i] == x && tailY[i] == y) {
            return true;
        }
    }
    return false;
}

// --- Function to place food at a valid random position ---
void PlaceFood() {
    int attempts = 0;
    do {
        // Generate coordinates within the playable area (1 to WIDTH, 1 to HEIGHT)
        foodX = (rand() % WIDTH) + 1;
        foodY = (rand() % HEIGHT) + 1;
        attempts++;
        // Prevent infinite loop in case the snake fills the entire screen
        if (attempts > WIDTH * HEIGHT) break;
    } while (isPositionOnSnake(foodX, foodY));
}

// --- Setup: Initializes the game state for a new game ---
void Setup() {
    srand(time(NULL)); // Seed the random number generator
    gameOver = 0;
    dir = STOP;
    headX = WIDTH / 2;
    headY = HEIGHT / 2;
    score = 0;
    nTail = 0;
    
    // Place initial food
    PlaceFood();
}

// --- DrawBoard: Draws the static elements (borders, instructions) once ---
void DrawBoard() {
    clear(); // Clear the entire screen once
    
    // Draw top and bottom borders
    for (int i = 0; i < WIDTH + 2; i++) {
        mvprintw(0, i, "#");
        mvprintw(HEIGHT + 1, i, "#");
    }

    // Draw side borders
    for (int i = 0; i < HEIGHT + 2; i++) {
        mvprintw(i, 0, "#");
        mvprintw(i, WIDTH + 1, "#");
    }
    
    // Instructions and score area
    mvprintw(HEIGHT + 3, 0, "Score: 0   ");
    mvprintw(HEIGHT + 4, 0, "Use WASD or Arrow keys. Press 'q' to quit.");
    refresh();
}

// --- Clear the entire game area (not including borders) ---
void ClearGameArea() {
    for (int i = 1; i <= HEIGHT; i++) {
        for (int j = 1; j <= WIDTH; j++) {
            mvprintw(i, j, " ");
        }
    }
    
    // Redraw borders to ensure they don't get accidentally overwritten
    // Top and bottom borders
    for (int i = 0; i < WIDTH + 2; i++) {
        mvprintw(0, i, "#");
        mvprintw(HEIGHT + 1, i, "#");
    }
    
    // Side borders
    for (int i = 0; i < HEIGHT + 2; i++) {
        mvprintw(i, 0, "#");
        mvprintw(i, WIDTH + 1, "#");
    }
}

// --- Draw: Renders the dynamic game elements (snake, food, score) ---
void Draw() {
    // Clear only the game area, not the borders
    ClearGameArea();
    
    // Draw the food first
    mvprintw(foodY, foodX, "F");
    
    // Draw the snake's tail with bounds checking
    for (int i = 0; i < nTail; i++) {
        // Ensure tail segments are within game boundaries
        if (tailX[i] >= 1 && tailX[i] <= WIDTH && tailY[i] >= 1 && tailY[i] <= HEIGHT) {
            mvprintw(tailY[i], tailX[i], "o");
        }
    }
    
    // Draw the snake's head (drawn last so it appears on top) with bounds checking
    if (headX >= 1 && headX <= WIDTH && headY >= 1 && headY <= HEIGHT) {
        mvprintw(headY, headX, "O");
    }

    // Update the score
    mvprintw(HEIGHT + 3, 0, "Score: %d   ", score);

    refresh(); // Refresh the screen to show changes
}

// --- Input: Handles user keyboard input during the game ---
void Input() {
    int ch;
    // Process all pending characters in the input buffer.
    while ((ch = getch()) != ERR) {
        switch (ch) {
            case 'a':
            case 'A':
            case KEY_LEFT:
                if (dir != RIGHT) dir = LEFT;
                break;
            case 'd':
            case 'D':
            case KEY_RIGHT:
                if (dir != LEFT) dir = RIGHT;
                break;
            case 'w':
            case 'W':
            case KEY_UP:
                if (dir != DOWN) dir = UP;
                break;
            case 's':
            case 'S':
            case KEY_DOWN:
                if (dir != UP) dir = DOWN;
                break;
            case 'q':
            case 'Q':
                gameOver = 1;
                break;
        }
    }
}

// --- Logic: Updates the game state based on rules ---
void Logic() {
    if (dir == STOP) return; // Don't move if not started

    // Check if we're about to eat food
    bool willEatFood = false;
    int newHeadX = headX;
    int newHeadY = headY;
    
    // Calculate new head position
    switch (dir) {
        case LEFT:  newHeadX--; break;
        case RIGHT: newHeadX++; break;
        case UP:    newHeadY--; break;
        case DOWN:  newHeadY++; break;
        default: break;
    }
    
    // Handle wall wrapping - ensure coordinates stay within game area (1 to WIDTH/HEIGHT)
    if (newHeadX > WIDTH) newHeadX = 1; 
    else if (newHeadX < 1) newHeadX = WIDTH;
    if (newHeadY > HEIGHT) newHeadY = 1; 
    else if (newHeadY < 1) newHeadY = HEIGHT;
    
    // Double-check bounds to prevent border corruption
    if (newHeadX < 1) newHeadX = 1;
    if (newHeadX > WIDTH) newHeadX = WIDTH;
    if (newHeadY < 1) newHeadY = 1;
    if (newHeadY > HEIGHT) newHeadY = HEIGHT;
    
    // Check if we will eat food
    willEatFood = (newHeadX == foodX && newHeadY == foodY);

    // Update tail position: each segment moves to where the one in front was
    if (nTail > 0) {
        for (int i = nTail - 1; i > 0; i--) {
            tailX[i] = tailX[i-1];
            tailY[i] = tailY[i-1];
        }
        tailX[0] = headX; // First tail segment takes head's old position
        tailY[0] = headY;
    }

    // Move head to new position
    headX = newHeadX;
    headY = newHeadY;

    // Check for self-collision
    for (int i = 0; i < nTail; i++) {
        if (tailX[i] == headX && tailY[i] == headY) {
            gameOver = 1;
            return;
        }
    }

    // Handle food eating
    if (willEatFood) {
        score += 10;
        nTail++; // Grow the snake
        PlaceFood(); // Place new food
    }
}

// --- Main Game Loop ---
int main() {
    // --- ncurses setup ---
    initscr();
    noecho();
    cbreak();
    curs_set(0);
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);

    // Check if terminal is large enough
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);
    if (max_y < HEIGHT + 6 || max_x < WIDTH + 2) {
        endwin();
        printf("Terminal too small! Need at least %dx%d\n", WIDTH + 2, HEIGHT + 6);
        return 1;
    }

    // --- Time-based loop variables ---
    struct timeval last_update, current_time;
    long elapsed_time;

    bool playing = true;
    do {
        Setup();
        DrawBoard();
        gettimeofday(&last_update, NULL);

        while (!gameOver) {
            Input();

            gettimeofday(&current_time, NULL);
            elapsed_time = (current_time.tv_sec - last_update.tv_sec) * 1000000L +
                           (current_time.tv_usec - last_update.tv_usec);

            if (elapsed_time >= GAME_SPEED) {
                Logic();
                Draw();
                last_update = current_time;
            }
        }

        // Game Over Screen
        nodelay(stdscr, FALSE);
        
        mvprintw(HEIGHT / 2, (WIDTH / 2) - 4, "GAME OVER");
        
        const char* restart_text = "Press 'r' to Restart or 'q' to Quit";
        int text_len = strlen(restart_text);
        mvprintw(HEIGHT / 2 + 2, (WIDTH + 2 - text_len) / 2, "%s", restart_text);
        
        refresh();

        int choice;
        do {
            choice = getch();
        } while (choice != 'r' && choice != 'R' && choice != 'q' && choice != 'Q');

        if (choice == 'q' || choice == 'Q') {
            playing = false;
        } else {
            nodelay(stdscr, TRUE);
        }

    } while (playing);
    
    // --- ncurses cleanup ---
    curs_set(1);
    endwin();

    printf("Thanks for playing! Final Score: %d\n", score);
    return 0;
}
