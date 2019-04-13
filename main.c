#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <SDL_ttf.h>
#include <pthread.h>

#include "SDL2/SDL.h"
#include "SDL2/SDL_image.h"
#include "Utils/LinkedList.h"
#include "Sprites/Block.h"
#include "Sprites/Sprite.h"
#include "Utils/Utilities.h"
#include "Conectividad/Ssocket.h"
#include "JsonBuilder/JsonConstructor.h"


int globalTime = 0, border = 10, alieny = 75, speed = 1, isObserver;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
const int puerto = 8080;
Player pl;
struct game_info game;
struct LinkedList *clients;
struct LinkedList *aliens;
struct LinkedList *bullets;
struct LinkedList *shields;
SDL_Renderer *renderer;

int n = 0;
bool isServer = true, playerConnected = false, serverConnected = false, lose = false;

struct client_info {
    int sockno;
    char ip[INET_ADDRSTRLEN];
};

void handler(int s) {
    printf("Caught SIGPIPE\n");
    exit(1);
}

void updateServer(Player *pl, struct LinkedList *aliens,
                  struct LinkedList *bullets,
                  struct LinkedList *shields,
                  SDL_Renderer *renderer,
                  const char *buffer) {

    int goDown = 0;
    if (isServer)
        changeSpriteAliens(aliens, &globalTime);

    if (pl->cooldown > 0) {
        pl->cooldown--;
    }

    //Cuando los aliens tocan los limites de la pantalla
    if (border >= 265) {
        goDown = 1;
        border = 265;
        speed = abs(speed) * (-1);
    }
    else if (border <= 0) {
        goDown = 1;
        border = 0;
        speed = abs(speed);
    }
    border += speed;

    //Selecciona uhn alien random para disparar
    if (globalTime % 50 == 0 && globalTime != 0) {
        struct Alien *alien = *(struct Alien **) get(aliens, rand() % length(aliens));
        addBulletAlien(bullets, alien, renderer);
    }

    //Mueve los aliens
    for (int i = 0; i < aliens->size; ++i) {
        struct Alien *tmp = *(struct Alien **) get(aliens, i);
        tmp->x += speed;
        tmp->y += 40 * goDown;
        if (tmp->y >= 650)
            lose = true;
    }

    if (goDown == 1)
        alieny += 40 * goDown;

    if (alieny == 155) {
        if (speed < 0)
            speed--;
        else
            speed++;
        alieny = 75;
    }


    //Mueve balas y Balas fueras del limite
    for (int k = 0; k < length(bullets); ++k) {
        struct Bullet *tmp = *(struct Bullet **) get(bullets, k);
        tmp->y += 15 * tmp->direction;

        if (tmp->y <= 0 || tmp->y >= 730) {
            delete_node(bullets, k, "Alien Bullet");
        }
    }

    //Colisiones
    if (bullets->size > 0 && length(aliens) > 0) {
        for (int l = 0; l < aliens->size; ++l) {
            struct Alien *alientmp = *(struct Alien **) get(aliens, l);
            for (int i = 0; i < length(bullets); ++i) {
                struct Bullet *bullettmp = *(struct Bullet **) get(bullets, i);

                if (bullettmp->direction == -1 && checkCollisionAlien(bullettmp, alientmp)) {
                    delete_node(bullets, i, "Player Bullet");
                    game.score += alientmp->score;
                    delete_node(aliens, l, "Alien");
                }
                else if (bullettmp->direction == 1 && checkCollisionPlayer(bullettmp, pl)) {
                    delete_node(bullets, i, "Player Bullet");
                    game.lifes--;
                }
            }
        }
    }

//    if (length(aliens) == 0) {
//        getAliens(renderer, aliens);
//        speed = 1;
//    }

    if (game.lifes == 0) {
        lose = true;
    }

    //Destruccion de los escudos
    if (bullets->size > 0 && length(aliens) > 0) {
        searchCollision(bullets, shields);
        collisionAlienBlock(aliens, shields);
    }
}

void updatePlayer(Player *pl, struct LinkedList *bullets,
                const char *buffer, struct game_info *game) {
    int typeRes;
    //Json---------------------------------------------
    struct json_object *parsed_json;
    struct json_object *tipo;
    struct json_object *jugador;
    struct json_object *temp_jug;
    struct json_object *balas;
    struct json_object *bala;
    struct json_object *score;
    struct json_object *vidas;

    parsed_json = json_tokener_parse(buffer);

    typeRes = isObserver;

    if (typeRes == 0) {
        json_object_object_get_ex(parsed_json,"Player", &jugador);
        json_object_object_get_ex(parsed_json,"Bullets", &balas);
        json_object_object_get_ex(parsed_json,"Score", &score);
        json_object_object_get_ex(parsed_json,"Vidas", &vidas);

        game->score = json_object_get_int(score);
        game->lifes = json_object_get_int(vidas);


        json_object_object_get_ex(jugador,"x",&temp_jug);
        pl->x = json_object_get_int(temp_jug);
        json_object_object_get_ex(jugador,"y",&temp_jug);
        pl->y = json_object_get_int(temp_jug);
        json_object_object_get_ex(jugador,"w",&temp_jug);
        pl->width = json_object_get_int(temp_jug);
        json_object_object_get_ex(jugador,"h",&temp_jug);
        pl->height = json_object_get_int(temp_jug);
        json_object_object_get_ex(jugador,"cSpr",&temp_jug);
        pl->currentSprite = json_object_get_int(temp_jug);
        json_object_object_get_ex(jugador,"time",&temp_jug);
        pl->time = json_object_get_int(temp_jug);
        json_object_object_get_ex(jugador,"cooldown",&temp_jug);
        pl->cooldown = json_object_get_int(temp_jug);

        int numBalas = json_object_array_length(balas);
        printf("Numero de Balas:  %i\n",numBalas);
        clear_list(bullets);
        for (int i = 0; i < numBalas; i++) {
            bala = json_object_array_get_idx(balas, i);
            struct json_object *temp_bala;
            struct Bullet *tmp = (struct Bullet *) malloc(sizeof(struct Bullet));
            json_object_object_get_ex(bala, "x", &temp_bala);
            tmp->x = json_object_get_int(temp_bala);
            json_object_object_get_ex(bala, "y", &temp_bala);
            tmp->y = json_object_get_int(temp_bala);
            json_object_object_get_ex(bala, "w", &temp_bala);
            tmp->width = json_object_get_int(temp_bala);
            json_object_object_get_ex(bala, "h", &temp_bala);
            tmp->height = json_object_get_int(temp_bala);
            json_object_object_get_ex(bala, "cSpr", &temp_bala);
            tmp->currentSprite = json_object_get_int(temp_bala);
            json_object_object_get_ex(bala, "dir", &temp_bala);
            tmp->direction = json_object_get_int(temp_bala);
            SDL_Surface *sheet = getBulletImage(tmp->direction);
            tmp->sheet = SDL_CreateTextureFromSurface(renderer, sheet);
            SDL_FreeSurface(sheet);
            add(bullets, &tmp);
        }
    }
}

void *send_to_observer(void *sock) {
    struct client_info client = *((struct client_info *) sock);
    char buffer[12000];
    char len[4];
    int largo;

    while (1) {
        json_object *jobj = json_object_new_object();
        stateJson(lose, jobj);
        typeClient(1, jobj);
        gameInfoJson(&game, jobj);
        playerJson(&pl, jobj);
        aliensJson(aliens, jobj);
        bulletsJson(bullets, jobj);
        blocksJson(shields, jobj);

        char const *mensaje = json_object_to_json_string(jobj);

        ///////////////// Envia el largo |||||||||||||||||||||
        int length = strlen(mensaje);
        printf("\nlargo de de de %i \n", length);
        char messageLength[5];
        sprintf(messageLength, "%d", length);

        printf("\nlargo de men: %lu\n", strlen(messageLength));

        if (send(client.sockno, messageLength, strlen(messageLength), 0) < 0) {
            perror("Error al enviar1");
            continue;
        }

        usleep(50);
        // ---------------------------------------------------

        // Recibe provisional
        char tesc[3];
        int test = recv(client.sockno, tesc, 3, 0);

        ///////////////// Envia el mensaje Json |||||||||||||||
        printf("\nMensaje: %s \n largosent: %lu\n", mensaje, strlen(mensaje));
        if (send(client.sockno, mensaje, strlen(mensaje), 0) < 0) {
            perror("Error al enviar2");
            break;
        }
        usleep(50);
        // ----------------------------------------------------

        // Recibe provisional
        test = recv(client.sockno, tesc, 3, 0);

        SDL_Delay(50);
    }
}

void *send_to_cplayer(void *sock) {
    struct client_info client = *((struct client_info *) sock);
    char buffer[12000];
    char len[4];
    int largo;

    while (1) {
        printf("\nhola");
        if (!isObserver)
            updateServer(&pl, aliens, bullets, shields, renderer, NULL);
        //draw(renderer, pl, aliens, bullets, shields, NULL);
        //done = eventPoll(pl, 0, bullets, renderer);

        ///////////////// Crea el mensaje |||||||||||||||||||||||||

        json_object *jobj = json_object_new_object();
        stateJson(lose, jobj);
        gameInfoJson(&game, jobj);
        typeClient(0, jobj);
        playerJson(&pl, jobj);
        aliensJson(aliens, jobj);
        bulletsJson(bullets, jobj);
        blocksJson(shields, jobj);

        // --------------------------------------------------------

        char const *mensaje = json_object_to_json_string(jobj);

        ///////////////// Envia el largo |||||||||||||||||||||
        int length = strlen(mensaje);
        printf("\nlargo de de de %i \n", length);
        char messageLength[5];
        sprintf(messageLength, "%d", length);

        printf("\nlargo de men: %lu\n", strlen(messageLength));

        if (send(client.sockno, messageLength, strlen(messageLength), 0) < 0) {
            perror("Error al enviar1");
            continue;
        }

//        send(client.sockno, messageLength, strlen(messageLength), 0);
        usleep(50);
        // ---------------------------------------------------

        // Recibe provisional
        char tesc[3];
        int test = recv(client.sockno, tesc, 3, 0);

        ///////////////// Envia el mensaje Json |||||||||||||||
        printf("\nMensaje: %s \n largosent: %lu\n", mensaje, strlen(mensaje));
        if (send(client.sockno, mensaje, strlen(mensaje), 0) < 0) {
            perror("Error al enviar2");
            break;
        }
        usleep(50);
//        send(client.sockno,mensaje,strlen(mensaje),0);

        // ----------------------------------------------------

        ///////////////// Recibe el largo |||||||||||||||||||||
        int bytesRecibidos = recv(client.sockno, len, 4, 0);

        if (bytesRecibidos < 0) {
            perror("Pus se deconecto we server");
        }

        len[bytesRecibidos] = '\0';
        printf("\n len: %s\n", len);

//        if (strcmp(len, quit) > 0) {
//            printf("ESC pressed!");
//            break;
//        }
        //printf("Me llegaron %d bytes con el mensaje: %s \n", bytesRecibidos, len);

        largo = atoi(len);
        printf("largo: %d\n",largo);
        usleep(50);
        // ---------------------------------------------------

        //Envia provicional
        char test2[3] = "23";
        send(client.sockno, test2, 3, 0);

        ///////////////// Recibe el mensaje |||||||||||||||||||||||||
        bytesRecibidos = recv(client.sockno, buffer, largo, 0);
        if (bytesRecibidos < 0) {
            perror("Pus se deconecto we cliente1");
        }
//        buffer[bytesRecibidos] = '\0';
//        if (strcmp(len, quit) > 0) {
//            printf("ESC pressed!");
//            break;
//        }
        //printf("Me llegaron %d bytes con el mensaje: %s \n", bytesRecibidos, buffer);
        usleep(50);
        // ----------------------------------------------------------

        if (!isObserver)
            updatePlayer(&pl, bullets, buffer, &game);

        SDL_Delay(50);
    }

    pthread_mutex_lock(&mutex);
    printf("%s disconnected\n", client.ip);
    for (int i = 0; i < length(clients); ++i) {
        int tmp = *(int *) get(clients, i);

        if (tmp == client.sockno) {
            delete_node(clients, i, "");
        }
    }
    pthread_mutex_unlock(&mutex);
}

int main(int argc, char* args[]) {
    SDL_Renderer *renderer = init();

    SDL_Surface *sheet = loadImage("../../resources/Ship.png");
    Player player = {394, 645, 56, 70, 2, 0, 0,
                     SDL_CreateTextureFromSurface(renderer, sheet)};
    SDL_FreeSurface(sheet);

    pl = player;

    isServer = true;
    isObserver = 0;

    game = (struct game_info) {3, 0};
    aliens = (struct LinkedList *) malloc(sizeof(struct LinkedList));
    getAliens(renderer, aliens);

    bullets = (struct LinkedList *) malloc(sizeof(struct LinkedList));
    createList(bullets, sizeof(struct Bullet *), free_bullet);

    shields = (struct LinkedList *) malloc(sizeof(struct LinkedList));
    createList(shields, sizeof(struct Block *), free_block);
    generateShields(shields);

    signal(SIGPIPE, handler);

    serverConnected = true;
    struct sockaddr_in my_addr,their_addr;
    int my_sock;
    int their_sock;
    socklen_t their_addr_size;
    pthread_t sendt,play;
    struct client_info cl;

    //truepthread_create(&play,NULL,runGame,&cl);
    clients = (struct LinkedList *) malloc(sizeof(struct LinkedList));
    createList(clients, sizeof(int), NULL);

    char ip[INET_ADDRSTRLEN];

    my_sock = openSocket();
    memset(my_addr.sin_zero,'\0',sizeof(my_addr.sin_zero));
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(puerto);
    my_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    their_addr_size = sizeof(their_addr);

    if(bind(my_sock,(struct sockaddr *)&my_addr,sizeof(my_addr)) != 0) {
        perror("binding unsuccessful");
        exit(1);
    }

    if(listen(my_sock,5) != 0) {
        perror("listening unsuccessful");
        exit(1);
    }

    while(1) {
        printf("HOLO HOLO HOLO");
        if((their_sock = accept(my_sock,(struct sockaddr *)&their_addr,&their_addr_size)) < 0) {
            perror("accept unsuccessful");
            exit(1);
        }
        pthread_mutex_lock(&mutex);
        inet_ntop(AF_INET, (struct sockaddr *)&their_addr, ip, INET_ADDRSTRLEN);
        printf("%s connected\n",ip);
        cl.sockno = their_sock;
        strcpy(cl.ip,ip);
        add(clients, &their_sock);
        n++;

        char opcode[2];
        int bytesRecibidos = recv(cl.sockno, opcode, 2, 0);
        opcode[bytesRecibidos] = '\0';
        int op = atoi(opcode);

        printf("Codigo: %i", op);
        if (op == 2) {
            pthread_create(&sendt, NULL, send_to_cplayer, &cl);
        }
        else {
            pthread_create(&sendt, NULL, send_to_observer, &cl);
        }

        pthread_mutex_unlock(&mutex);
    }

    for (int i = 0; i < 4; ++i) {
        struct Block *tmp = *(struct Block **) get(shields, i);
        list_destroy(tmp->shield);
    }
    list_destroy(bullets);
    list_destroy(aliens);
    list_destroy(shields);
    SDL_DestroyRenderer(renderer);
    SDL_Quit();

    return 0;
}
