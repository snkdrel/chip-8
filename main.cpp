#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include <bitset>
#include <random>
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
    std::ifstream programFile {"testPrograms/3-corax+.ch8", std::ios::binary};
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

Nibble mapKeyToValue(SDL_Keycode key) {
    switch (key) 
    {
    case SDLK_1:
        return 1;
    case SDLK_2:
        return 2;
    case SDLK_3:
        return 3;
    case SDLK_4:
        return 0xC;
    case SDLK_Q:
        return 4;
    case SDLK_W:
        return 5;
    case SDLK_E:
        return 6;
    case SDLK_R:
        return 0xD;
    case SDLK_A:
        return 7;
    case SDLK_S:
        return 8;
    case SDLK_D:
        return 9;
    case SDLK_F:
        return 0xE;
    case SDLK_Z:
        return 0xA;
    case SDLK_X:
        return 0;
    case SDLK_C:
        return 0xB;
    case SDLK_V:
        return 0xF;
    default:
        return -1;
    }
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
    std::vector<Byte> registers (16);
    // Delay and sound timers
    Byte dTimer = 0;
    Byte sTimer = 0;
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
    // instantiate PRNG engine
    std::mt19937 mt{}; // 32-bit mersenne twister

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
            
            // Update screen and timer (60Hz)
            currentTime = SDL_GetTicks();
            if (currentTime > lastUpdate + 16.66) {
                // update screen
                updateScreen(pixels, screen_width, screen_height, renderer);
                // Update timers
                if (dTimer > 0) dTimer--;
                if (sTimer > 0) sTimer--;
                
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
                        ((third_nibble) << 4 | fourth_nibble)) {
                    pc += 2;
                }
                break;
            case 4: // skip if VX does not equal NN
                if (registers[second_nibble] != 
                        ((third_nibble) << 4 | fourth_nibble)) {
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
            case 0xC: // CXNN (Random)
                registers[second_nibble] = 
                    mt() & (third_nibble << 4 | fourth_nibble);
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
            case 0xE:
                if (third_nibble == 0x9 && fourth_nibble == 0xE) {
                    // EX9E skip if key is pressed
                    if (e.type == SDL_EVENT_KEY_DOWN) {
                        // check if VX key is pressed
                        if (mapKeyToValue(e.key.key) == second_nibble) {
                            pc += 2;
                        }
                    }
                } else if (third_nibble == 0xA && fourth_nibble == 0x1) {
                    // EXA1 skip if key is not pressed
                    if (e.type != SDL_EVENT_KEY_DOWN || 
                            mapKeyToValue(e.key.key) != second_nibble) {
                        pc += 2;
                    }
                }
            case 0xF:
                if (third_nibble == 0 && fourth_nibble == 7) {
                    // Sets VX to delay timer value
                    registers[second_nibble] = dTimer;
                } else if (third_nibble == 1 && fourth_nibble == 5) {
                    // Sets delay timer to VX value
                    dTimer = registers[second_nibble];
                } else if (third_nibble == 1 && fourth_nibble == 8) {
                    // set sound timer to VX value
                    sTimer = registers[second_nibble];
                } else if (third_nibble == 1 && fourth_nibble == 0xE) {
                    // add VX value to I register
                    i_reg += registers[second_nibble];
                    // Amiga (spacefight 2091!) behavior
                    if (i_reg > 0x0FFF) {
                        registers[0xF] = 1;
                    }
                } else if (third_nibble == 0 && fourth_nibble == 0xA) {
                    // Get key
                    if (e.type == SDL_EVENT_KEY_DOWN) {
                        Nibble val = mapKeyToValue(e.key.key);
                        if (val != -1) {
                            registers[second_nibble] = val;
                        } else {
                            pc -= 2;
                        }
                    } else {
                        pc -= 2;
                    }
                } else if (third_nibble == 2 && fourth_nibble == 9) {
                    // font character
                    int offset = 40 * (registers[second_nibble] & 0xF);
                    i_reg = 0x50 + offset;
                } else if (third_nibble == 3 && fourth_nibble == 3) {
                    // binary-coded decimal conversion
                    ram[i_reg] = registers[second_nibble] / 100;
                    ram[i_reg+1] = (registers[second_nibble] / 10) % 10;
                    ram[i_reg+2] = registers[second_nibble] % 10;
                } else if (third_nibble == 5 && fourth_nibble == 5) {
                    // store registers into memory
                    for (int i = 0; i <= second_nibble; i++) {
                        ram[i_reg + i] = registers[i];
                    }
                    // configurable
                    //i_reg = i_reg + second_nibble + 1;
                } else if (third_nibble == 6 && fourth_nibble == 5) {
                    // load memory into registers
                    for (int i = 0; i <= second_nibble; i++) {
                        registers[i] = ram[i_reg + i];
                    }
                    // configurable
                    //i_reg = i_reg + second_nibble + 1;
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