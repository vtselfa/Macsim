/*****************************************************************************/
//    Generacion de variables aleatorias
//    ==================================
//
//    Adaptado del programa simlib.c de A.M. Law y W.D. Kelton
//
//    Generador congruencial lineal multiplicativo de modulo primo
//
//           x_n = (630360016 * x_{n-1}) mod (2^31-1)
//
//    basado en la implementacion de Marse y Roberts (1983)
//    Se admiten 101 streams, con semillas separadas por 1000000
//    numeros.
/*****************************************************************************/

/* Define constantes del generador */

static const long MODULO = 2147483647;
static const long MULT1 = 24112;
static const long MULT2 = 26143;

/* Semillas de los 101 streams */

static long inicial[] =
{         1,
 1973272912, 281629770,  20006270,1280689831,2096730329,1933576050,
  913566091, 246780520,1363774876, 604901985,1511192140,1259851944,
  824064364, 150493284, 242708531,  75253171,1964472944,1202299975,
  233217322,1911216000, 726370533, 403498145, 993232223,1103205531,
  762430696,1922803170,1385516923,  76271663, 413682397, 726466604,
  336157058,1432650381,1120463904, 595778810, 877722890,1046574445,
   68911991,2088367019, 748545416, 622401386,2122378830, 640690903,
 1774806513,2132545692,2079249579,  78130110, 852776735,1187867272,
 1351423507,1645973084,1997049139, 922510944,2045512870, 898585771,
  243649545,1004818771, 773686062, 403188473, 372279877,1901633463,
  498067494,2087759558, 493157915, 597104727,1530940798,1814496276,
  536444882,1663153658, 855503735,  67784357,1432404475, 619691088,
  119025595, 880802310, 176192644,1116780070, 277854671,1366580350,
 1142483975,2026948561,1053920743, 786262391,1792203830,1494667770,
 1923011392,1433700034,1244184613,1147297105, 539712780,1545929719,
  190641742,1645390429, 264907697, 620389253,1502074852, 927711160,
  364849192,2049576050, 638580085, 547070247 };

/*****************************************************************************/
//   Generador congruencial lineal multiplicativo de modulo primo
//   Genera el siguiente numero aleatorio U(1,0)
//   [LawKelton2000, pag. 430]

double macsim_random(int stream)
{
    long zi, lowprd, hi31;

    zi     = inicial[stream];
    lowprd = (zi & 65535) * MULT1;
    hi31   = (zi >> 16) * MULT1 + (lowprd >> 16);
    zi     = ((lowprd & 65535) - MODULO) +
             ((hi31 & 32767) << 16) + (hi31 >> 15);
    if (zi < 0) zi += MODULO;
    lowprd = (zi & 65535) * MULT2;
    hi31   = (zi >> 16) * MULT2 + (lowprd >> 16);
    zi     = ((lowprd & 65535) - MODULO) +
             ((hi31 & 32767) << 16) + (hi31 >> 15);
    if (zi < 0) zi += MODULO;
    inicial[stream] = zi;
    return((zi >> 7 | 1) / 16777216.0);
}

/*****************************************************************************/
//   Cambia la semilla de un stream
void macsim_seed(long seed, int stream) 
{
    inicial[stream] = seed;
}

/*****************************************************************************/
//   Devuelve el valor actual de un stream
long  macsim_stream_value(int stream)
{
    return inicial[stream];
}
