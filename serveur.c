//
// Created by Loïc on 07/10/2021.
//

#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define MAX_CLIENT_COUNT 10
#define MAX_QUESTION 5
u_int8_t QUESTION_ACTUEL = 1;

struct pseudonyme //Definir type à envoyer avant
{
    char pseudonyme[255];
}__attribute__((packed));

struct question //Definir type à envoyer avant
{
    u_int8_t numQuestion;
    char question[255];
    char answer1[255], answer2[255], answer3[255], answer4[255];
}__attribute__((packed));
struct question question[10];

struct regles //Definir type à envoyer avant
{
    char texteRegle[1000];
}__attribute__((packed));
struct regles regle;

struct reponse
{
    u_int8_t numQuestion;
    char reponse[255];
};
struct reponse reponses[10];

struct donneeClient
{
    char pseudonyme[255];
    int point;
};

struct client
{
    int clientSocket[MAX_CLIENT_COUNT];
    bool allocated[MAX_CLIENT_COUNT];
    struct donneeClient clientInfo[MAX_CLIENT_COUNT];
    int alloue;
};
struct client donneeClient;

struct resultat
{
    u_int8_t numQuestion;
    bool res;
};
struct resultat resultat;

struct ranking
{
    struct donneeClient donneeClient[3];
    bool allocated[3];
}__attribute__((packed));
struct ranking generalRanking;

bool getClient(struct donneeClient **client, int skt)
{
    for (int i = 0 ; i < MAX_CLIENT_COUNT ; i++)
    {
        if (donneeClient.clientSocket[i] == skt)
        {
            *client = &donneeClient.clientInfo[i];
            return false;
        }
    }

    perror("Client dont exist...");
    return true;
}

bool sendPacket(long int *bufferSize, int skt, uint8_t type);
bool recvPacket(long int *bufferSize, struct client *client, int skt, struct donneeClient *clientInfo);

bool newClient(struct client *client, int skt)
{
    for (int i = 0 ; i < MAX_CLIENT_COUNT ; ++i)
    {
        if (client->allocated[i] == false)
        {
            client->clientSocket[i] = skt;
            client->allocated[i] = true;
            client->alloue++;
            return false;
        }
    }

    return true;
}

bool killClient(struct client *client, int skt)
{
    for (int i = 0 ; i < MAX_CLIENT_COUNT ; ++i)
    {
        if (client->clientSocket[i] == skt)
        {
            client->clientSocket[i] = 0;
            client->allocated[i] = false;
            client->alloue--;
            return false;
        }
    }

    return true;
}

void defineRanking(struct ranking *ranking)
{
    int max = -1;
    int pos_pass[3] = {-1, -1, -1};
    int pos_max = 0;

    for (int i = 0 ; i < 3 ; i++)
    {
        ranking->allocated[i] = false;
    }

    for (int i = 0 ; i < donneeClient.alloue ; i++)
    {
        if (i == 3 || donneeClient.alloue == i)
        {
            break;
        }

        for (int j = 0 ; j < MAX_CLIENT_COUNT ; j++)
        {
            if (donneeClient.allocated[j] == true && pos_pass[0] != j && pos_pass[1] != j && pos_pass[2] != j)
            {
                if (max < donneeClient.clientInfo[j].point)
                {
                    pos_max = j;
                    max = donneeClient.clientInfo[j].point;
                }
            }
        }

        pos_pass[i] = pos_max;
        ranking->donneeClient[i] = donneeClient.clientInfo[pos_max];
        ranking->allocated[i] = true;
        max = -1;
    }
}

bool sendQuestionAll()
{
    long int bufferSize;
    uint8_t type;
    if (QUESTION_ACTUEL == MAX_QUESTION)
    {
        QUESTION_ACTUEL = 1;
    }
    else
    {
        QUESTION_ACTUEL++;
    }

    defineRanking(&generalRanking);

    for (int i = 0 ; i < MAX_CLIENT_COUNT ; i++)
    {
        if (donneeClient.allocated[i] == true)
        {
            type = 5;
            if (sendPacket(&bufferSize, donneeClient.clientSocket[i], type))
            {
                perror("Send ranking...");
                return true;
            }

            type = 4;
            if (sendPacket(&bufferSize, donneeClient.clientSocket[i], type))
            {
                perror("ReSend question to all...");
                return true;
            }
        }
    }

    return false;
}

bool sendPacket(long int *bufferSize, int skt, uint8_t type)
{
    if (type == 1) //Initialisation
    {
        if ((*bufferSize = send(skt, (char *)&type, sizeof(type), 0)) < 0)
        {
            return true;
        }

        if ((*bufferSize = send(skt, (char*)&regle, sizeof(regle), 0)) < 0)
        {
            return true;
        }

        if ((*bufferSize = send(skt, (char*)&question[QUESTION_ACTUEL - 1], sizeof(question[QUESTION_ACTUEL - 1]), 0)) < 0)
        {
            return true;
        }
    }

    if (type == 2 || type == 4) // Envoie de la question actuel
    {
        if ((*bufferSize = send(skt, (char *)&type, sizeof(type), 0)) < 0)
        {
            return true;
        }

        if ((*bufferSize = send(skt, (char*)&question[QUESTION_ACTUEL - 1], sizeof(question[QUESTION_ACTUEL - 1]), 0)) < 0)
        {
            return true;
        }
    }

    if (type == 3) // Résultat d'une réponse envoyé
    {
        if ((*bufferSize = send(skt, (char *)&type, sizeof(type), 0)) < 0)
        {
            return true;
        }

        if ((*bufferSize = send(skt, (char*)&resultat, sizeof(resultat), 0)) < 0)
        {
            return true;
        }
    }

    if (type == 5) // Envoie du classement
    {
        if ((*bufferSize = send(skt, (char *)&type, sizeof(type), 0)) < 0)
        {
            return true;
        }

        if ((*bufferSize = send(skt, (char*)&generalRanking, sizeof(generalRanking), 0)) < 0)
        {
            return true;
        }
    }

    return false;
}

bool recvPacket(long int *bufferSize, struct client *client, int skt, struct donneeClient *clientInfo)
{
    uint8_t type = 0;

    if ((*bufferSize = recv(skt, (char *)&type, sizeof(type), 0)) < 1)
    {
        if(*bufferSize == 0)
        {
            puts("Client disconnected");
            fflush(stdout);
            return killClient(client, skt);
        }
        else if(*bufferSize == -1)
        {
            perror("recv failed");
            return true;
        }
    }

    printf("Chaine envoyée : %d\n", type);

    switch (type)
    {
        case 1: //Initialisation
        {
            struct pseudonyme pseudonyme;
            if ((*bufferSize = recv(skt, (char*)&pseudonyme, sizeof(pseudonyme), 0)) > 0)
            {
                puts("Message recu avec pseudonyme");
                strcpy(clientInfo->pseudonyme, pseudonyme.pseudonyme);
                clientInfo->point = 0;
                puts(clientInfo->pseudonyme);

                long int buffer = 0;
                if (sendPacket(&buffer, skt, 1))
                {
                    perror("Error send type 1...");
                    return true;
                }

                puts("Reponse regles");
            }
            return false;
        }

        case 2: //Réponse du client à une question
        {
            struct reponse reponse;
            if ((*bufferSize = recv(skt, (char*)&reponse, sizeof(reponse), 0)) > 0)
            {
                struct donneeClient * clientD = NULL;
                if (getClient(&clientD, skt))
                {
                    perror("Client dont exist");
                    return true;
                }

                if (clientD == NULL)
                {
                    return true;
                }

                printf("Réponse %s à la question n°%d : %s\n", clientD->pseudonyme, reponse.numQuestion, reponse.reponse);

                if (QUESTION_ACTUEL == reponse.numQuestion && strcmp(reponse.reponse, reponses[QUESTION_ACTUEL - 1].reponse) == 0)
                {
                    resultat.numQuestion = reponse.numQuestion;
                    resultat.res = true;
                    clientD->point++;
                }
                else if (QUESTION_ACTUEL == reponse.numQuestion && strcmp(reponse.reponse, reponses[QUESTION_ACTUEL - 1].reponse) != 0)
                {
                    resultat.numQuestion = reponse.numQuestion;
                    resultat.res = false;
                }

                long int buffer = 0;
                if (sendPacket(&buffer, skt, 3))
                {
                    perror("Error send type 3...");
                    return true;
                }

                if (resultat.res == true)
                {
                    if (sendQuestionAll())
                    {
                        perror("Error resend question to all client...");
                        return true;
                    }
                }

                return false;
            }
        }
    }

    return true;
}

bool setRFDS(fd_set * rfds, int sktServeur)
{
    FD_ZERO(rfds);
    FD_SET(sktServeur, rfds);

    for (int i = 0 ; i < MAX_CLIENT_COUNT ; i++)
    {
        if (donneeClient.allocated[i] == true)
        {
            FD_SET(donneeClient.clientSocket[i], rfds);
            //Vérifier bon fonctionnement FD_SET, pas trouvé la valeur de retour FD_SET.
        }
    }

    return false;
}

void initOfData()
{
    //Définition regle
    strcpy(regle.texteRegle, "Bonjour, bienvenue dans question pour un pigeon.\n\n"
                             "Votre but aujourd'hui est de répondre en premier au question qui vont apparaitre à votre écran.\n"
                             "Attention, pour répondre à la question vous devez écrire la réponse entièrement et sans faute.\n"
                             "Bonne chance à vous et que la connaissance vous sois favorable !\n");

    //Définition question
    question[0].numQuestion = 1;
    strcpy(question[0].question, "Quel est la capital de la France?");
    strcpy(question[0].answer1, "Berlin");
    strcpy(question[0].answer2, "Paris");
    strcpy(question[0].answer3, "Dublin");
    strcpy(question[0].answer4, "Londre");
    reponses[0].numQuestion = 1;
    strcpy(reponses[0].reponse, "Paris");

    question[1].numQuestion = 2;
    strcpy(question[1].question, "Quel est la capital de l'Irlande?");
    strcpy(question[1].answer1, "Berlin");
    strcpy(question[1].answer2, "Paris");
    strcpy(question[1].answer3, "Dublin");
    strcpy(question[1].answer4, "Tokyo");
    reponses[1].numQuestion = 2;
    strcpy(reponses[1].reponse, "Dublin");

    question[2].numQuestion = 3;
    strcpy(question[2].question, "Quel est la production moyenne de déchets en France chaque année?");
    strcpy(question[2].answer1, "34 000 m3");
    strcpy(question[2].answer2, "21 000 m3");
    strcpy(question[2].answer3, "13 500 m3");
    strcpy(question[2].answer4, "8 000 m3");
    reponses[2].numQuestion = 3;
    strcpy(reponses[2].reponse, "13 500 m3");

    question[3].numQuestion = 4;
    strcpy(question[3].question, "Quel est la vitesse de mach 1?");
    strcpy(question[3].answer1, "1234.8 Km/h");
    strcpy(question[3].answer2, "967.4 Km/h");
    strcpy(question[3].answer3, "1678.5 Km/h");
    strcpy(question[3].answer4, "2465.8 Km/h");
    reponses[3].numQuestion = 4;
    strcpy(reponses[3].reponse, "1234.8 Km/h");

    question[4].numQuestion = 5;
    strcpy(question[4].question, "Quel est la vitesse d'une fusée pour aller dans l'espace?");
    strcpy(question[4].answer1, "8 400 Km/h");
    strcpy(question[4].answer2, "4 500 Km/h");
    strcpy(question[4].answer3, "5 400 Km/h");
    strcpy(question[4].answer4, "6 500 Km/h");
    reponses[4].numQuestion = 5;
    strcpy(reponses[4].reponse, "5 400 Km/h");
}

int main(int argc, char const *argv[])
{
    //Debut code serveur

    initOfData();

    int server_socket, new_socket, c;
    long int bufferSize;
    struct sockaddr_in address, client;

    fd_set rfds;

    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) //
    {
        perror("Error socket...");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(6666);

    if (bind(server_socket, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("Bind failed...");
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, MAX_CLIENT_COUNT) < 0)
    {
        perror("Error listen...");
        exit(EXIT_FAILURE);
    }

    for (int i = 0 ; i < MAX_CLIENT_COUNT ; ++i)
    {
        donneeClient.clientSocket[i] = 0;
        donneeClient.allocated[i] = false;
    }

    c = sizeof(struct sockaddr_in);

    while (true)
    {
        setRFDS(&rfds, server_socket);

        if (select(FD_SETSIZE, &rfds, NULL, NULL, NULL) != 1)
        {
            perror("Error select...");
            exit(EXIT_FAILURE);
        }

        for (int i = 0 ; i < MAX_CLIENT_COUNT ; ++i)
        {
            printf("i: %d, %s, %d\n", i, donneeClient.allocated[i] ? "true" : "false", donneeClient.clientSocket[i]);
            fflush(stdout);
        }

        if (FD_ISSET(server_socket, &rfds))
        {
            while ((new_socket = accept(server_socket, (struct sockaddr *)&client, (socklen_t *)&c)) > 0)
            {
                puts("Connection accepted");
                break;
            }

            printf("SOCKET : %d\n", new_socket);
            fflush(stdout);

            if (newClient(&donneeClient, new_socket))
            {
                perror("Error New Client...");
                exit(EXIT_FAILURE);
            }

            for (int i = 0 ; i < MAX_CLIENT_COUNT ; ++i)
            {
                if (donneeClient.clientSocket[i] == new_socket && donneeClient.allocated[i] == true)
                {
                    if (recvPacket(&bufferSize, &donneeClient, new_socket, &donneeClient.clientInfo[i]))
                    {
                        perror("Erreur lors de la reception d'un paquet client....");
                        exit(EXIT_FAILURE);
                    }
                }
            }
        }
        else
        {
            for (int i = 0 ; i < MAX_CLIENT_COUNT ; ++i)
            {
                if (donneeClient.allocated[i] == true && FD_ISSET(donneeClient.clientSocket[i], &rfds))
                {
                    if (recvPacket(&bufferSize, &donneeClient, donneeClient.clientSocket[i], &donneeClient.clientInfo[i]))
                    {
                        perror("Erreur lors de la reception d'un paquet client....");
                        exit(EXIT_FAILURE);
                    }

                    break;
                }
            }
        }

        printf("%ld\n", bufferSize);

        for (int i = 0 ; i < MAX_CLIENT_COUNT ; i++)
        {
            printf("pseudonyme : %s : %d\n", donneeClient.clientInfo[i].pseudonyme, donneeClient.clientInfo[i].point);
        }
    }
}