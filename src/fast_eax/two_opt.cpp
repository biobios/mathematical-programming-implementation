#include "two_opt.hpp"

#include <iostream>
#include <numeric>
#include <array>
#include <chrono>

namespace {
    struct Node {
        struct NodeVec {
            std::vector<Node>& nodes;
            Node& operator[](size_t index) {
                if (index >= nodes.size()) {
                    throw std::out_of_range("Index out of range");
                }
                return nodes[index];
            }
            
            size_t size() const {
                return nodes.size();
            }
            
            operator std::vector<Node>&() {
                return nodes;
            }
        };
        size_t city = std::numeric_limits<size_t>::max();
        size_t parent = std::numeric_limits<size_t>::max();
        size_t left = std::numeric_limits<size_t>::max();
        size_t right = std::numeric_limits<size_t>::max();
        bool reversed = false;

        void apply_reverse(std::vector<Node>& tree_, bool parents_is_applied) {
            NodeVec tree{tree_};
            if (parent != std::numeric_limits<size_t>::max() && !parents_is_applied) {
                tree[parent].apply_reverse(tree, false);
            }

            if (reversed) {
                std::swap(left, right);
                if (left != std::numeric_limits<size_t>::max()) {
                    tree[left].reversed = !tree[left].reversed;
                }
                if (right != std::numeric_limits<size_t>::max()) {
                    tree[right].reversed = !tree[right].reversed;
                }
                reversed = false;
            }
        }
        
        bool has_parent() const {
            return parent != std::numeric_limits<size_t>::max();
        }
        
        bool is_left_child(std::vector<Node>& tree_, bool parents_is_applied = false) {
            NodeVec tree{tree_};
            apply_reverse(tree, parents_is_applied);
            return has_parent() && tree[parent].left == city;
        }

        bool is_right_child(std::vector<Node>& tree_, bool parents_is_applied = false) {
            NodeVec tree{tree_};
            apply_reverse(tree, parents_is_applied);
            return has_parent() && tree[parent].right == city;
        }
        
        // 右側に存在する祖先を探す
        size_t get_right_parent(std::vector<Node>& tree_, bool parents_is_applied = false) {
            NodeVec tree{tree_};
            apply_reverse(tree, parents_is_applied);
            if (!has_parent()) {
                return std::numeric_limits<size_t>::max(); // 親がいない場合
            }
            
            if (is_left_child(tree, true)) {
                return parent; // 右親は親ノード
            }

            return tree[parent].get_right_parent(tree, true);
        }
        
        // 左側に存在する祖先を探す
        size_t get_left_parent(std::vector<Node>& tree_, bool parents_is_applied = false) {
            NodeVec tree{tree_};
            apply_reverse(tree, parents_is_applied);
            if (!has_parent()) {
                return std::numeric_limits<size_t>::max(); // 親がいない場合
            }
            
            if (is_right_child(tree, true)) {
                return parent; // 左親は親ノード
            }

            return tree[parent].get_left_parent(tree, true);
        }
        
        // 右隣りのノードを取得
        size_t get_next(std::vector<Node>& tree_, bool parents_is_applied = false) {
            NodeVec tree{tree_};
            apply_reverse(tree, parents_is_applied);

            // 右側に子ノードがある場合はその左端を返す
            if (right != std::numeric_limits<size_t>::max()) {
                return tree[right].get_leftmost(tree, true);
            }
            
            // そうでなければ自身より右側に存在する祖先を探す
            return tree[city].get_right_parent(tree, true);
        }

        // 左隣りのノードを取得
        size_t get_prev(std::vector<Node>& tree_, bool parents_is_applied = false) {
            NodeVec tree{tree_};
            apply_reverse(tree, parents_is_applied);
            
            // 左側に子ノードがある場合はその右端を返す
            if (left != std::numeric_limits<size_t>::max()) {
                return tree[left].get_rightmost(tree, true);
            }
            // そうでなければ自身より左側に存在する祖先を探す
            return tree[city].get_left_parent(tree, true);
        }
        
        // サブツリーの右端のノードを取得
        size_t get_rightmost(std::vector<Node>& tree_, bool parents_is_applied = false) {
            NodeVec tree{tree_};
            apply_reverse(tree, parents_is_applied);
            if (right != std::numeric_limits<size_t>::max()) {
                return tree[right].get_rightmost(tree, true);
            }
            
            return city; // 右端のノードは自身
        }
        
        // サブツリーの左端のノードを取得
        size_t get_leftmost(std::vector<Node>& tree_, bool parents_is_applied = false) {
            NodeVec tree{tree_};
            apply_reverse(tree, parents_is_applied);
            
            if (left != std::numeric_limits<size_t>::max()) {
                return tree[left].get_leftmost(tree, true);
            }
            
            return city; // 左端のノードは自身
        }
        
        // 回転
        void rotate(std::vector<Node>& tree_, bool parents_is_applied = false) {
            NodeVec tree{tree_};
            apply_reverse(tree, parents_is_applied);
            if (!has_parent()) {
                return; // 親がいない場合は回転できない
            }
            // if (city == 807 || city == 139 || city == 787 || city == 866) {
            //     std::cout << "break" << std::endl;
            // }
            // if (city >= 1084) {
            //     exit(1);
            // }
            // if (parent == std::numeric_limits<size_t>::max()) {
            //     std::cerr << "Error: Parent is not set." << std::endl;
            // }
            auto& parent_node = tree[parent];
            if (is_left_child(tree, true)) {
                // 左子ノードならば右に回転
                if (right != std::numeric_limits<size_t>::max()) {
                    auto& right_node = tree[right];
                    if (parent_node.has_parent()) {
                        auto& grandparent_node = tree[parent_node.parent];
                        if (parent_node.is_left_child(tree, true)) {
                            grandparent_node.left = city;
                        } else {
                            grandparent_node.right = city;
                        }
                        parent_node.left = right_node.city;
                        right = parent_node.city;

                        parent = grandparent_node.city;
                        parent_node.parent = city;
                        right_node.parent = parent_node.city;
                    } else {
                        parent_node.left = right_node.city;
                        right = parent_node.city;
                        parent = std::numeric_limits<size_t>::max(); // ルートノードになった場合
                        parent_node.parent = city;
                        right_node.parent = parent_node.city;
                    }
                } else {
                    if (parent_node.has_parent()) {
                        auto& grandparent_node = tree[parent_node.parent];
                        if (parent_node.is_left_child(tree, true)) {
                            grandparent_node.left = city;
                        } else {
                            grandparent_node.right = city;
                        }
                        parent_node.left = std::numeric_limits<size_t>::max(); // 左子ノード
                        right = parent_node.city;
                        parent = grandparent_node.city;
                        parent_node.parent = city;
                    } else {
                        parent_node.left = std::numeric_limits<size_t>::max(); // 左子ノード
                        right = parent_node.city;
                        parent = std::numeric_limits<size_t>::max(); // ルートノードになった場合
                        parent_node.parent = city;
                    }
                }
                // if (right != std::numeric_limits<size_t>::max()) {
                //     tree[parent].left = right;
                //     tree[right].parent = parent;
                // } else {
                //     tree[parent].left = std::numeric_limits<size_t>::max(); // 左子ノードがいない場合
                // }
                
                // right = parent;
                // if (tree[parent].has_parent()) {
                //     size_t new_parent = tree[parent].parent;
                //     if (tree[parent].is_left_child(tree, true)) {
                //         tree[new_parent].left = city;
                //     } else {
                //         tree[new_parent].right = city;
                //     }
                //     tree[parent].parent = city;
                //     parent = new_parent;
                // } else {
                //     tree[parent].parent = city;
                //     parent = std::numeric_limits<size_t>::max(); // ルートノードになった場合
                // }
            } else {
                // 右子ノードならば左に回転
                if (left != std::numeric_limits<size_t>::max()) {
                    auto& left_node = tree[left];
                    
                    if (parent_node.has_parent()) {
                        auto& grandparent_node = tree[parent_node.parent];
                        if (parent_node.is_left_child(tree, true)) {
                            grandparent_node.left = city;
                        } else {
                            grandparent_node.right = city;
                        }
                        parent_node.right = left_node.city;
                        left = parent_node.city;

                        parent = grandparent_node.city;
                        parent_node.parent = city;
                        left_node.parent = parent_node.city;
                    } else {
                        parent_node.right = left_node.city;
                        left = parent_node.city;
                        parent = std::numeric_limits<size_t>::max(); // ルートノードになった場合
                        parent_node.parent = city;
                        left_node.parent = parent_node.city;
                    }
                } else {
                    if (parent_node.has_parent()) {
                        auto& grandparent_node = tree[parent_node.parent];
                        if (parent_node.is_left_child(tree, true)) {
                            grandparent_node.left = city;
                        } else {
                            grandparent_node.right = city;
                        }
                        parent_node.right = std::numeric_limits<size_t>::max(); // 右子ノード
                        left = parent_node.city;
                        parent = grandparent_node.city;
                        parent_node.parent = city;
                    } else {
                        parent_node.right = std::numeric_limits<size_t>::max(); // 右子ノード
                        left = parent_node.city;
                        parent = std::numeric_limits<size_t>::max(); // ルートノードになった場合
                        parent_node.parent = city;
                    }
                }
                // if (right != std::numeric_limits<size_t>::max()) {
                //     tree[parent].right = left;
                //     tree[left].parent = parent;
                // } else {
                //     tree[parent].right = std::numeric_limits<size_t>::max(); // 右子ノードがいない場合
                // }

                // left = parent;
                // if (tree[parent].has_parent()) {
                //     size_t new_parent = tree[parent].parent;
                //     if (tree[parent].is_left_child(tree, true)) {
                //         tree[new_parent].left = city;
                //     } else {
                //         tree[new_parent].right = city;
                //     }
                //     tree[parent].parent = city;
                //     parent = new_parent;
                // } else {
                //     tree[parent].parent = city;
                //     parent = std::numeric_limits<size_t>::max(); // ルートノードになった場合
                // }
            }
            // if (has_parent() && (parent == left || parent == right)) {
            //     std::cerr << "Error: Parent is now a child of this node." << std::endl;
            //     exit(1);
            // }
            // if (parent == std::numeric_limits<size_t>::max()) {
            //     // ルートノードになった場合は自身をルートに設定
            //     std::cout << "Node " << city << " is now the root." << std::endl;
            // }
        }

        void splay(std::vector<Node>& tree_, bool parents_is_applied = false) {
            NodeVec tree{tree_};
            apply_reverse(tree, parents_is_applied);
            while (has_parent()) {
                if (!tree[parent].has_parent()) {
                    rotate(tree, true);
                } else {
                    bool is_left = is_left_child(tree, true);
                    bool parent_is_left = tree[parent].is_left_child(tree, true);
                    if (is_left == parent_is_left) {
                        // zig-zig
                        tree[parent].rotate(tree, true);
                        rotate(tree, true);
                    } else {
                        // zig-zag
                        rotate(tree, true);
                        rotate(tree, true);
                    }
                }
            }
        }
        
        // ルートの一つ下までSplayする
        void splay_subtree(std::vector<Node>& tree_, bool parents_is_applied = false) {
            NodeVec tree{tree_};
            apply_reverse(tree, parents_is_applied);
            while (has_parent() && tree[parent].has_parent()) {
                if (!tree[tree[parent].parent].has_parent()) {
                    rotate(tree, true);
                } else {
                    bool is_left = is_left_child(tree, true);
                    bool parent_is_left = tree[parent].is_left_child(tree, true);
                    if (is_left == parent_is_left) {
                        // zig-zig
                        tree[parent].rotate(tree, true);
                        rotate(tree, true);
                    } else {
                        // zig-zag
                        rotate(tree, true);
                        rotate(tree, true);
                    }
                }
            }
        }
        
        template <typename Func>
            requires std::invocable<Func, Node&>
        void for_each(std::vector<Node>& tree_, Func&& func, bool parents_is_applied = false) {
            NodeVec tree{tree_};
            apply_reverse(tree, parents_is_applied);
            if (left != std::numeric_limits<size_t>::max()) {
                tree[left].for_each(tree, func, true);
            }
            func(*this);
            if (right != std::numeric_limits<size_t>::max()) {
                tree[right].for_each(tree, func, true);
            }
        }
    };
    
    using NodeVec = Node::NodeVec;
    
    size_t get_next_city(std::vector<Node>& tree_, size_t current_city, size_t& root) {
        NodeVec tree{tree_};
        tree[current_city].splay(tree);
        root = current_city; // Splayしたノードをルートに設定
        size_t next_city = tree[current_city].get_next(tree);
        if (next_city == std::numeric_limits<size_t>::max()) {
            return tree[root].get_leftmost(tree);
        }
        
        return next_city;
    }

    size_t get_prev_city(std::vector<Node>& tree_, size_t current_city, size_t& root) {
        NodeVec tree{tree_};
        tree[current_city].splay(tree);
        root = current_city; // Splayしたノードをルートに設定
        size_t prev_city = tree[current_city].get_prev(tree);
        if (prev_city == std::numeric_limits<size_t>::max()) {
            return tree[root].get_rightmost(tree);
        }
        
        return prev_city;
    }

    // a -> b, c -> d のように接続されている
    void two_opt_swap(std::vector<Node>& tree_, size_t a, size_t b, size_t c, size_t d, size_t& root) {
        NodeVec tree{tree_};
        size_t next_a = tree[a].get_next(tree);
        if (b != next_a) { // b -> ? -> c -> d -> ? -> a 
                           // a -> b が端の場合
            tree[d].splay(tree);
            auto& reverse_range = tree[tree[d].left];
            reverse_range.reversed = !reverse_range.reversed;
            root = d;
            return;
        }
        size_t next_c = tree[c].get_next(tree);
        if (d != next_c) { // d -> ? -> a -> b -> ? -> c
                           // c -> d が端の場合
            tree[a].splay(tree);
            auto& reverse_range = tree[tree[a].right];
            reverse_range.reversed = !reverse_range.reversed;
            root = a;
            return;
        }

        tree[a].splay(tree);
        tree[d].splay_subtree(tree);
        root = a;
        
        if (tree[a].right == d) {
            // b ~ c の部分を逆順にする
            auto& reverse_range = tree[tree[d].left];
            reverse_range.reversed = !reverse_range.reversed;
        } else {
            // tree[c].splay(tree);
            // tree[b].splay_subtree(tree);
            // // d ~ a の部分を逆順にする
            // auto& reverse_range = tree[tree[b].left];
            // reverse_range.reversed = !reverse_range.reversed;
            // root = c;
            
            // ~ c と b ~ の部分を交換して逆順にする
            auto& a_node = tree[a];
            auto& d_node = tree[d];
            auto& b_seg = tree[a_node.right];
            auto& c_seg = tree[d_node.left];

            a_node.right = c_seg.city;
            c_seg.parent = a_node.city;
            
            d_node.left = b_seg.city;
            b_seg.parent = d_node.city;
            
            c_seg.reversed = !c_seg.reversed;
            b_seg.reversed = !b_seg.reversed;
            
            // if (get_next_city(tree, b, root) != d || get_next_city(tree, a, root) != c) {
            //     std::cerr << "Error: Two-opt swap failed to maintain connectivity." << std::endl;
            //     exit(1);
            // }
        }
    }

double time_a = 0.0;
}

namespace eax {

void print_2opt_time() {
    std::cout << "Time: " << time_a << " seconds" << std::endl;
}


TwoOpt::TwoOpt(const std::vector<std::vector<int64_t>> &distance_matrix, const std::vector<std::vector<std::pair<int64_t, size_t>>> &nearest_neighbors)
    : distance_matrix(distance_matrix), nearest_neighbors(nearest_neighbors)
{
    size_t n = distance_matrix.size();
    near_cities.resize(n);
    
    for (size_t i = 0; i < n; ++i) {
        for (size_t j = 0; j < n / 20; ++j) {
            auto& [distance, neighbor_index] = nearest_neighbors[i][j];
            near_cities[neighbor_index].push_back(i);
        }
    }
}

void TwoOpt::apply(std::vector<size_t>& path, std::mt19937::result_type seed)
{
    auto start_time = std::chrono::high_resolution_clock::now();
    std::mt19937 rng(seed);
    // 平衡二分木を構築
    std::vector<Node> tree;
    size_t n = path.size();
    tree.resize(n);
    for (size_t i = 0; i < n; ++i) {
        tree[i].city = i;
    }
    auto build_tree = [](auto& self, std::vector<Node>& tree, const std::vector<size_t>& path, size_t begin, size_t mid, size_t end) -> void{
        size_t mid_city = path[mid];
        size_t begin_mid = (begin + mid) / 2;
        size_t mid_end = (mid + end + 1) / 2;
        if (begin_mid < mid) {
            size_t left_child = path[begin_mid];
            tree[mid_city].left = left_child;
            tree[left_child].parent = mid_city;
            self(self, tree, path, begin, begin_mid, mid);
        }

        if (mid_end < end) {
            size_t right_child = path[mid_end];
            tree[mid_city].right = right_child;
            tree[right_child].parent = mid_city;
            self(self, tree, path, mid + 1, mid_end, end);
        }
    };
    
    size_t root = path[n / 2];
    build_tree(build_tree, tree, path, 0, n / 2, n);

    std::vector<uint8_t> is_active(n, true);
    
    std::uniform_int_distribution<size_t> dist(0, n - 1);
    bool improved = true;
    while (improved) {
        improved = false;
        size_t start = dist(rng);
        size_t prev_city = get_prev_city(tree, start, root);
        size_t current_city = start;
        do {
            size_t next_city = get_next_city(tree, current_city, root);
            if (!is_active[current_city]) {
                prev_city = current_city;
                current_city = next_city;
                continue;
            }

            for (size_t i = 0; i < n / 20; ++i) {
                size_t neighbor_city = nearest_neighbors[current_city][i].second;
                size_t neighbor_prev_city = get_prev_city(tree, neighbor_city, root);
                
                int64_t length_diff = distance_matrix[current_city][prev_city] - distance_matrix[current_city][neighbor_city];
                if (length_diff > 0) {
                    length_diff += distance_matrix[neighbor_city][neighbor_prev_city] - distance_matrix[prev_city][neighbor_prev_city];
                    if (length_diff > 0) {
                        // 2-optスワップする
                        two_opt_swap(tree, prev_city, current_city, neighbor_prev_city, neighbor_city, root);
                        std::array<size_t, 4> swap_cities = {prev_city, current_city, neighbor_prev_city, neighbor_city};

                        for (size_t city : swap_cities) {
                            for (auto neighbor : near_cities[city]) {
                                is_active[neighbor] = true;
                            }
                        }
                        improved = true;
                        break;
                    }
                } else break;
            }
            
            if (improved) break;

            for (size_t i = 0; i < n / 20; ++i) {
                size_t neighbor_city = nearest_neighbors[current_city][i].second;
                size_t neighbor_next_city = get_next_city(tree, neighbor_city, root);
                
                int64_t length_diff = distance_matrix[current_city][next_city] - distance_matrix[current_city][neighbor_city];
                if (length_diff > 0) {
                    length_diff += distance_matrix[neighbor_city][neighbor_next_city] - distance_matrix[next_city][neighbor_next_city];
                    if (length_diff > 0) {
                        // 2-optスワップする
                        two_opt_swap(tree, current_city, next_city, neighbor_city, neighbor_next_city, root);
                        std::array<size_t, 4> swap_cities = {current_city, next_city, neighbor_city, neighbor_next_city};
                        for (size_t city : swap_cities) {
                            for (auto neighbor : near_cities[city]) {
                                is_active[neighbor] = true;
                            }
                        }
                        improved = true;
                        break;
                    }
                } else break;
            }
            
            if (improved) break;
            
            is_active[current_city] = false;
            
            prev_city = current_city;
            current_city = next_city;

        } while (current_city != start);
    }

    // 最後に木を走査してパスを更新
    path.clear();
    tree[root].for_each(tree, [&path](Node& node) {
        path.push_back(node.city);
    });
    int64_t total_distance = 0;
    for (size_t i = 0; i < path.size(); ++i) {
        size_t next_index = (i + 1) % path.size();
        total_distance += distance_matrix[path[i]][path[next_index]];
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    time_a = std::chrono::duration<double>(end_time - start_time).count();
}
}
