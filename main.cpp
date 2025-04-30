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
    renderer = NULL;

    // Destroy window
    SDL_DestroyWindow(window);
    window = NULL;

    // Quit SDL subsystems
    SDL_Quit();
}

void clearScreen (std::vector<std::vector<bool>> & pixels,
                    SDL_Renderer* renderer) {
    std::fill(pixels.begin(), pixels.end(), 
        std::vector<bool>(64, false));
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);
}

void updateScreen (const std::vector<std::vector<bool>> & pixels, 
                    int w, int h, SDL_Renderer* renderer) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_RenderClear(renderer);
    SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
    for (int i = 0; i < w; i++) {
        for (int j = 0; j < h; j++) {
            if (pixels[i][j]) {
                SDL_FRect rect = {
                    i * 10, // x coordinate
                    j * 10, // y coordinate
                    9, // width
                    10 // height 
                };
                SDL_RenderFillRect(renderer, &rect);
            }
        }
    }
    SDL_RenderPresent(renderer);
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
    const int screen_width = 64;
    const int screen_height = 32;
    // pixels' on / off states
    std::vector< std::vector<bool> > pixels (
        screen_width, std::vector<bool>(screen_height, false));
    // struct for drawing rects as pixels 
    SDL_FRect rect = {
        0, // x coordinate
        0, // y coordinate
        10, // width
        10, // height
    };
    // size of loaded program
    int programSize;
    // timer variables
    unsigned int lastUpdate = 0, currentTime;

    loadFont(ram);
    loadProgram(ram, programSize);
    pc = 0x200;

    // SDL stuff
    SDL_Window* window = NULL;
    SDL_Renderer* renderer;

    if (!initSDL(window, renderer, 
            screen_width * 10, screen_height * 10)) {
        SDL_Log("Failed to initialize!\n");
    } else {        
        // Main loop flag
        bool quit = false;

        // Event handler
        SDL_Event e;
        
        while (SDL_PollEvent(&e) != 0 || !quit) {
            if (e.type == SDL_EVENT_QUIT) {
                quit = true;
            }
            
            // Check if screen should update
            currentTime = SDL_GetTicks();
            if (currentTime > lastUpdate + 16.66) { // 60 fps
                updateScreen(pixels, screen_width, screen_height, renderer);
                lastUpdate = currentTime;
            }

            // decode instruction
            Nibble first_nibble = (ram[pc] & 0xF0) >> 4;
            Nibble second_nibble = ram[pc++] & 0x0F;
            Nibble third_nibble = (ram[pc] & 0xF0) >> 4;
            Nibble fourth_nibble = ram[pc++] & 0x0F;
            
            switch(first_nibble) 
            {
            case 0:
                if (second_nibble == 0 && third_nibble == 0xE 
                        && fourth_nibble == 0x0) { 
                    // clear the screen
                    clearScreen(pixels, renderer);
                } else if (second_nibble == 0 && third_nibble == 0xE 
                        && fourth_nibble == 0xE) { 
                    // return
                    pc = stack.back();
                    stack.pop_back();
                } else { 
                    // call machine code routine at NNN
                }
                break;
            case 1: // jump (set pc to NNN)
                pc = (second_nibble << 8) | (third_nibble << 4) 
                    | fourth_nibble;
                break;
            case 2: // calls subroutine at NNN
                stack.push_back(pc);
                pc = (second_nibble << 8) | (third_nibble << 4) 
                    | fourth_nibble;
                break;
            case 3: // skip if VX equals NN
                if (registers[second_nibble] == 
                        (third_nibble) << 4 | fourth_nibble) {
                    pc += 2;
                }
                break;
            case 4: // skip if VX does not equal NN
                if (registers[second_nibble] != 
                        (third_nibble) << 4 | fourth_nibble) {
                    pc += 2;
                }
                break;
            case 5: // skip if VX == VY
                if (registers[second_nibble] == 
                        registers[third_nibble]) {
                    pc += 2;
                }
                break;
            case 6: // set register VX to NN
                registers[second_nibble] = 
                    (third_nibble << 4) | fourth_nibble;
                break;
            case 7: // add value NN to register VX
                registers[second_nibble] += 
                    (third_nibble << 4) | fourth_nibble;
                break;
            case 8: // logical and arithmetic instructions
                switch (fourth_nibble) {
                    case 0: // Set VX to value of VY
                        registers[second_nibble] = 
                            registers[third_nibble];
                        break;
                    case 1: // Binary OR
                        registers[second_nibble] |= 
                            registers[third_nibble];
                        break;
                    case 2: // Binary AND
                        registers[second_nibble] &= 
                            registers[third_nibble];
                        break;
                    case 3: // Logical XOR
                        registers[second_nibble] ^= 
                            registers[third_nibble];
                        break;
                    case 4: // Add
                        // check for overflow
                        if (registers[third_nibble] > 0 
                                && registers[second_nibble] > 65535 - registers[third_nibble]) {
                            registers[0xF] = 1; 
                        } else {
                            registers[0xF] = 0;
                        }
                        registers[second_nibble] += 
                            registers[third_nibble];
                        break;
                    case 5: // Subtract VX - VY
                        if (registers[second_nibble] > registers[third_nibble]) {
                            registers[0xF] = 1;
                        } else {
                            registers[0xF] = 0;
                        }
                        registers[second_nibble] = 
                            registers[second_nibble] - registers[third_nibble];
                        break;
                    case 6: // Shift right
                        // Optional or configurable
                        // registers[second_nibble] = registers[third_nibble];
                        registers[0xF] = 
                            registers[second_nibble] & 1;
                        registers[second_nibble] = 
                            registers[second_nibble] >> 1;
                        break;
                    case 7: // Subtract VY - VX
                        if (registers[third_nibble] > registers[second_nibble]) {
                            registers[0xF] = 1;
                        } else {
                            registers[0xF] = 0;
                        }
                        registers[second_nibble] = 
                            registers[third_nibble] - registers[second_nibble];
                        break;
                    case 0xE: // Shift left
                        // Optional or configurable
                        // registers[second_nibble] = registers[third_nibble];
                        registers[0xF] = 
                            (registers[second_nibble] & (1 << 15)) >> 15;
                        registers[second_nibble] = 
                            registers[second_nibble] << 1;
                        break;
                }
            case 9: // skip if VX != VY
                if (registers[second_nibble] != 
                        registers[third_nibble]) {
                    pc += 2;
                }
                break;
            case 0xA: // set index register I to NNN
                i_reg = (second_nibble << 8) 
                        | (third_nibble << 4) | fourth_nibble;
                break;
            case 0xB: // Jump with offset
                // optional / configurable 
                // XNN plus value in register VX
                // pc = registers[second_nibble] + (second_nibble << 8) | (third_nibble << 4) | fourth_nibble;
                pc = registers[0] + 
                    ((second_nibble << 8) | (third_nibble << 4) 
                    | fourth_nibble);
                break;
            case 0xD: // display (DXYN)*/
                {
                    // get x and y coordinates
                    int x = registers[second_nibble] & (screen_width - 1);
                    int y = registers[third_nibble] & (screen_height - 1);
                    
                    // set flag register to 0
                    registers[0xF] = 0;

                    // iterate over height N
                    for (int i = 0; i < fourth_nibble; i++) {
                        // if screen bottom is reached
                        if (y + i >= screen_height) {
                            break;
                        }
                        // iterate over sprite row
                        Byte spriteRow = ram[i_reg + i];
                        for (int j = 7; j >= 0; j--) {
                            // if screen edge is reached
                            if (x + 7 - j >= screen_width) {
                                continue;
                            }

                            bool currentBit = spriteRow & (1 << j);
                            // coordinates for pixels vector
                            int xP = x + 7 - j;
                            int yP = y + i;

                            if (currentBit) {
                                if (pixels[xP][yP]) {
                                    registers[0xF] = 1;
                                }
                                pixels[xP][yP] = !pixels[xP][yP];
                            }
                        }
                    }
                }
                break;
            default: // undetermined
                /*
                std::cout << std::hex << 
                    (int)first_nibble <<
                    (int)second_nibble <<
                    (int)third_nibble <<
                    (int)fourth_nibble << "\n";
                */
                break;
                // ERROR
            }
        }
    }

    // Free resources and close SDL
    closeSDL(window, renderer);

    return 0;
}