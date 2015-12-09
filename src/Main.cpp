#include <iostream>
#include <unistd.h>
#include <string>

#include "epa.hpp"

using namespace std;

static void print_help()
{
  cout << "EPA - Evolutionary Placement Algorithm" << endl << endl;
  cout << "USAGE: epa [options] <reference_tree_file> <reference_MSA_file>" << endl << endl;
  cout << "OPTIONS:" << endl;
  cout << "  -h \tDisplay this page" << endl;
  cout << "  -q \tPath to separate query MSA file. If none is provided, epa will assume" << endl;
  cout << "     \tquery reads are in the reference_MSA_file (second parameter)" << endl;
  cout << "  -b \toptimize branch lengths on insertion" << endl;
  cout << "  -m \tSpecify model of nucleotide substitution" << endl << endl;
  cout << "     \tGTR \tGeneralized time reversible" << endl;
  cout << "     \tJC69 \tJukes-Cantor Model" << endl;
  cout << "     \tK80 \tKimura 80 Model" << endl;
};

int main(int argc, char** argv)
{
  string invocation("");
  string model_id("GTR");
  bool heuristic = true;
  for (int i = 0; i < argc; ++i)
  {
    invocation += argv[i];
    invocation += " ";
  }

  string query_file("");

  int c;
  while((c =  getopt(argc, argv, "hbq:")) != EOF)
  {
      switch (c)
      {
           case 'q':
               query_file += optarg;
               break;
           case 'h':
               print_help();
               exit(0);
               break;
           case 'b':
               heuristic = false;
               break;
           case ':':
               cerr << "Missing option." << endl;
               exit(EXIT_FAILURE);
               break;
      }
  }

  if (argc < 2)
  {
    cerr << "Insufficient parameters!" << endl;
    print_help();
    cout.flush();
    exit(EXIT_FAILURE);
  }
  // first two params are always the reference tree and msa file paths
  string tree_file(argv[optind]);
  string reference_file(argv[optind + 1]);

  Model model(model_id);

	epa(tree_file,
      reference_file,
      query_file,
      model,
      heuristic,
      invocation);
	return 0;
}
