#include <stat_eval.h>

/* function for comparing two double values */
int 
compare_time_stamp_vals(const void *elem1, 
    			const void *elem2) {
	double val1 = *((double*)elem1);
	double val2 = *((double*)elem2);

	if (val1 > val2)
		return 1;

	if (val1 < val2)
	    	return -1;

	return 0;
}


/* helper function for the box-plot evaluation */
static inline void
box_plot_evaluation(double *values, 
    		    unsigned int iterations,
		    box_plot_vals_t *box_plot) {
	
	/* iterations is even */
	if (iterations%2 == 0) {
		box_plot->median = 
		    (values[iterations/2-1]+values[iterations/2])/2;

		/* halfs are even */
		if ((iterations/2)%2 == 0) {
			box_plot->lower_quartil = 
			    (values[iterations/4-1]+
			     values[iterations/4])/2;	    
			box_plot->upper_quartil = 
			    (values[iterations*3/4-1]+
			     values[iterations*3/4])/2;	    
		}
		/* halfs are odd */
		else {
			box_plot->lower_quartil = 
			    values[iterations/4];
			box_plot->upper_quartil = 
			    values[iterations*3/4];

		}
	} 
	/* iterations is odd */
	else {
		box_plot->median = values[iterations/2];

		/* halfs are even */
		if ((iterations/2)%2 == 0) {
			box_plot->lower_quartil = 
			    (values[iterations/4-1]+
			     values[iterations/4])/2;	    
			box_plot->upper_quartil = 
			    (values[iterations*3/4]+
			     values[iterations*3/4+1])/2;	    
		}
		/* halfs are odd */
		else {
			box_plot->lower_quartil = 
			    values[iterations/4];
			box_plot->upper_quartil = 
			    values[iterations*3/4];
		}
	}

	/* 
	 * determine whiskers and outlier
	 */
	double iqr = box_plot->upper_quartil-
	    box_plot->lower_quartil;

	/* lower whisker and outlier */
	box_plot->lower_outlier_idx = 0;
	while (values[box_plot->lower_outlier_idx] < 
	    (box_plot->lower_quartil-1.5*iqr)) {
		box_plot->lower_outlier_idx++;
	}
	box_plot->lower_whisker = 
	    values[box_plot->lower_outlier_idx];
	box_plot->lower_outlier = box_plot->lower_outlier_idx;

	/* upper whisker and outlier */
	box_plot->upper_outlier_idx = iterations-1;
	while (values[box_plot->upper_outlier_idx] > 
	    (box_plot->upper_quartil+1.5*iqr)) {
		box_plot->upper_outlier_idx--;
	}
	box_plot->upper_whisker = 
	    values[box_plot->upper_outlier_idx];
	box_plot->upper_outlier = (iterations-1)-box_plot->upper_outlier_idx;
}

/* function for performing a statistical evaluation on an array of doubles */
void 
statistical_eval(double *values, 
		 unsigned int iterations, 
		 stat_eval_t *stat_values) {
	unsigned int i;
	
	/* error check */
	if ((values == NULL) || (iterations == 0) || (stat_values == NULL)) {
		return;
	}
	
	/* sort the results */
	qsort(values, iterations, sizeof(double), compare_time_stamp_vals);
	
	/* determine basic properties */
	stat_values->minimum = values[0];
	stat_values->maximum = values[iterations-1];

	stat_values->average = 0;
	for (i=0; i<iterations; ++i) {
		stat_values->average += values[i];
	}
	stat_values->average /= iterations;

	stat_values->variance = 0;
	for (i=0; i<iterations; ++i) {
		stat_values->variance += 
		    (stat_values->average-values[i])*
		    (stat_values->average-values[i]);
	}
	stat_values->variance /= (iterations-1);

	stat_values->std_devation = sqrt(stat_values->variance);

	/* determine boxplot parameters */
	box_plot_evaluation(values, iterations, &(stat_values->box_plot));	
		
}

/* print the reults of a statistical evalution to 'output' */
void 
print_statistics(const stat_eval_t *stat_values,
    		 uint32_t iterations,
    		 FILE *output) {

	fprintf(output, "\n");
	fprintf(output, "##----------------------------------------------\n");
	fprintf(output, "##----------------------------------------------\n");
	fprintf(output, "#Iterations     %d\n", iterations);
	fprintf(output, "#Minimum        %.2f\n", stat_values->minimum);
	fprintf(output, "#Maximum        %.2f\n", stat_values->maximum);
	fprintf(output, "#Average        %.2f\n", stat_values->average);
	fprintf(output, "#Variance       %.2f\n", stat_values->variance);
	fprintf(output, "#Std-Deviation  %.2f\n", stat_values->std_devation);
	fprintf(output, "##----------------------------------------------\n");
	fprintf(output, "#Median         %.2f\n", stat_values->box_plot.median);
	fprintf(output, "#Lower Quartil  %.2f\n", stat_values->box_plot.lower_quartil);
	fprintf(output, "#Upper Quartil  %.2f\n", stat_values->box_plot.upper_quartil);
	fprintf(output, "#Lower Whisker  %.2f\n", stat_values->box_plot.lower_whisker);
	fprintf(output, "#Upper Whisker  %.2f\n", stat_values->box_plot.upper_whisker);
	fprintf(output, "#Lower Outlier  %d\n", stat_values->box_plot.lower_outlier);
	fprintf(output, "#Upper Outlier  %d\n", stat_values->box_plot.upper_outlier);
	fprintf(output, "test_run_xy  %.4f %.4f %.4f %.4f %.4f\n", 
	    	stat_values->box_plot.median,
		stat_values->box_plot.upper_quartil,
		stat_values->box_plot.lower_quartil,
		stat_values->box_plot.upper_whisker,
		stat_values->box_plot.lower_whisker);

	return;
}

