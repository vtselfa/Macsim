#ifndef RANDOM_H
#define RANDOM_H

double macsim_random(int stream);
long macsim_stream_value(int stream);
void macsim_seed(long seed, int stream); 

#endif /* RANDOM_H */
