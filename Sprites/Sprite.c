#include "Sprite.h"

void changeSpriteAliens(struct LinkedList *aliens, int *time) {
    *time = *time + 1;
    int timeaux = *time;
    if (timeaux > 50) {
        *time = 0;
    }
    if (timeaux % 10 == 0) {
        for (int i = 0; i < aliens->size; ++i) {
            struct Alien *tmp = *(struct Alien **) get(aliens, i);
            tmp->currentSprite = ++tmp->currentSprite % 2;
        }
    }
}
