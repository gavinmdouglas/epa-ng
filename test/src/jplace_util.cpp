#include "Epatest.hpp"

#include "src/pllhead.hpp"
#include "src/file_io.hpp"
#include "src/epa_pll_util.hpp"
#include "src/jplace_util.hpp"
#include "src/Tree.hpp"
#include "src/MSA.hpp"
#include "src/place.hpp"

#include <string>
#include <vector>
#include <iostream>

using namespace std;

TEST(jplace_util, pquery_to_jplace_string)
{
  // buildup
  auto query_msa = build_MSA_from_file(env->query_file);
  auto reference_msa = build_MSA_from_file(env->reference_file);
  auto tree = Tree(env->tree_file, reference_msa, env->model, env->options);

  // tests
  auto sample = place(tree, query_msa);
  vector<string> out;

  for (auto const &p : sample)
    out.push_back(pquery_to_jplace_string(p, query_msa));

  EXPECT_EQ(out.size(), 2);

  // teardown

}
