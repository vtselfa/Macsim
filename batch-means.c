#include "batch-means.h" 

/*****************************************************************************/
//   Bloque de análisis de la salida de la simulación.
//   Realiza el cálculo de intervalos de confianza por batch means.
//   Alarga la simulación hasta alcanzar la precisión solicitada.
//   Extraído y adaptado de la biblioteca SMPL de  M.H. MacDougall.
//   Algoritmo descrito en la página 118 del libro
//   "Simulating Computer Systems.Techniques and tools". The MIT Press. 1987
// 

static long transitorio,num_batches,batch,obs;
static double suma,gransuma,gransuma2,granmedia,h,precision, nivel;

/*--------  COMPUTE pth QUANTILE OF THE NORMAL DISTRIBUTION  ---------*/
double Z(double p)
    { /* This function computes the pth upper quantile of the stand-  */
      /* ard normal distribution (i.e., the value of z for which the  */
      /* are under the curve from z to +infinity is equal to p).  'Z' */
      /* is a transliteration of the 'STDZ' function in Appendix C of */
      /* "Principles of Discrete Event Simulation", G. S. Fishman,    */
      /* Wiley, 1978.   The  approximation used initially appeared in */
      /* in  "Approximations for Digital Computers", C. Hastings, Jr.,*/
      /* Princeton U. Press, 1955. */
      double q,z1,n,d; 
      double sqrt(),log();
      q=(p>0.5)? (1-p):p; z1=sqrt(-2.0*log(q));
      n=(0.010328*z1+0.802853)*z1+2.515517;
      d=((0.001308*z1+0.189269)*z1+1.43278)*z1+1;
      z1-=n/d; 
      if (p>0.5) z1=-z1;
      return(z1);
    }

/*-----------  COMPUTE pth QUANTILE OF THE t DISTRIBUTION  -----------*/
double T(double p,int ndf)
    { /* This function computes the upper pth quantile of the t dis-  */
      /* tribution (the value of t for which the area under the curve */
      /* from t to +infinity is equal to p).  It is a transliteration */
      /* of the 'STUDTP' function given in Appendix C of  "Principles */
      /* of Discrete Event Simulation", G. S. Fishman, Wiley, 1978.   */
      int i; 
      double z1,z2,h[4],x=0.0; 
      double fabs();
      z1=fabs(Z(p)); 
      z2=z1*z1;
      h[0]=0.25*z1*(z2+1.0); 
      h[1]=0.010416667*z1*((5.0*z2+16.0)*z2+3.0);
      h[2]=0.002604167*z1*(((3.0*z2+19.0)*z2+17.0)*z2-15.0);
      h[3]=0.000010851*z1*((((79.0*z2+776.0)*z2+1482.0)*z2-1920.0)*z2-945.0);
      for (i=3; i>=0; i--) x=(x+h[i])/(double)ndf;
      z1+=x; 
      if (p>0.5) z1=-z1;
      return(z1);
    }

/*****************************************************************************/
//    define los párametros del método batch means
//       observaciones del transitorio (no se tienen en cuenta)
//       tamaño de los lotes
//       precisión (semintervalo/media)
//       nivel de confianza
void batch_mean(long obs_trans,long tam_batch, double precis, double nivelconf)
{
  transitorio=obs_trans; // núm. de observaciones del transitorio
  batch=tam_batch;       // núm. de observaciones por lote
  precision=precis;      // precisión a alcanzar
  nivel=nivelconf;       // nivel de confianza
  suma=gransuma=gransuma2=0.0;// inicializar variables
  num_batches=obs=0;
}

/*****************************************************************************/

int observacion(double valor)
{
  int r=0; 
  double var;
  double media_batch;

  if (transitorio) {
    transitorio--; 
    return(r);
  }
  suma+=valor; 
  obs++;
  if (obs==batch) {             // batch completado
    media_batch=suma/obs;       // media del batch
    gransuma+=media_batch;
    gransuma2+=media_batch*media_batch; 
    num_batches++;
    fprintf(stderr,"media batch num. %2ld = %.3f",num_batches,media_batch);
    suma=0.0;
    obs=0;
    if (num_batches>=10) {    // calcula, al menos, 10 batches
      granmedia=gransuma/num_batches;    // gran media
      var=(gransuma2-num_batches*granmedia*granmedia)/(num_batches-1);    //varianza muestral
      h=T((1-nivel)/2.0,num_batches-1)*sqrt(var/num_batches);
//      h=T(0.025,num_batches-1)*sqrt(var/num_batches);
      fprintf(stderr,", rel. HW = %.3f",h/granmedia);
      if (h/granmedia<=precision) r=1;
    }
    fprintf(stderr,"\n");
  }
  return(r);
}

/*****************************************************************************/

void resultado(double *media, double *semi_intervalo, int *num_b)
{
  *media=granmedia;
  *semi_intervalo=h;
  *num_b=num_batches;
}
