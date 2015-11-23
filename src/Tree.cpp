#include "Tree.hpp"

#include <stdexcept>
#include <tuple>
#include <cassert>

#include "pll_util.hpp"
#include "epa_pll_util.hpp"
#include "file_io.hpp"
#include "Sequence.hpp"

using namespace std;

Tree::Tree(const string& tree_file, const string& msa_file, const Model& model) : model_(model)
{
  // TODO detect filetype

  //parse, build tree
  nums_ = Tree_Numbers();

  ref_msa_ = build_MSA_from_file(msa_file);

  tie(partition_, tree_) = build_partition_from_file(tree_file, model_, nums_, ref_msa_.num_sites());

  link_tree_msa(tree_, partition_, ref_msa_, nums_.tip_nodes);

  precompute_clvs(tree_, partition_, nums_);
}

Tree::~Tree()
{
  // free data segment of tree nodes
  utree_free_node_data(tree_);
	pll_partition_destroy(partition_);
  pll_utree_destroy(tree_);

}

Placement_Set Tree::place(const MSA &msa) const
{
  // get all edges
  auto node_list = new pll_utree_t*[nums_.branches];
  auto num_traversed = utree_query_branches(tree_, node_list);

  // output class
  Placement_Set placements(get_numbered_newick_string(tree_));

  // place all s on every edge
  for (auto const &s : msa)// make sure a reference, not a copy, is returned
  {
    placements.emplace_back(nums_.branches, s);
    for (unsigned int i = 0; i < num_traversed; ++i)
    {
      placements.back().set(i, place_on_edge(s, node_list[i]));
    }
  }
  delete [] node_list;

  return placements;
}

/* Compute loglikelihood of a Sequence, when placed on the edge defined by node */
// TODO pass tiny tree instead of node
double Tree::place_on_edge(const Sequence& s, pll_utree_t * node, bool optimize) const
{
  assert(node != NULL);

  int old_left_clv_index = 0;
  int old_right_clv_index = 1;
  int new_tip_clv_index = 2;
  int inner_clv_index = 3;
  pll_utree_t * old_left = node;
  pll_utree_t * old_right = node->back;

  /* create new tiny tree including the given nodes and a new node for the sequence s

               [2]
             new_tip
                |
                |
              inner [3]
             /     \
            /       \
      old_left     old_right
        [0]           [1]

    numbers in brackets are the nodes clv indices
  */

  // stack (TODO) new partition with 3 tips, 1 inner node
  //partition_create_stackd(tiny_partition,
  pll_partition_t * tiny_partition =
  pll_partition_create(   3, // tips
                          1, // extra clv's
                          partition_->states,
                          partition_->sites,
                          0, // number of mixture models
                          partition_->rate_matrices,
                          3, // number of prob. matrices (one per unique branch length)
                          partition_->rate_cats,
                          4, // number of scale buffers (one per node)
                          partition_->attributes);

  // shallow copy model params
  tiny_partition->rates = partition_->rates;
  tiny_partition->subst_params = partition_->subst_params;
  tiny_partition->frequencies = partition_->frequencies;

  // shallow copy 2 existing nodes clvs
  tiny_partition->clv[old_left_clv_index] =
                          partition_->clv[old_left->clv_index];
  tiny_partition->clv[old_right_clv_index] =
                          partition_->clv[old_right->clv_index];

  // shallow copy scalers
  if (old_left->scaler_index != PLL_SCALE_BUFFER_NONE)
    tiny_partition->scale_buffer[old_left_clv_index] =
                          partition_->scale_buffer[old_left->scaler_index];
  if (old_right->scaler_index != PLL_SCALE_BUFFER_NONE)
    tiny_partition->scale_buffer[old_right_clv_index] =
                          partition_->scale_buffer[old_right->scaler_index];

  // init the new tip with s.sequence(), branch length
  pll_set_tip_states(tiny_partition, new_tip_clv_index, pll_map_nt, s.sequence().c_str());

  // set up branch lengths
  /* heuristic insertion as described in EPA paper from 2011 (Berger et al.):
    original branch, now split by "inner", or base, node of the inserted sequence,
    defines the new branch lengths between inner and old left/right respectively
    as old branch length / 2.
    The new branch leading from inner to the new tip is initialized with length 0.9,
    which is the default branch length in RAxML.
    */
  double branch_lengths[2] = { old_left->length / 2, DEFAULT_BRANCH_LENGTH};
  unsigned int matrix_indices[2] = { 0, 1 };

  // TODO Newton-Raphson
  if (optimize)
  {
    // something like: setup params structure, call opt. branch lengths local with radius 1
    // on the inner node
  }

  // use branch lengths to compute the probability matrices
  pll_update_prob_matrices(tiny_partition, 0, matrix_indices, branch_lengths, 2);

  /* Creating a single operation for the inner node computation.
    Here we specify which clvs and pmatrices to use, so we don't need to mess with
    what the internal tree structure points to */
  pll_operation_t ops[1];
  ops[0].parent_clv_index    = inner_clv_index;
  ops[0].child1_clv_index    = old_left_clv_index;
  ops[0].child2_clv_index    = old_right_clv_index;
  ops[0].child1_matrix_index = 0;// TODO depends on NR vs heuristic
  ops[0].child2_matrix_index = 0;// TODO depends on NR vs heuristic
  ops[0].parent_scaler_index = inner_clv_index;// TODO this should be inner_clv_index once scale buffers are fixed
  ops[0].child1_scaler_index = old_left_clv_index;
  ops[0].child2_scaler_index = old_right_clv_index;

  // use update_partials to compute the clv poining toward the new tip
  pll_update_partials(tiny_partition, ops, 1);

  // compute the loglikelihood using inner node and new tip
  double likelihood =  pll_compute_edge_loglikelihood(tiny_partition,
                                        new_tip_clv_index,
                                        PLL_SCALE_BUFFER_NONE,// scaler_index
                                        inner_clv_index,
                                        inner_clv_index,  // scaler_index
                                        1,// matrix index of branch TODO depends on NR
                                        0);// freq index

  // TODO properly free the tiny partition or allocate it on the stack
  // unset model params
  tiny_partition->rates = NULL;
  tiny_partition->subst_params = NULL;
  tiny_partition->frequencies = NULL;
  // unset existing nodes clvs
  tiny_partition->clv[old_left_clv_index] = NULL;
  tiny_partition->clv[old_right_clv_index] = NULL;
  // unset scalers
  tiny_partition->scale_buffer[old_left_clv_index] = NULL;
  tiny_partition->scale_buffer[old_right_clv_index] = NULL;

  pll_partition_destroy(tiny_partition);

  return likelihood;
}
