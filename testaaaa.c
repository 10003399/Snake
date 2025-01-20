#include <stdint.h>

// Memory-mapped addresses
#define VGA_ADDR 0x08000000
#define VGA_BUFFER 0x04000100
#define TIMER_BASE 0x04000020
#define SWITCH_BASE 0x04000010
#define LED_BASE 0x04000000
#define DISPLAY 0x04000050
extern void enable_interrupts();

// Button address
#define BTN_ADDR 0x040000d0

// Screen dimensions
#define VGA_WIDTH 320
#define VGA_HEIGHT 240

// Game grid dimensions
#define GRID_WIDTH 40
#define GRID_HEIGHT 30
#define CELL_SIZE 8 // Each grid cell is one pixel for simplicity

// Snake representation
int snakeX[100] = {10, 9, 8}; // Snake body X-coordinates
int snakeY[100] = {15, 15, 15}; // Snake body Y-coordinates
int snakeLength = 3;

// Apple coordinates
int appleX = 20;
int appleY = 15;
int score = 0;


// Snake direction: 1 = Right, 2 = Left, 3 = Down, 4 = Up
int snakeDirection = 1;


// Display numbers/values
int digit_value[10] = {0xC0, 0xF9, 0xA4, 0xB0, 0x99, 0x92, 0x82, 0xF8, 0x80, 0x90};

// Switch state
int prevswitch = 0;

// VGA buffer pointer
volatile char *VGA = (volatile char *)VGA_ADDR;

// Random seed
static uint32_t seed = 0xDEADBEEF;

// Draw a single cell at the grid coordinate
void drawCell(int gridX, int gridY, char color) {
    int startX = gridX * CELL_SIZE;
    int startY = gridY * CELL_SIZE;

    for (int y = 0; y < CELL_SIZE; y++) {
        for (int x = 0; x < CELL_SIZE; x++) {
            int screenIndex = (startY + y) * VGA_WIDTH + (startX + x);
            VGA[screenIndex] = color;
        }
    }
}

void set_displays ( int display_number, int value){
    volatile int* display = (volatile int*) (DISPLAY + (display_number * 0x10));
    *display = value; 
}

// Initialize hardware resources
 void labinit(void) {
    volatile int *periodL = (int *)0x04000028;
    volatile int *periodH = (int *)0x0400002C;
    volatile int *control = (int *)0x04000024;

    *periodL = (29999999/5) & 0xFFFF;
    *periodH = (29999999/5) >> 16;
    *control = 0x7;

    for (int i = 0; i < 6; i++){
        set_displays(i, digit_value[0]);
    }
    enable_interrupts();
}

//Switch switch
int get_sw (void)
{
    int volatile *switches = (volatile int *)SWITCH_BASE;

    int switchVal = *switches;

    return switchVal & 0x3FF;
}

int get_btn (void){
    int volatile *btn = (volatile int *) BTN_ADDR;
    int btn_status = *btn;

    return btn_status & 0x1;
}


// Custom random number generator
void custom_srand(uint32_t s) {
    seed = s;
}

uint32_t custom_rand() {
    seed = (1664525 * seed + 10139044223);
    return seed;
}

int random_in_range(int min, int max) {
    return (custom_rand() % (max - min + 1)) + min;
}

// Clear the VGA screen
void clearScreen() {
    for (int i = 0; i < snakeLength; i++) {
        drawCell(snakeX[i], snakeY[i], 0);
    }
}


void update_score(int score){

    if (score > 99){
        set_displays(0, digit_value[score % 10]);
        set_displays(1, digit_value[(score / 10) - 1]);
        set_displays(2, digit_value[score / 100]);
    }

    if (score > 9){
        set_displays(0, digit_value[score % 10]);
        set_displays(1, digit_value[score / 10]);
    }

    else{
        set_displays(0, digit_value[score % 10]);
    }

}



// Draw the game grid
void drawGame() {
    // Draw the snake
    for (int i = 0; i < snakeLength; i++) {
        drawCell(snakeX[i], snakeY[i], 10); // Blue color
    }
    // Draw the apple
    drawCell(appleX, appleY, 12); // Green color
}

// Spawn an apple in a random free cell
void spawnApple() {
    int isValid = 0;
    while (!isValid) {
        appleX = random_in_range(1, GRID_WIDTH - 2); // Ensure the apple is within grid bounds
        appleY = random_in_range(1, GRID_HEIGHT - 2);

        isValid = 1;
        // Ensure the apple does not spawn on the snake's body
        for (int i = 0; i < snakeLength; i++) {
            if (snakeX[i] == appleX && snakeY[i] == appleY) {
                isValid = 0;
                break;
            }
        }
    }
}

void clearall(){
    for (int i = 0; i < VGA_HEIGHT * VGA_WIDTH; i++){
        VGA[i] = 0;
    }
}

void endgame(){
    volatile int *control = (volatile int*)0x04000024;
    *control = 0x8;
}

// Update the snake's position
void movesnake() {
    int prevX = snakeX[0];
    int prevY = snakeY[0];
    int tempX, tempY;

    // Move the head
    switch (snakeDirection) {
        case 1: snakeX[0]++; break; // Move right
        case 2: snakeX[0]--; break; // Move left
        case 3: snakeY[0]++; break; // Move up
        case 4: snakeY[0]--; break; // Move down
    }

    //drawCell(prevX, prevY, 0);
   
    
    // Move the body
    for (int i = 1; i < snakeLength; i++) {
        tempX = snakeX[i];
        tempY = snakeY[i];
        snakeX[i] = prevX;
        snakeY[i] = prevY;
        prevX = tempX;
        prevY = tempY;
    }

    if (snakeX[0] > GRID_WIDTH - 2) {
        endgame(); // Wrap to left wall
    }
    if (snakeX[0] < + 1) {
        endgame();// Wrap to right wall
    }

    // Wrap around Y-axis
    if (snakeY[0] > GRID_HEIGHT - 2) {
        endgame(); // Wrap to top wall
    }
    if (snakeY[0] < + 1) {
        endgame(); // Wrap to bottom wall
    }

    // Check if the snake eats the apple
    if (snakeX[0] == appleX && snakeY[0] == appleY) {

        snakeLength++;
        score++;
        update_score(score);
        spawnApple();
        movesnake();
    }

    // Check if the snake hits itself
    if (snakeLength > 5){
        for (int i = 1; i < snakeLength; i++){
            if(snakeX[0] == snakeX[i] && snakeY[0] == snakeY[i]){
                endgame();
            }
            
        }
    }
}


// Update LEDs based on the game state
void set_leds(int led_mask) {
    volatile int *ledp = (volatile int *)LED_BASE;
    *ledp = led_mask & 0x3FF;
}

void checkSwitches() {
    volatile int *switches = (volatile int *)SWITCH_BASE;
    int switchState = *switches;
    int change = switchState - prevswitch;

    if (change < 0) {
        change = -change;
    }

    // Update the direction based on the switch change
    switch (change) {
        case 1:
            if (snakeDirection != 2) snakeDirection = 1;  // Right
            break;
        case 512:
            if (snakeDirection != 1) snakeDirection = 2;  // Left
            break;
        case 2:
            if (snakeDirection != 3) snakeDirection = 4;  // Up
            break;
        case 256:
            if (snakeDirection != 4) snakeDirection = 3;  // Down
            break;
        case 32:
            clearall();
            appleX = 20;
            appleY = 15;
            score = 0;
            snakeLength = 3;
            for (int i = 8; i < 11; i++){
                snakeX[i - 8] = i;
                snakeY[i - 8] = 15;
            }
            snakeDirection = 1;
            labinit();
            break;
        default:
            break;
    }

    prevswitch = switchState; 
}



// Timer interrupt handler
void handle_interrupt(unsigned cause) {
    clearScreen();
    movesnake();
    drawGame();
    

    if (cause == 16){
        // Acknowledge the interrupt
        volatile int *timerControl = (volatile int *)TIMER_BASE;
        *timerControl = 0;
    }
    
    if (cause == 17){
        checkSwitches();
        
    }

}

// Main function
int main() {
    custom_srand(0x12345678);
    drawCell(appleX, appleY, 12);
    drawGame();
    while (1) {
        
        if(get_btn() == 1){
            labinit();
            drawGame();
            while(1){
                checkSwitches();
            }
        }
        
    }
    //done
    return 0;
}


