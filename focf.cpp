#include <iostream>
#include <fstream>
#include <string>
#include <ctime>
#include <cstdlib>
#include <cmath>
#include <map>
#include "btree.h"

#define T0 100          // initial temperature
#define P 100           // trials per reduction of temperature
#define r 0.99          // temperature reduction rate
#define NUM_TREE 5      // number of trees

#define EPSILON 1e-8
#define INF 1e8

using namespace std;

// static variables
map<string, Term*> Btree::_term_map;
map<string, Node*> Btree::_node_map;
vector<Net*> Btree::_net_list;
int Btree::_out_width;
int Btree::_out_height;
int Btree::_block_num;
double Btree::_w1;
double Btree::_w2;
double Btree::_r_star;
double Btree::_alpha;

// functions
void tuneParameter(Btree&,bool);

int main(int argc, char** argv) {
    string dump;
    fstream fin, fout;
    int outline_width, outline_height;
    int block_num, term_num;
    int net_num, deg_num; 
    int total_runs = 0;
    bool solved = false;
    clock_t start = clock();

    srand(time(NULL));

    // read .block file
    fin.open(argv[2], ios::in);
    fin >> dump;
    fin >> outline_width;
    fin >> outline_height;
    fin >> dump;
    fin >> block_num;
    fin >> dump;
    fin >> term_num;

    // initialize static member
    Btree::_out_width = outline_width;
    Btree::_out_height = outline_height;
    Btree::_block_num = block_num;
    Btree::_alpha = stod(argv[1]); 
    Btree::_r_star = 1.0 * outline_height / outline_width;

    // parse nodes
    Btree* trees = new Btree[NUM_TREE];
    Node* node;
    string name;
    int width, height;
    for (int i = 0; i < block_num; ++i) {
        fin >> name >> width >> height;
        node = new Node(name, width, height, i);
        trees[0]._node_list.push_back(node);
        trees[0]._node_map.insert(pair<string, Node*>(name, node));
    }

    // parse terminals
    Term* term;
    int x, y;
    for (int i = 0; i < term_num; ++i) {
        fin >> name >> dump >> x >> y;
        term = new Term(name, x, y);
        trees[0]._term_map.insert(pair<string, Term*>(name, term));
    }
    fin.close();

    // read .net file and build nets
    fin.open(argv[3], ios::in);
    fin >> dump;
    fin >> net_num;
    Net* net;
    map<string, Term*>& term_map = trees[0]._term_map;
    map<string, Node*>& node_map = trees[0]._node_map;
    for (int i = 0; i < net_num; ++i) {
        fin >> dump;
        fin >> deg_num;
        net = new Net();
        for (int j = 0; j < deg_num; ++j) {
            fin >> name;
            if (term_map.find(name) != term_map.end())      // it is a terminal
                net->_terms.push_back(term_map[name]);
            else        // or a node
                net->_nodes.push_back(node_map[name]);
        }
        trees[0]._net_list.push_back(net);
    }
    fin.close();

    // build b-star trees
    trees[0].buildTree();
    for (int i = 1; i < NUM_TREE; ++i) {
        trees[i] = trees[0];
        trees[i].buildTree();
    }

    // keep best tree
    Btree tree_best = trees[0];
    tree_best._t_width = INF;
    tree_best._t_height = INF;
    tree_best._wire_len = INF;

    tuneParameter(trees[0], true);        // tune _w1
    tuneParameter(trees[0], false);       // tune _w2
    //cout << Btree::_w1 << " " << Btree::_w2 << endl;

    // initialize simulated annealing
    bool p_break;                           // whether any single tree has reach P
    double new_cost;                        // cost of new state
    double min_cost;                        // mininum cost amoung all trees
    double cost_sum = 0;                    // summation of every min_cost
    int cost_count = 0;                     // count min_cost times
    double prob;                            // probability of jumping to worse state
    int* p = new int[NUM_TREE];             // every single tree's p
    int info[4];                            // record del_n_ins informations
    double temperature = T0;                // initialize temperature

    // SA starts
    while (temperature > EPSILON) {
        for (int i = 0; i < NUM_TREE; ++i)
            p[i] = 0;
        while (true) {
            p_break = false;
            ++total_runs;
            for (int i = 0; i < NUM_TREE; ++i) {
                ++p[i];
                // random pick operations and operatees
                int pick = rand() % 5;
                int rand_node1 = rand() % block_num;
                int rand_node2 = rand() % block_num;
                if (rand_node1 == rand_node2)
                    rand_node2 = (rand_node2 + 1) % block_num;

                // execute operation
                if (pick == 0) {
                    if (!trees[i].rotate_left(rand_node1))
                        continue;
                } else if (pick == 1) {
                    if (!trees[i].rotate_right(rand_node1))
                        continue;
                } else if (pick == 2) {
                    trees[i].rotate_macro(rand_node1);
                } else if (pick == 3) {
                    trees[i].swap(rand_node1, rand_node2);
                } else {
                    if (!trees[i].del_n_ins(rand_node1, rand_node2, info))
                        continue;
                }

                // calculate cost of new state
                new_cost = trees[i].countCost(trees[i]._node_list[trees[i]._root], false);

                if (new_cost > trees[i]._cost) {        // worse cost --> consider do or not do operation
                    prob = 1.0 * rand() / RAND_MAX; 
                    if (prob > exp(-1 * (new_cost - trees[i]._cost) / temperature)) {       // worse cost --> undo operation
                        if (pick == 0) {
                            trees[i].rotate_right(trees[i]._node_list[rand_node1]->_parent->_nidx);
                        } else if (pick == 1) {
                            trees[i].rotate_left(trees[i]._node_list[rand_node1]->_parent->_nidx);
                        } else if (pick == 2) {
                            trees[i].rotate_macro(rand_node1);
                        } else if (pick == 3) {
                            trees[i].swap(rand_node1, rand_node2);
                        } else {
                            trees[i].recover_del_n_ins(rand_node1, info);
                        }
                    } else {        // worst cost --> still do operation
                        trees[i]._cost = new_cost;
                        ++p[i];
                    }
                } else {        // better cost --> do operation
                    trees[i]._cost = new_cost;
                    ++p[i];
                }

                // check if fits outline
                if (trees[i].isInOutline()) {
                    //cout << "SUCCESS!!!!!!\t"  << i << endl;
                    solved = true;
                    if (trees[i].calRealCost() < tree_best.calRealCost()) {
                        //cout << "UPDATE BEST!!!!!" << endl;
                        //cout << trees[i]._t_width << "\t" << trees[i]._t_height << endl;
                        tree_best.copyAll(trees[i]);
                    }
                }
            }
            
            // check if p reach P
            for (int i = 0; i < NUM_TREE; ++i)
                if (p[i] >= P) {
                    p_break = true;
                    break;
                }

            if (p_break)
                break;
        }
        // temperature decay
        temperature *= r;
        //cout << "temperature = " << temperature;

        // calculate minimum cost among all trees
        min_cost = INF;
        for (int i = 0; i < NUM_TREE; ++i) {
            if (trees[i]._cost < min_cost)
                min_cost = trees[i]._cost;
        }
        cost_sum += min_cost;
        ++cost_count;
        //cout << "\tmin cost = " << min_cost;
        //cout << "\tavg cost = " << cost_sum / cost_count << endl;
    }

    if (!solved) {
        cout << "Failed to put all macros in the fixed-outline." << endl;
        cout << "You are welcome to tune the parameters and give it another try." << endl;
        min_cost = INF;
        int best_idx;
        for (int i = 0; i < NUM_TREE; ++i) {
            if (trees[i].calRealCost() < min_cost) {
                min_cost = trees[i].calRealCost();
                best_idx = i;
            }
        }
        tree_best.copyAll(trees[best_idx]);
    }

    // write output to file
    fout.open(argv[4], ios::out);
    fout << fixed;
    fout << tree_best.calRealCost() << endl;
    fout << tree_best._wire_len << endl;
    fout << tree_best.calArea() << endl;
    fout << tree_best._t_width << " " << tree_best._t_height << endl;
    fout << (clock() - start) / (double) CLOCKS_PER_SEC << endl;
    for (vector<Node*>::iterator it = tree_best._node_list.begin(); it < tree_best._node_list.end(); ++it)
        fout << (*it)->_name << " " << (*it)->_x1 << " " << (*it)->_y1 << " " << (*it)->_x2 << " " << (*it)->_y2 << endl;
    fout.close();
    
    //cout << "Total: " << total_runs << endl;

    return 0;
}

void tuneParameter(Btree& tree, bool is_w1) {       // get the rough estimate for normalization
    Btree test = tree;
    double max_cost = -INF;
    int info[4];
    double w_temp;
    
    if (is_w1) {
        Btree::_w1 = 1;
        w_temp = Btree::_w2;
        Btree::_w2 = 0;
    } else {
        Btree::_w2 = 1;
        w_temp = Btree::_w1;
        Btree::_w1 = 0;
    }

    test.buildTree();

    for (int i = 0; i < 1000;) {
        // random pick operations and operatees
        int pick = rand() % 5;
        int rand_node1 = rand() % test._block_num;
        int rand_node2 = rand() % test._block_num;
        if (rand_node1 == rand_node2)
            rand_node2 = (rand_node2 + 1) % test._block_num;

        // execute operation
        if (pick == 0) {
            if (!test.rotate_left(rand_node1))
                continue;
        } else if (pick == 1) {
            if (!test.rotate_right(rand_node1))
                continue;
        } else if (pick == 2) {
            test.rotate_macro(rand_node1);
        } else if (pick == 3) {
            test.swap(rand_node1, rand_node2);
        } else {
            if (!test.del_n_ins(rand_node1, rand_node2, info))
                continue;
        }
        ++i;

        // calculate cost of new state and update max cost
        test._cost = test.countCost(test._node_list[test._root], false);
        if (test._cost > max_cost)
            max_cost = test._cost;
    }
    if (is_w1) {
        Btree::_w1 = 1 / max_cost;
        Btree::_w2 = w_temp;
    } else { 
        Btree::_w2 = 1 / max_cost;
        Btree::_w1 = w_temp;
    }
}
