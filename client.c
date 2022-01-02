//
// Created by Loïc on 07/10/2021.
//

#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <string.h>

#include "couleur.h"

struct pseudonyme //Definir type à envoyer avant
{
    char pseudonyme[255];
}__attribute__((packed));
struct pseudonyme initialisation;

struct question //Definir type à envoyer avant
{
    u_int8_t numQuestion;
    char question[255];
    char answer1[255], answer2[255], answer3[255], answer4[255];
}__attribute__((packed));
struct question question;

struct rules //Definir type à envoyer avant
{
    char textRule[1000];
}__attribute__((packed));

struct answer
{
    u_int8_t numQuestion;
    char answer[255];
};
struct answer answer;

struct resultat
{
    u_int8_t numQuestion;
    bool res;
};

struct donneeClient
{
    char pseudonyme[255];
    int point;
};

struct ranking
{
    struct donneeClient donneeClient[3];
    bool allocated[3];
}__attribute__((packed));

bool cleared = true;

bool sendPacket(long int *bufferSize, int skt, uint8_t type)
{
    if (type == 1)
    {
        if ((*bufferSize = send(skt, (char *)&type, sizeof(type), 0)) < 0)
        {
            return true;
        }

        if ((*bufferSize += send(skt, (char*)&initialisation, sizeof(initialisation), 0)) < 0)
        {
            return true;
        }
    }

    if (type == 2)
    {
        if ((*bufferSize = send(skt, (char *)&type, sizeof(type), 0)) < 0)
        {
            return true;
        }

        if ((*bufferSize += send(skt, (char*)&answer, sizeof(answer), 0)) < 0)
        {
            return true;
        }
    }

    return false;
}

bool recvPacket(long int *bufferSize, int *typeRetour, uint8_t *numeroQuestion, int skt)
{
    uint8_t type = 0;

    if ((*bufferSize = recv(skt, (char *)&type, sizeof(type), 0)) < 1)
    {
        if(*bufferSize == 0)
        {
            puts("Server disconnected");
            fflush(stdout);
            exit(EXIT_SUCCESS);
        }
        else if(*bufferSize == -1)
        {
            perror("Recv failed");
            return true;
        }
    }

    switch (type)
    {
        case 1: //Initialisation
        {
            struct rules rule;
            if ((*bufferSize = recv(skt, (char*)&rule, sizeof(rule), 0)) > 0)
            {
                //clrscr();
                couleur("31;1");
                printf("Message recu avec les règles :\n");
                couleur("0");
                couleur("31");
                puts(rule.textRule);
            }

            if ((*bufferSize = recv(skt, (char*)&question, sizeof(question), 0)) > 0)
            {
                couleur("34");
                puts("Nouvelle question :");
                printf("Question n°%d : %s\n", question.numQuestion, question.question);
                couleur("32");
                printf("    A - %s\n", question.answer1);
                couleur("33");
                printf("    B - %s\n", question.answer2);
                couleur("35");
                printf("    C - %s\n", question.answer3);
                couleur("36");
                printf("    D - %s\n", question.answer4);
                fflush(stdout);
                *numeroQuestion = question.numQuestion;
                *typeRetour = 1;
                return false;
            }
        }

        case 2: //Nouvelle question
        {
            if ((*bufferSize = recv(skt, (char*)&question, sizeof(question), 0)) > 0)
            {
                couleur("34");
                puts("Nouvelle question :");
                printf("Question n°%d : %s\n", question.numQuestion, question.question);
                couleur("32");
                printf("    A - %s\n", question.answer1);
                couleur("33");
                printf("    B - %s\n", question.answer2);
                couleur("35");
                printf("    C - %s\n", question.answer3);
                couleur("36");
                printf("    D - %s\n", question.answer4);
                fflush(stdout);
                *numeroQuestion = question.numQuestion;
                *typeRetour = 1;
                return false;
            }
        }

        case 3: //Resultat d'une question ou j'ai répondu
        {
            struct resultat resultat;
            if ((*bufferSize = recv(skt, (char*)&resultat, sizeof(resultat), 0)) > 0)
            {
                if (resultat.res)
                {
                    clrscr();
                    couleur("32");
                    printf("Réponse à la question n°%d : la réponse est bonne.\n\n", resultat.numQuestion);
                    *typeRetour = 2;
                    cleared = false;
                }
                else
                {
                    clrscr();
                    couleur("31");
                    printf("Réponse à la question n°%d : la réponse est fausse.\n\n", resultat.numQuestion);

                    couleur("34");
                    printf("Question n°%d : %s\n", question.numQuestion, question.question);
                    couleur("32");
                    printf("    A - %s\n", question.answer1);
                    couleur("33");
                    printf("    B - %s\n", question.answer2);
                    couleur("35");
                    printf("    C - %s\n", question.answer3);
                    couleur("36");
                    printf("    D - %s\n", question.answer4);

                    *typeRetour = 1;
                }
                fflush(stdout);

                return false;
            }
        }

        case 4: //Question suivante car quelqu'un a répondu
        {
            if ((*bufferSize = recv(skt, (char*)&question, sizeof(question), 0)) > 0)
            {
                couleur("34");
                printf("Question suivante n°%d : %s\n", question.numQuestion, question.question);
                couleur("32");
                printf("    A - %s\n", question.answer1);
                couleur("33");
                printf("    B - %s\n", question.answer2);
                couleur("35");
                printf("    C - %s\n", question.answer3);
                couleur("36");
                printf("    D - %s\n", question.answer4);
                fflush(stdout);
                *numeroQuestion = question.numQuestion;
                *typeRetour = 1;
                return false;
            }
        }

        case 5: //Envoie du classement actuel des 3 premiers
        {
            struct ranking ranking;
            if ((*bufferSize = recv(skt, (char*)&ranking, sizeof(ranking), 0)) > 0)
            {
                if (cleared == true)
                {
                    clrscr();
                }

                couleur("35");
                printf("Classement actuel :\n");
                for (int i = 0 ; i < 3 ; i++)
                {
                    if (ranking.allocated[i])
                    {
                        printf("    %d - %s : %d\n", (i + 1), ranking.donneeClient[i].pseudonyme, ranking.donneeClient[i].point);
                    }
                }

                cleared = true;
                return false;
            }
        }

        default:
        {
            perror("Type dont exist...");
            return true;
        }
    }
}

int main(int argc, char const *argv[])
{
    //Debut du code client

    if (argc == 2)
    {
        uint16_t port = (uint16_t) atoi(argv[1]);
        int skt;

        //Select
        fd_set rfds;
        int retval;

        //debug
        long int bufferSize = 0;

        if ((skt = socket(AF_INET, SOCK_STREAM, 0)) == -1) //
        {
            perror("Erreur socket");
            exit(EXIT_FAILURE);
        }

        struct sockaddr_in sktadrs;
        sktadrs.sin_family = AF_INET;
        sktadrs.sin_port = htons(port);

        struct in_addr address;
        address.s_addr = inet_addr("127.0.0.1");
        sktadrs.sin_addr = address;

        clrscr();
        printf("Entrez votre pseudo : ");
        scanf("%[^\n]%*c" , initialisation.pseudonyme);

        if (connect(skt, (struct sockaddr *) &sktadrs, sizeof(sktadrs)) >= 0)
        {
            uint8_t type = 1; /*entier à envoyer*/
            int typeRetour = 0; /*entier de retour*/
            uint8_t numeroQuestion = 0;

            if (sendPacket(&bufferSize, skt, type))
            {
                perror("Erreur lors de l'envoie du paquet....");
            }

            if (recvPacket(&bufferSize, &typeRetour, &numeroQuestion, skt))
            {
                perror("Erreur lors de la reception du serveur....");
            }

            while (true)
            {
                if (typeRetour == 1)
                {
                    couleur("33");
                    printf("En attente de votre réponse : \n");
                    fflush(stdout);
                    couleur("35");

                    FD_ZERO(&rfds);

                    FD_SET(0, &rfds);
                    FD_SET(skt, &rfds);

                    if ((retval = select(FD_SETSIZE, &rfds, NULL, NULL, NULL)) != 1)
                    {
                        perror("Select");
                        exit(EXIT_FAILURE);
                    }

                    if (FD_ISSET(0, &rfds))
                    {
                        //Entrée terminal

                        char *line = NULL;
                        size_t len = 0;

                        couleur("31");
                        strcpy(answer.answer, "");
                        if (getline(&line, &len, stdin) != -1)
                        {
                            strcat(answer.answer, line);
                        }

                        for (int i = 0; i < sizeof(answer.answer) ; i++)
                        {
                            if (answer.answer[i] == '\n')
                            {
                                answer.answer[i] = '\0';
                            }
                        }
                        answer.numQuestion = numeroQuestion;
                        type = 2;

                        if (sendPacket(&bufferSize, skt, type))
                        {
                            perror("Erreur lors de l'envoie du paquet....");
                        }

                        if (recvPacket(&bufferSize, &typeRetour, &numeroQuestion, skt))
                        {
                            perror("Erreur lors de la reception du serveur....");
                        }
                    }
                    else if (FD_ISSET(skt, &rfds))
                    {
                        //Message serveur
                        if (recvPacket(&bufferSize, &typeRetour, &numeroQuestion, skt))
                        {
                            perror("Erreur lors de la reception du serveur....");
                        }
                    }
                }

                if (typeRetour == 2)
                {
                    if (recvPacket(&bufferSize, &typeRetour, &numeroQuestion, skt))
                    {
                        perror("Erreur lors de la reception du serveur....");
                    }
                }
            }
        }
        else
        {
            perror("Erreur de connexion....");
        }
    }
    else
    {
        perror("Forme de la commande: ./Client <Port>");
    }
}
