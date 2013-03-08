#ifndef BATCH_MEANS_H 
#define BATCH_MEANS_H 

#include <stdio.h>
#include <math.h>

void batch_mean(long obs_trans,long tam_batch, double precis, double nivelconf);
int observacion(double valor);
void resultado(double *media, double *semi_intervalo, int *num_b);

#endif /* BATCH_MEANS_H */

