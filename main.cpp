#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

using Byte = unsigned char;

// store font data in memory (050-09F)
void loadFont(std::vector<Byte> &ram) {
    // open file
    std::ifstream file {"font.txt"};
    if(!file) {
        std::cerr << "Font file could not be opened.\n";
        // HANDLE ERROR
        // return 1;
    }

    std::string strInput{};
    int i = 0x50;
    while (file >> strInput) {
        ram[i++] = std::stoi(strInput, 0, 16);
    }
}

// load program into memory at 0x200
void loadProgram(std::vector<Byte> &ram) {
    std::ifstream programFile {"test.ch8"};
    if(!programFile) {
        std::cerr << "Program file could not be opened.\n";
        // HANDLE ERROR
        // return 1;
    }

    std::string strInput{};
    int i = 0x200;
    while (programFile >> strInput) {
        ram[i++] std::stoi(strInput, 0, 16);
    }
}

bool initSDL(SDL_Window*& window, SDL_Surface*& screenSurface, 
                int width, int height) {
    // Initialization flag
    bool success = true;

    // Initialize SDL
    if (!SDL_Init( SDL_INIT_VIDEO)) {
        SDL_Log("SDL could not initialize! SDL_Error: %s\n", 
            SDL_GetError());
		success = false;
    } else {
        // Create window
        window = SDL_CreateWindow("Chip-8", width, height, 0);
        if (window == NULL) {
            SDL_Log("Window could not be created! SDL_Error: %\n", 
                SDL_GetError());
        } else {
            // Get window surface
            screenSurface = SDL_GetWindowSurface(window);
        }
    }
    return success;
}

void closeSDL(SDL_Window*& window, SDL_Surface*& screenSurface) {
    // Destroy window
    SDL_DestroyWindow(window);
    window = NULL;

    // Quit SDL subsystems
    SDL_Quit();
}

int main(int argc, char* args[]) {

    // 4 kB RAM (program should be loaded at 512 or 0x200)
    std::vector<Byte> ram (4096);
    // PC (16-bit, 12-bit actual)
    char16_t pc;
    // 16-bit index register (I)
    char16_t i_reg;
    // 16-bit stack
    std::vector<char16_t> stack (16); // 16 levels of nesting
    // 8-bit delay timer
    Byte delay_timer;
    // 8-bit sound timer
    Byte sound_timer;
    // 16 8-bit variable registers (V0 - VF)
    std::vector<char16_t> registers (16);
    // screen dimensions
    const int screen_width = 64;
    const int screen_height = 32;

    loadFont(ram);
    loadProgram(ram);
    pc = 0x200;

    // SDL stuff
    SDL_Window* window = NULL;
    SDL_Surface* screenSurface = NULL;

    if (!initSDL(window, screenSurface, 
            screen_width, screen_height)) {
        SDL_Log("Failed to initialize!\n");
    } else {
        // Fill the surface white
        SDL_FillSurfaceRect(screenSurface, NULL, 
            SDL_MapSurfaceRGB( screenSurface, 0xFF, 0xFF, 0xFF));
        // Update the surface
        SDL_UpdateWindowSurface(window);

        // Main loop flag
        bool quit = false;

        // Event handler
        SDL_Event e;
        
        // While application is running
        while (!quit) {
            // Handle events on queue
            while (SDL_PollEvent(&e) != 0) {
                // User requests quit
                if (e.type == SDL_EVENT_QUIT) {
                    quit = true;
                } // else if user presses a key
            }
        }
    }

    // Free resources and close SDL
    closeSDL(window, screenSurface);

    return 0;
}