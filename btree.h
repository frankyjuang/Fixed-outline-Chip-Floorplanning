#include <iostream>
#include <string>
#include <vector>
#include <cmath>
#include <cstdio>
#include <algorithm>

using namespace std;

#define EPSILON 1e-8
#define INF 1e8

int myrandom (int i) { return rand() % i; }

class Node {
public:
    Node(string name, int w, int h, int nidx) : _name(name), _width(w), _height(h), _nidx(nidx) {
        _left = _right = _parent = 0;
        _x1 = _x2 = _y1 = _y2 = 0;
        _mid_x = _mid_y = 0.0;
    }

    Node(const Node &n) {
        _name = n._name;
        _width = n._width;
        _height = n._height;
        _nidx = n._nidx;

        _left = _right = _parent = 0;
        _x1 = _x2 = _y1 = _y2 = 0;
        _mid_x = _mid_y = 0.0;
    }

    void operator=(const Node &n) {
        _x1 = n._x1;
        _x2 = n._x2;
        _y1 = n._y1;
        _y2 = n._y2;
    }

    void printDebug() {
        printf("%p %p %p %p %d %d %d %d %d %f %f %d %d %s\n", this, _left, _right, _parent, _nidx, _x1, _x2, _y1, _y2, _mid_x, _mid_y, _width, _height, _name.c_str());
    }
//private
    Node* _left;
    Node* _right;
    Node* _parent;
    int _x1, _x2, _y1, _y2;
    double _mid_x, _mid_y;

    string _name;
    int _width, _height;
    int _nidx;
};

class Term {
public:
    Term(string name, int x, int y) : _name(name), _x(x), _y(y) {}

    string _name;
    int _x;
    int _y;
};

class Net {
public:
    Net() {}

    vector<Term*> _terms;
    vector<Node*> _nodes;
};

class Btree {
public:
    Btree() {
        _root = 0;
        _t_width = _t_height = 0;
        _cost = _wire_len = 0.0;
    }

    Btree(const Btree &obj) {
        _root = 0;
        _t_width = _t_height = 0;
        _cost = _wire_len = 0.0;

        for (int i = 0; i < _block_num; ++i) {
            Node* n = new Node(*(obj._node_list[i]));
            _node_list.push_back(n);
        }
    }

    void operator=(const Btree &obj) {
        for (int i = 0; i < _block_num; ++i) {
            Node* n = new Node(*(obj._node_list[i]));
            _node_list.push_back(n);
        }
    }

    void buildTree() {
        
        int* indices = new int[_block_num];
        for (int i = 0; i < _block_num; ++i) {
            indices[i] = i;
        }
        random_shuffle(indices, indices + _block_num, myrandom); 

        _root = indices[0];
        bool go_right;
        Node* current;
        Node* left_most = _node_list[_root];
        Node* right_most = _node_list[_root];
        for (int i = 1; i < _block_num; ++i) {
            current = _node_list[indices[i]];
            if (go_right) {
                right_most->_right = current;
                current->_parent = right_most;
                left_most = right_most = current;
                go_right = false;
                continue;
            }
            left_most->_left = current;
            current->_parent = left_most;
            left_most = current;
            countCost(_node_list[_root], false);
            if (_t_width > _out_width)
                go_right = true;
        }
        countCost(_node_list[_root], false);
        //cout << _t_width << " " << _t_height << endl;
    }

    void calculateWireLength() {
        double left, right, top, bottom;
        double total_length = 0;
        for (vector<Net*>::iterator it_net = _net_list.begin(); it_net < _net_list.end(); ++it_net) {
            left = bottom = INF;
            right = top = -INF;
            vector<Term*>& terms = (*it_net)->_terms;
            for (vector<Term*>::iterator it_term = terms.begin(); it_term < terms.end(); ++it_term) {
                int& x = (*it_term)->_x;
                int& y = (*it_term)->_y;
                if (x < left)
                    left = x;
                if (x > right)
                    right = x;
                if (y < bottom)
                    bottom = y;
                if (y > top)
                    top = y;
            }
            vector<Node*>& nodes = (*it_net)->_nodes;
            for (vector<Node*>::iterator it_node = nodes.begin(); it_node < nodes.end(); ++it_node) {
                double& x = (*it_node)->_mid_x;
                double& y = (*it_node)->_mid_y;
                if (x < left)
                    left = x;
                if (x > right)
                    right = x;
                if (y < bottom)
                    bottom = y;
                if (y > top)
                    top = y;
            }
            //cout << left << " " << right << " " << top << " " << bottom << endl;
            total_length += (right - left) + (top - bottom);
        }
        _wire_len = total_length;
    }

    double countCost(Node* node, bool fromLeft) {
        if (!node) {
            return 0;
        } else if (node->_nidx == _root) {
            _skyline.clear();
            _t_width = _t_height = 0;
            node->_x1 = node->_y1 = 0;
            while (_skyline.size() < node->_width)
                _skyline.push_back(0);
        } else {
            if (fromLeft) {     // come from left node
                node->_x1 = node->_parent->_x2;
            } else {            // come from right node
                node->_x1 = node->_parent->_x1;
            }
            node->_y1 = 0;
            for (int i = node->_x1; i < node->_x1 + node->_width; ++i) {
                while (_skyline.size() < i+1)
                    _skyline.push_back(0);
                if (node->_y1 < _skyline[i])
                    node->_y1 = _skyline[i];
            }
        }
        node->_x2 = node->_x1 + node->_width;
        node->_y2 = node->_y1 + node->_height;
        node->_mid_x = 0.5 * (node->_x1 + node->_x2);
        node->_mid_y = 0.5 * (node->_y1 + node->_y2);

        // update tree-graph-border
        if (node->_x2 > _t_width)
            _t_width = node->_x2;
        if (node->_y2 > _t_height)
            _t_height = node->_y2;

        // update skyline
        for (int i = node->_x1; i < node->_x2; ++i)
            if (node->_y2 > _skyline[i])
                _skyline[i] = node->_y2;

        // preorder traverse
        countCost(node->_left, true);
        countCost(node->_right, false);

        // return cost 
        if (node->_nidx == _root) {
            double ratio_cost = pow((_r_star - 1.0 * _t_height / _t_width), 2.0);
            calculateWireLength();
            return _w1 * calRealCost() + _w2 * ratio_cost;
        } else {
            return 0;
        }
    }

    int calArea() {
        return _t_height * _t_width;
    }

    double calRealCost() {
        return 1.0 * _alpha * calArea() + (1-_alpha) * _wire_len;
    }
     
    void copyAll(Btree& best) {
        _t_width = best._t_width;
        _t_height = best._t_height;
        _wire_len = best._wire_len;

        for (size_t i = 0; i < _node_list.size(); ++i)
            _node_list[i]->operator=(*(best._node_list[i]));
    }

    bool isInOutline() {
        if (_t_width <= _out_width && _t_height <= _out_height)
            return true;
        return false;
    }

    bool rotate_left(int node_idx) {
        Node* x = _node_list[node_idx];
        if (!x->_right)
            return false;
        Node* y = x->_right;
        x->_right = y->_left;
        if (y->_left)
            y->_left->_parent = x;
        y->_parent = x->_parent;
        if (!x->_parent)
            _root = y->_nidx;
        else if (x == x->_parent->_left)
            x->_parent->_left = y;
        else
            x->_parent->_right = y;
        y->_left = x;
        x->_parent = y;
        return true;
    }

    bool rotate_right(int node_idx) {
        Node* x = _node_list[node_idx];
        if (!x->_left)
            return false;
        Node* y = x->_left;
        x->_left = y->_right;
        if (y->_right)
            y->_right->_parent = x;
        y->_parent = x->_parent;
        if (!x->_parent)
            _root = y->_nidx;
        else if (x == x->_parent->_left)
            x->_parent->_left = y;
        else
            x->_parent->_right = y;
        y->_right = x;
        x->_parent = y;
        return true;
    }

    void rotate_macro(int node1_idx) {
        Node* x = _node_list[node1_idx];
        int tmp = x->_width;
        x->_width = x->_height;
        x->_height = tmp;
    }

    void swap(int node1_idx, int node2_idx) {
        //cout << node1_idx << " " << node2_idx << endl;
        Node* x = _node_list[node1_idx];
        Node* y = _node_list[node2_idx];

        if (x->_parent == y) {          // If x, y are adjacent, make sure x is y's parent.
            x = _node_list[node2_idx];
            y = _node_list[node1_idx];
        }
        if (x->_left == y) {            // x, y are adjacent, and y is x's left child
            if (x->_right)
                x->_right->_parent = y;
            if (y->_right)
                y->_right->_parent = x;

            Node* x_right = x->_right;
            x->_right = y->_right;
            y->_right = x_right;

            if (!x->_parent)
                _root = y->_nidx;
            else if (x == x->_parent->_left)
                x->_parent->_left = y;
            else
                x->_parent->_right = y;

            if (y->_left)
                y->_left->_parent = x;
            x->_left = y->_left;
            y->_parent = x->_parent;

            x->_parent = y;
            y->_left = x;

            return;
        } else if (x->_right == y) {    // x, y are adjacent, and y is x's right child
            if (x->_left)
                x->_left->_parent = y;
            if (y->_left)
                y->_left->_parent = x;

            Node* x_left = x->_left;
            x->_left = y->_left;
            y->_left = x_left;

            if (!x->_parent)
                _root = y->_nidx;
            else if (x == x->_parent->_left)
                x->_parent->_left = y;
            else
                x->_parent->_right = y;

            if (y->_right)
                y->_right->_parent = x;
            x->_right = y->_right;
            y->_parent = x->_parent;

            x->_parent = y;
            y->_right = x;

            return;
        }

        // handle x-parent back to y
        if (!x->_parent)
            _root = y->_nidx;
        else if (x == x->_parent->_left)
            x->_parent->_left = y;
        else
            x->_parent->_right = y;

        // handle y-parent back to x
        if (!y->_parent)
            _root = x->_nidx;
        else if (y == y->_parent->_left)
            y->_parent->_left = x;
        else
            y->_parent->_right = x;

        // exchange subtree parent pointers
        if (x->_left)
            x->_left->_parent = y;
        if (x->_right)
            x->_right->_parent = y;
        if (y->_left)
            y->_left->_parent = x;
        if (y->_right)
            y->_right->_parent = x;

        // exchange x, y's out-pointers
        Node* x_parent = x->_parent;
        Node* x_left = x->_left;
        Node* x_right = x->_right;
        x->_parent = y->_parent;
        x->_left = y->_left;
        x->_right = y->_right;
        y->_parent = x_parent;
        y->_left = x_left;
        y->_right = x_right;
    }

    bool del_n_ins(int node1_idx, int node2_idx, int* info) {
        Node* x = _node_list[node1_idx];
        Node* y = _node_list[node2_idx];

        Node* y2 = y;
        while (y2->_parent) {       // check if x is y's ancestor
            y2 = y2->_parent;
            if (y2 == x)
                return false;
        }

        if (!x->_parent)
            return false;
        else if (x == x->_parent->_left) {
            x->_parent->_left = 0;
            info[1] = 0;
        }
        else {
            x->_parent->_right = 0;
            info[1] = 1;
        }
        info[0] = x->_parent->_nidx;
        
        while (y->_left && y->_right) {
            int choice = rand() % 2;
            if (choice)
                y = y->_left;
            else
                y = y->_right;
        }
        x->_parent = y;
        info[2] = y->_nidx;
        if (!y->_left && !y->_right) {
            int choice = rand() % 2;
            if (choice) {
                y->_left = x;
                info[3] = 0;
            }
            else {
                y->_right = x;
                info[3] = 1;
            }
        } else if (!y->_left) {
            y->_left = x;
            info[3] = 0;
        } else {
            y->_right = x;
            info[3] = 1;
        }
        return true;
    }

    void recover_del_n_ins(int node_idx, int* info) {
        Node* x = _node_list[node_idx];
        Node* x_parent = _node_list[info[0]];
        Node* y = _node_list[info[2]];
        if (info[3])        // x is y's right node
            y->_right = 0;
        else
            y->_left = 0;

        x->_parent = x_parent;
        if (info[1])        // x is x_parent's right node
            x_parent->_right = x;
        else
            x_parent->_left = x;
    }

//private
    vector<int> _skyline;       // y-axis skyline
    vector<Node*> _node_list;   // store all nodes
    int _t_width, _t_height;    // tree graph width, tree graph height
    double _wire_len;           // HPML
    double _cost;
    int _root;

    static map<string, Term*> _term_map;
    static map<string, Node*> _node_map;
    static vector<Net*> _net_list;
    static int _out_width, _out_height;    // outline width, outline height
    static int _block_num;
    static double _w1, _w2;
    static double _r_star;
    static double _alpha;
};
