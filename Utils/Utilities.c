#include <SDL_ttf.h>
#include "Utilities.h"

//width 900
//height 720

void free_alien(void *data) {
    free(*(struct Alien **)data);
}

void free_bullet(void *data) {
    free(*(struct Bullet **) data);
}

void setAttributes(struct Alien *alien, int column, int index, int row) {
    alien->y = 75 + row * 32;

    if (row == 0) {
        alien->x = 16 + column * 60;
        alien->width = 24;
        alien->score = 40;
    }
    else if (row == 1 || row == 2) {
        alien->x = 11 + column * 60;
        alien->width = 33;
        alien->score = 20;
    }
    else {
        alien->x = 10 + column * 60;
        alien->width = 35;
        alien->score = 10;
    }

    alien->height = 24;
    alien->currentSprite = 0;
    alien->index = index;
    alien->row = row;
}

SDL_Surface *getAlienImage(int row) {
    if (row == 0) {
        return loadImage("../../resources/SquidAlien.png");
    }
    else if (row == 1 || row == 2) {
        return loadImage("../../resources/BasicAlien.png");
    }
    else {
        return loadImage("../../resources/FatAlien.png");
    }
}

SDL_Surface *getBulletImage(int dir) {
    if (dir == -1) {
        return loadImage("../../resources/Bullet.png");
    }else {
        return loadImage("../../resources/AlienBullet.png");
    }
}

void getAliens(SDL_Renderer *renderer, struct LinkedList *aliens) {
    int row = 0, column = 0, index = 0;
    createList(aliens, sizeof(struct Alien *), free_alien);

    while (row != 5) {
        SDL_Surface *sheet = getAlienImage(row);
        struct Alien *alien = (struct Alien *) malloc(sizeof(struct Alien));
        setAttributes(alien, column, index, row);
        alien->sheet = SDL_CreateTextureFromSurface(renderer, sheet);
        SDL_FreeSurface(sheet);
        add(aliens, &alien);

        column++;
        index++;
        if (column == 11) {
            column = 0;
            row++;
        }
    }
    printf("LLEgue");
}

bool checkCollisionAlien(struct Bullet *bullet, struct Alien* alien) {
    return
            (bullet->x) < (alien->x + alien->width) &&
            (bullet->y) < (alien->y + alien->height) &&
            (bullet->x + bullet->width) > (alien->x) &&
            (bullet->y + bullet->height) > (alien->y);
}

bool checkCollisionPlayer(struct Bullet *bullet, Player *pl) {
    return
            (bullet->x) < (pl->x + pl->width) &&
            (bullet->y) < (pl->y + pl->height) &&
            (bullet->x + bullet->width) > (pl->x) &&
            (bullet->y + bullet->height) > (pl->y);
}

SDL_Renderer *init() {
    int width = 900;
    int height = 720;

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "Cannot initialise SDL: %s\n", SDL_GetError());
        exit(1);
    }

    SDL_Window *window = SDL_CreateWindow(
            "SpaceInvaders",
            SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED,
            width, height,
            SDL_WINDOW_SHOWN
    );
    if (window == NULL) {
        fprintf(stderr, "Unable to create window: %s\n", SDL_GetError());
        exit(1);
    }

    int imgFlags = IMG_INIT_PNG;
    if( !( IMG_Init( imgFlags ) & imgFlags ) ) {
        printf( "SDL_image could not initialize! SDL_image Error: %s\n", IMG_GetError() );
    }

    if( TTF_Init() == -1 ) {
        printf( "SDL_ttf could not initialize! SDL_ttf Error: %s\n", TTF_GetError() );
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(
            window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );
    if (renderer == NULL) {
        fprintf(stderr, "Unable to create renderer: %s\n", SDL_GetError());
        exit(1);
    }

    if (SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND) != 0) {
        fprintf(stderr, "SDL_BlendMode failed: %s\n", SDL_GetError());
        exit(1);
    }

    return renderer;
}

void addBulletAlien(struct LinkedList *bullets, struct Alien *alien, SDL_Renderer * renderer) {
    struct Bullet *bullet = (struct Bullet *) malloc(sizeof(struct Bullet));
    bullet->y = alien->y - 5;
    bullet->x = alien->x + 15;
    bullet->width = 10;
    bullet->height = 10;
    bullet->direction = 1;
    SDL_Surface *sheet = loadImage("../../resources/AlienBullet.png");
    bullet->sheet = SDL_CreateTextureFromSurface(renderer, sheet);
    SDL_FreeSurface(sheet);
    add(bullets , &bullet);
}

SDL_Texture *loadText(SDL_Renderer *renderer, char* text) {
    TTF_Font* Sans = TTF_OpenFont("../../resources/ostrich-regular.ttf", 24);
    if( Sans == NULL ) {
        printf( "Failed to load lazy font! SDL_ttf Error: %s\n", TTF_GetError() );
    }
    SDL_Color White = {255, 255, 255};
    SDL_Surface* surfaceMessage = TTF_RenderText_Solid(Sans, text, White);
    SDL_Texture* Message = SDL_CreateTextureFromSurface(renderer, surfaceMessage);
    return Message;
}

SDL_Surface *loadImage(char* path) {
    SDL_Surface *sheet = IMG_Load(path);
    if(!sheet) {
        printf("Cannot find sheet\n");
        return NULL;
    }
    return sheet;
}