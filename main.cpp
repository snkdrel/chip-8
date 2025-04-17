#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include <bitset>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

using Byte = unsigned char;
using Nibble = Byte;
using TwoByte = char16_t;

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
void loadProgram(std::vector<Byte> &ram, int &programSize) {
    std::ifstream programFile {"test.ch8", std::ios::binary};
    if(!programFile) {
        std::cerr << "Program file could not be opened.\n";
        // HANDLE ERROR
        // return 1;
    }
    // determine file length
    programFile.seekg(0, programFile.end);
    programSize = programFile.tellg();
    programFile.seekg(0, programFile.beg);
    
    for (int i = 0x200; i < programSize + 0x200; i++) {
        programFile.read((char*) &ram[i], sizeof(Byte));
    }
}

bool initSDL(SDL_Window*& window, SDL_Renderer*& renderer, 
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
            renderer = SDL_CreateRenderer(window, NULL);
        }
    }
    return success;
}

void closeSDL(SDL_Window*& window, SDL_Renderer*& renderer) {
    // Destroy renderer
    SDL_DestroyRenderer(renderer);

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
    TwoByte pc;
    // 16-bit index register (I)
    TwoByte i_reg;
    // 16-bit stack
    std::vector<TwoByte> stack (16); // 16 levels of nesting
    // 8-bit delay timer
    Byte delay_timer;
    // 8-bit sound timer
    Byte sound_timer;
    // 16 8-bit variable registers (V0 - VF)
    std::vector<TwoByte> registers (16);
    // screen dimensions
    const int screen_width = 640;
    const int screen_height = 320;
    // pixels' on / off states
    std::vector< std::vector<bool> > pixels (32, std::vector<bool>(64, false));
    // struct for drawing rects as pixels 
    SDL_FRect rect = {
        0, // x coordinate
        0, // y coordinate
        10, // width
        10, // height
    };
    // size of loaded program
    int programSize;

    loadFont(ram);
    loadProgram(ram, programSize);
    pc = 0x200;

    // SDL stuff
    SDL_Window* window = NULL;
    SDL_Renderer* renderer;

    if (!initSDL(window, renderer, 
            screen_width, screen_height)) {
        SDL_Log("Failed to initialize!\n");
    } else {        
        /*
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
        */
        
        while (pc < 0x200 + programSize) {
            
            // decode instruction
            Nibble first_nibble = (ram[pc] & 0xF0) >> 4;
            Nibble second_nibble = ram[pc++] & 0x0F;
            Nibble third_nibble = (ram[pc] & 0xF0) >> 4;
            Nibble fourth_nibble = ram[pc++] & 0x0F;
            
            switch(first_nibble) {
                case 0:
                    if (second_nibble == 0 && 
                            third_nibble == 0xE && 
                            fourth_nibble == 0x0) { // clear screen
                        std::fill(pixels.begin(), pixels.end(), 
                            std::vector<bool>(64, false));
                        SDL_SetRenderDrawColor(
                            renderer, 0, 0, 0, 0);
                        SDL_RenderClear(renderer);
                        SDL_RenderPresent(renderer);
                    } else if (second_nibble == 0 && 
                        third_nibble == 0xE && 
                        fourth_nibble == 0xE) { // return
                        //std::cout << "return\n";
                    } else { // call machine code routine at NNN
                        //std::cout << "call machine language routine\n";   
                    }
                    break;
                case 1: // jump (set pc to NNN)
                    pc = (second_nibble << 8) 
                        & (third_nibble << 4) & fourth_nibble;
                    break;
                case 6: // set register VX to NN
                    registers[second_nibble] = 
                        (third_nibble << 4) & fourth_nibble;
                    break;
                case 7: // add value NN to register VX
                    registers[second_nibble] += 
                        (third_nibble << 4) & fourth_nibble;
                    break;
                case 0xA: // set index register I to NNN
                    i_reg = (second_nibble << 8) 
                            & (third_nibble << 4) & fourth_nibble;
                    break;
                case 0xD: // display (DXYN)
                    {
                        // get x and y coordinates
                        int x = registers[second_nibble] & 63; // mod 64
                        int y = registers[third_nibble] & 31;

                        // set flag register to 0
                        registers[0xF] = 0;

                        // iterate over height N
                        for (int i = 0; i < fourth_nibble; i++) {
                            // if screen bottom is reached
                            if (y + i >= 32) {
                                break;
                            }
                            // iterate over sprite row
                            Byte spriteRow = ram[i_reg + i];
                            for (int j = 7; j >= 0; j--) {
                                // if screen edge is reached
                                if (x + 7 - j >= 64) {
                                    continue;
                                }
                                bool currentBit = spriteRow & (1 << j);
                                // coordinates for pixels vector
                                int xP = x + 7 - j;
                                int yP = y + i;
                                
                                if (currentBit) {
                                    if (pixels[xP][yP]) {
                                        registers[0xF] = 1;
                                        // turn off pixel
                                        pixels[xP][yP] = false;
                                        SDL_SetRenderDrawColor(
                                            renderer, 0, 0, 0, 0);
                                    } else {
                                        // turn on pixel
                                        pixels[xP][yP] = true;
                                        SDL_SetRenderDrawColor(
                                            renderer, 0xFF, 0xFF, 0xFF, 0xFF);
                                    }
                                    // update backbuffer
                                    rect.x = xP * 10;
                                    rect.y = yP * 10;
                                    SDL_RenderFillRect(renderer, &rect);
                                }
                                SDL_RenderPresent(renderer);
                            }
                        }
                    }
                    break;
                default: // undetermined
                    std::cout << std::hex << 
                        (int)first_nibble <<
                        (int)second_nibble <<
                        (int)third_nibble <<
                        (int)fourth_nibble << "\n";
                    break;
                    // ERROR
            }
        }
    }

    // Free resources and close SDL
    closeSDL(window, renderer);

    return 0;
}