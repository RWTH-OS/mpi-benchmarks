#ifndef _STAT_EVAL_H
#define _STAT_EVAL_H

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>

typedef struct _box_plot_vals_t {
	double lower_whisker;
	double upper_whisker;
	double lower_quartil;
	double upper_quartil;
	double median;
	uint32_t lower_outlier;
	uint32_t upper_outlier;
	uint32_t lower_outlier_idx;
	uint32_t upper_outlier_idx;
} box_plot_vals_t;

typedef struct _stat_eval_t {
	double minimum;
	double maximum;
	double average;
	double variance;
	double std_devation;
	box_plot_vals_t box_plot;
} stat_eval_t;

void 
statistical_eval(double *values, 
		 uint32_t iterations, 
		 stat_eval_t *stat_values);

void 
print_statistics(const stat_eval_t *stat_values,
		 uint32_t iterations,    	
    		 FILE *output);
#endif /* _STAT_EVAL_H */
