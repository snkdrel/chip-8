#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

using Byte = unsigned char;

// 4 kB RAM (program should be loaded at 512 or 0x200)
std::vector<Byte> g_ram (4096);
// PC (16-bit, 12-bit actual)
char16_t g_pc;
// 16-bit index register (I)
char16_t g_i;
// 16-bit stack
std::vector<char16_t> g_stack (16); // 16 levels of nesting
// 8-bit delay timer
Byte g_delay_timer;
// 8-bit sound timer
Byte g_sound_timer;
// 16 8-bit variable registers (V0 - VF)
std::vector<char16_t> g_registers (16);

const int SCREEN_WIDTH = 64;
const int SCREEN_HEIGHT = 32;

// store font data in memory (050-09F)
void loadFont() {
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
        g_ram[i++] = std::stoi(strInput, 0, 16);
    }

    // Output font data
    /*
    for(int i = 0x50; i <= 0x9f; i++) {
        outf << g_ram[i] << std::endl;
    }
    */
}

int main(int argc, char* args[]) {

    loadFont();

    // SDL stuff
    SDL_Window* window = NULL;

    SDL_Surface* screenSurface = NULL;

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL could not initialize! SDL_Error: %s\n", 
            SDL_GetError());
    } else {
        window = SDL_CreateWindow("Chip-8", SCREEN_WIDTH, 
            SCREEN_HEIGHT, 0);
        if (window == NULL) {
            SDL_Log("Window could not be created! SDL_Error: %\n", 
                SDL_GetError());
        } else {
            screenSurface = SDL_GetWindowSurface(window);

            SDL_FillSurfaceRect(screenSurface, NULL, 
                SDL_MapSurfaceRGB(screenSurface, 0xFF, 0xFF, 0xFF));

            SDL_UpdateWindowSurface(window);

            SDL_Event e; bool quit = false; while( quit == false ){ while( SDL_PollEvent( &e ) ){ if( e.type == SDL_EVENT_QUIT ) quit = true; } }

            SDL_DestroyWindow(window);

            SDL_Quit();

            return 0;
        }
    }
}