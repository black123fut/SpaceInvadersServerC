cmake_minimum_required(VERSION 3.13)
project(SpaceInvadersServer C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/CMAKE")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake")

find_package(SDL2 REQUIRED)
find_package(SDL2_IMAGE)
find_package(SDL2TTF)
find_package(SDL2Mixer)

add_executable(SpaceInvadersServer main.c Utils/LinkedList.c
                                    Utils/LinkedList.h
                                    Sprites/Sprite.h
                                    Utils/Utilities.h
                                    Utils/Utilities.c
                                    Sprites/Block.h Sprites/Block.c
                                    Sprites/Sprite.c
                                    JsonBuilder/JsonConstructor.h
                                    JsonBuilder/JsonConstructor.c
                                    Conectividad/Ssocket.h
                                    Conectividad/Ssocket.c)

include_directories(${SDL2_INCLUDE_DIR} ${SDL2TTF_INCLUDE_DIR} ${SDL2_IMAGE_INCLUDE_DIR} ${SDL2Mixer_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} ${SDL2_LIBRARY} ${SDL2TTF_LIBRARY} ${SDL2_IMAGE_LIBRARY} ${SDL2Mixer_LIBRARY} json-c)

