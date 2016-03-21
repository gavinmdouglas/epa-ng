#pragma once

#include "Sample.hpp"
#include "Range.hpp"
#include "MSA.hpp"

void compute_and_set_lwr(Sample& sample);
void discard_bottom_x_percent(Sample& sample, const double x);
void discard_by_support_threshold(Sample& sample, const double thresh);
void discard_by_accumulated_threshold(Sample& sample, const double thresh);
Range superset(Range a, Range b);
Range get_valid_range(std::string sequence);
void find_collapse_equal_sequences(MSA& msa);
