/* 
 * File:   main.c
 * Author: andre
 *
 * Created on February 17, 2019, 11:25 PM
 */

#include <stdio.h>
#include <stdlib.h>
#include "../metaEngine.c"

void ontick(double *tick){
    int time = (int) tick[0];
    double price = tick[1];

    if(time < 1388657220)
        sendOrder(Order_Kind_Buy, price, 3, -1, -1, 10, -1, Order_Source_Client);
}


/*
 * 
 */
int main(int argc, char** argv) {
    double ticks[] = {
        1388657160,        80088,          563,          160,
        1388657211,        80049,          563,          160,
        1388657217,        80118,          563,          160,
        1388657220,        80080,          223,           61};
    double pmoney[4];
    
    Simulator(5000., 0.2, 4.0, ticks, 4, pmoney, ontick);
    return (EXIT_SUCCESS);
}

