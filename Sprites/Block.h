
#ifndef SPACEINVADERS_BLOCK_H
#define SPACEINVADERS_BLOCK_H

#include <SDL_rect.h>
#include <stdbool.h>
#include "../Utils/LinkedList.h"
#include "../Sprites/Sprite.h"

struct SpaceRect {
    SDL_Rect *rect;
    int i, j;
};

struct Block {
    struct LinkedList *shield;
    int state[14][16];
};

void free_block(void *data);
void free_rect(void *data);
void buildBlockMatrix(struct Block *block);
void createBlock(struct Block *block, int xPos);
void generateShields(struct LinkedList *shields);
bool collisionA(struct SDL_Rect *rect, struct Alien *alien);
bool collisionB(struct SDL_Rect *rect, struct Bullet *bullet);
void collisionAlienBlock(struct LinkedList *aliens, struct LinkedList *shields);
void searchCollision(struct LinkedList *bullets, struct LinkedList *shields);
void deleteRect(struct Block *block, int i, int j);

#endif //SPACEINVADERS_BLOCK_H
