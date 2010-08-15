/* filter.c */
/* contains all filter creation and processing code */

#include <stdlib.h>
#include <complex.h>
#include <math.h>
#include "filter.h"
#include "sdr.h"

static complex *impulse;
static complex *samples;
static double *buf_I;
static double *buf_Q;
static double *imp_I;
static double *imp_Q;
static int index;
static int size;
static int taps;

static void make_impulse(complex fir_imp[], float sample_rate, int taps, float bw, float centre) {

    float K = bw * taps / sample_rate;
    float w;
    complex z;
    int k, i=0;

    float tune = 2.0 * M_PI * centre / sample_rate;
    
    for (k=-taps/2; k<=taps/2; k++) {
        if (k==0) z=(float)K/taps;
        else z=1.0/taps*sin(M_PI*k*K/taps)/sin(M_PI*k/taps);
        // apply a windowing function.  I can't hear any difference...
        w = 0.5 + 0.5 * cos(2.0 * M_PI * k / taps); // Hanning window
        //w = 0.42 + 0.5 * cos(2.0 * M_PI * k / N) + 0.08 * cos(4. * M_PI * k / N); // Blackman window
        //w=1; // No window
        z *= w; 
        z *= 2*cexp(-1*I * tune * k);
        fir_imp[i] = z;
        i++;
    }
}

void *filter_fir_new(int taps_i, int size_i) {
    size = size_i;
    taps = taps_i;
    impulse = malloc(sizeof(complex)*taps);
    imp_I = malloc(sizeof(double)*taps);
    imp_Q = malloc(sizeof(double)*taps);
    buf_I = malloc(sizeof(double)*taps);
    buf_Q = malloc(sizeof(double)*taps);


    index = 0;

}

void filter_fir_destroy() {

        if (impulse) free(impulse);
                printf("2\n");
        if (imp_I) free(imp_I);
                printf("3\n");
        if (imp_Q) free(imp_Q);
                printf("4\n");
        if (buf_I) free(buf_I);
                printf("5\n");
        if (buf_Q) free(buf_Q);
                printf("6\n");
}

void filter_fir_set_response(int sample_rate, float bw, float centre) {
    // plop an impulse into the appropriate array
    int i;
    make_impulse(impulse, sample_rate, taps, bw, centre);
    
    for (i=0; i<taps; i++) {
       imp_I[i] = creal(impulse[i]);
       imp_Q[i] = cimag(impulse[i]);
    } 
}

void filter_fir_process(SDRData *sdr) {
    int i, j, k;
    complex c;
    double accI, accQ;
    complex out[size];
        
    for (i = 0; i < size; i++) {
        c = sdr->iqSample[i];
        buf_I[index] = creal(c)+(0.000000001*((float)rand()/RAND_MAX-0.5));
    	buf_Q[index] = cimag(c)+(0.000000001*((float)rand()/RAND_MAX-0.5));
        accI = accQ = 0;
        j = index;
    	for (k = 0; k < taps; k++) {
            accI += buf_I[j] * imp_I[k];
            accQ += buf_Q[j] * imp_Q[k];
            if (++j >= taps) j = 0;
        }
        out[i] = accI + I * accQ;
        if (++index >= taps) index = 0;
    }
    for(i=0; i<size; i++) {
        sdr->iqSample[i] = out[i];
    }
}

/* scratchpad below here */
/*---------------------------------------------------------*/

#if 0

void fft_impulse() {
    // at this point, fir_imp[] contains our desired impulse.  Now to FFT it ;-)

#if 0
    /* this bit should be broken out to a function only called if the FFT size changes */
    fir_len = N;
    // Compute FFT and data block lengths.
    fft_len = MAX_FIR_LEN;
    blk_len = 128; // Min block length.
    n = 0;
    while ((int) pow(2.0,(double) n) < MAX_FIR_LEN) {
    	if  ((int) pow(2.0, (double) n) >= fir_len + blk_len + 1) {
            fft_len = (int) pow(2, (double) n);
            break;
        }   
	    n++;
    }

    blk_len = fft_len + 1 - fir_len;

    // Check that blk_len is smaller than fragment size (=length/4).
    if (length/4 < blk_len) { 
        blk_len = length/4;
    }
    
    // okay, so we know how big to make the FFT blocks
    if (fwd) fftw_destroy_plan(fwd);
    if (bwd) fftw_destroy_plan(bwd);
    if (imp) fftw_destroy_plan(imp);
    fwd = fftw_plan_dft_1d(fft_len, fir_in, fir_fft, FFTW_FORWARD, FFTW_ESTIMATE);
    bwd = fftw_plan_dft_1d(fft_len, fir_fft, fir_in, FFTW_BACKWARD, FFTW_ESTIMATE); 
    imp = fftw_plan_dft_1d(fft_len, fir_imp, fir_imp_fft, FFTW_FORWARD, FFTW_ESTIMATE); 
    /* right up to here */
    
    // now we can FFT the impulse
    fftw_execute(imp);   
    
#endif
}

void fft_filter() {
    // FIXME - check if this is ever needed
    // Check if in-buffer is shorter than blk_len.
    if (length/4 < blk_len) { 
        blk_len = length/4;
    }
    
    // Process a new data block.
    for (blk_pos = 0; blk_pos<length; blk_pos += blk_len) {
  
    // Read in a new data block from input stream.
        for (i=0; i < fft_len; i++) {
            if (blk_pos+i < length  && i < blk_len ) {
            	fir_in[i] = (double) sdr->iqSample[blk_pos + i]; // Get a new left channel sample.
            }
            else { 
	            fir_in[i] = 0.0;
	        } // if
        } // for

    // DFT, apply filter, IDFT
    fftw_execute(fwd);
    for (i=0; i<fft_len; i++) {
        fir_fft[i] *= fir_imp_fft[i]) / (double) fft_len;
    }
    fftw_execute(bwd);
    

    // Overlap-and-Add
    if ( (length-blk_pos) >= blk_len) // Have blk_len samples in buffer.
      k = blk_len;
    else		  // Have less than blk_len samples in buffer.
      k = (length-blk_pos);
    
    // Add the last block's overlap to the current one.
    for (i = 0; i < fir_len-1; i++) {
      fir_in[i] += fir_overlap[i];
    }

    // Save the samples that will overlap to the next block.
    for (i = k; i < k + fir_len; i++) {
        fir_overlap[i-k] = fir_in[i];
    }

  
    for (i=0; i<blk_len; i++) {
        if (blk_pos + i < length) sdr->iqSample[blk_pos+i] = fir_in[i];
    }
  
  } // for (blk_pos ...

  // Restore blk_len
  blk_len = fft_len + 1 - fir_len;



}

#endif
