//
// Created by Loïc on 16/12/2021.
//

#ifndef COULEURS
#define COULEURS

#include <stdio.h>

#define clrscr() printf("\033[H\033[2J")

#define couleur(param) printf("\033[%sm",param)

#endif
