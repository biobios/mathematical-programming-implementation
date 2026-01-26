#include "two_opt.hpp"

#include <iostream>
#include <numeric>
#include <array>
#include <chrono>

namespace {
    struct Node {
        size_t city;
        Node* parent = nullptr;
        Node* left = nullptr;
        Node* right = nullptr;
        bool reversed = false;

        bool has_parent() const {
            return parent != nullptr;
        }
        
        /**
         * @pre 任意の祖先ancestorについて !ancestor.reversed であること
         * @post this->!reversed
         */
        void apply_reverse_single() {
            if (reversed) {
                reversed = false;
                std::swap(left, right);
                if (left != nullptr) {
                    left->reversed = !left->reversed;
                }
                if (right != nullptr) {
                    right->reversed = !right->reversed;
                }
            }
        }
        
        /**
         * @post path にはこのノードからルートまでのパスが格納される
         * @param path ノードのポインタを格納するベクター
         */
        void path_to_root(std::vector<Node*>& path) {
            path.clear();
            Node* current = this;
            while (current != nullptr) {
                path.push_back(current);
                current = current->parent;
            }
        }
        
        /**
         * @post this->!reversed
         * @post 任意の祖先ancestorについて !ancestor.reversed となる
         * @post working_memory にはこのノードからルートまでのパスが格納される
         * @param working_memory ノードのポインタを格納するベクター
         */
        void apply_reverse(std::vector<Node*>& working_memory) {
            path_to_root(working_memory);
            for (auto it = working_memory.rbegin(); it != working_memory.rend(); ++it) {
                (*it)->apply_reverse_single();
            }
        }
        
        /**
         * @post this->!reversed
         * @post 任意の祖先ancestorについて !ancestor.reversed となる
         */
        void apply_reverse() {
            std::vector<Node*> working_memory;
            apply_reverse(working_memory);
        }
        
        /**
         * @pre 任意の祖先ancestorについて !ancestor.reversed であること
         * @return もし自身が左子ノードならばtrue, そうでなければfalse
         */
        bool is_left_child() {
            return has_parent() && parent->left == this;
        }
        
        /**
         * @pre 任意の祖先ancestorについて !ancestor.reversed であること
         * @return もし自身が右子ノードならばtrue, そうでなければfalse
         */
        bool is_right_child() {
            return has_parent() && parent->right == this;
        }
        
        /**
         * @pre 任意の祖先ancestorについて !ancestor.reversed であること
         * @return 自身よりも右側に存在する祖先のノードのポインタ
         */
        Node* get_right_ancestor() {
            Node* current = this;
            while (current->has_parent()) {
                if (current->is_left_child()) {
                    return current->parent;
                }
                current = current->parent;
            }
            return nullptr;
        }
        
        /**
         * @pre 任意の祖先ancestorについて !ancestor.reversed であること
         * @return 自身よりも左側に存在する祖先のノードのポインタ
         */
        Node* get_left_ancestor() {
            Node* current = this;
            while (current->has_parent()) {
                if (current->is_right_child()) {
                    return current->parent;
                }
                current = current->parent;
            }
            return nullptr;
        }
        
        /**
         * @pre 任意の祖先ancestorについて !ancestor.reversed であること
         * @pre !this->reversed
         * @return 右隣りのノードのポインタ
         */
        Node* get_next() {
            if (right != nullptr) {
                right->apply_reverse_single();
                return right->get_leftmost();
            }
            return get_right_ancestor();
        }
        
        /**
         * @pre 任意の祖先ancestorについて !ancestor.reversed であること
         * @pre !this->reversed
         * @return 左隣りのノードのポインタ
         */
        Node* get_prev() {
            if (left != nullptr) {
                left->apply_reverse_single();
                return left->get_rightmost();
            }
            return get_left_ancestor();
        }
        
        /**
         * @pre 任意の祖先ancestorについて !ancestor.reversed であること
         * @pre !this->reversed
         * @return 自身の子孫の中で最も左端のノードのポインタ
         */
        Node* get_leftmost() {
            Node* current = this;
            while (current->left != nullptr) {
                current = current->left;
                current->apply_reverse_single();
            }
            return current;
        }
        
        /**
         * @pre 任意の祖先ancestorについて !ancestor.reversed であること
         * @pre !this->reversed
         * @return 自身の子孫の中で最も右端のノードのポインタ
         */
        Node* get_rightmost() {
            Node* current = this;
            while (current->right != nullptr) {
                current = current->right;
                current->apply_reverse_single();
            }
            return current;
        }
        
        /**
         * @pre 任意の祖先ancestorについて !ancestor.reversed であること
         * @pre !this->reversed
         * @post 親が存在する場合、回転を行う
         */
        void rotate() {
            if (!has_parent()) {
                return; // 親がいない場合は回転できない
            }

            auto& parent_node = *parent;
            if (is_left_child()) {
                // 左子ノードならば右に回転
                if (right != nullptr) {
                    auto& right_node = *right;
                    if (parent_node.has_parent()) {
                        auto& grandparent_node = *parent_node.parent;
                        if (parent_node.is_left_child()) {
                            grandparent_node.left = this;
                        } else {
                            grandparent_node.right = this;
                        }
                        parent_node.left = &right_node;
                        right = &parent_node;

                        parent = &grandparent_node;
                        parent_node.parent = this;
                        right_node.parent = &parent_node;
                    } else {
                        parent_node.left = &right_node;
                        right = &parent_node;
                        parent = nullptr; // ルートノードになった場合
                        parent_node.parent = this;
                        right_node.parent = &parent_node;
                    }
                } else {
                    if (parent_node.has_parent()) {
                        auto& grandparent_node = *parent_node.parent;
                        if (parent_node.is_left_child()) {
                            grandparent_node.left = this;
                        } else {
                            grandparent_node.right = this;
                        }
                        parent_node.left = nullptr; // 左子ノード
                        right = &parent_node;
                        parent = &grandparent_node;
                        parent_node.parent = this;
                    } else {
                        parent_node.left = nullptr; // 左子ノード
                        right = &parent_node;
                        parent = nullptr; // ルートノードになった場合
                        parent_node.parent = this;
                    }
                }
            } else {
                // 右子ノードならば左に回転
                if (left != nullptr) {
                    auto& left_node = *left;

                    if (parent_node.has_parent()) {
                        auto& grandparent_node = *parent_node.parent;
                        if (parent_node.is_left_child()) {
                            grandparent_node.left = this;
                        } else {
                            grandparent_node.right = this;
                        }
                        parent_node.right = &left_node;
                        left = &parent_node;

                        parent = &grandparent_node;
                        parent_node.parent = this;
                        left_node.parent = &parent_node;
                    } else {
                        parent_node.right = &left_node;
                        left = &parent_node;
                        parent = nullptr; // ルートノードになった場合
                        parent_node.parent = this;
                        left_node.parent = &parent_node;
                    }
                } else {
                    if (parent_node.has_parent()) {
                        auto& grandparent_node = *parent_node.parent;
                        if (parent_node.is_left_child()) {
                            grandparent_node.left = this;
                        } else {
                            grandparent_node.right = this;
                        }
                        parent_node.right = nullptr; // 右子ノード
                        left = &parent_node;
                        parent = &grandparent_node;
                        parent_node.parent = this;
                    } else {
                        parent_node.right = nullptr; // 右子ノード
                        left = &parent_node;
                        parent = nullptr; // ルートノードになった場合
                        parent_node.parent = this;
                    }
                }
            }
        }
    };
    
    class PathTree {
    public:
        PathTree(const std::vector<size_t>& path) {
            size_t n = path.size();
            nodes.resize(n);
            for (size_t i = 0; i < n; ++i) {
                nodes[i].city = i;
            }
            auto build_tree = [this, &path](auto& self, size_t begin, size_t mid, size_t end) -> void {
                size_t mid_city = path[mid];
                size_t begin_mid = (begin + mid) / 2;
                size_t mid_end = (mid + end + 1) / 2;
                if (begin_mid < mid) {
                    size_t left_child = path[begin_mid];
                    nodes[mid_city].left = &nodes[left_child];
                    nodes[left_child].parent = &nodes[mid_city];
                    self(self, begin, begin_mid, mid);
                }

                if (mid_end < end) {
                    size_t right_child = path[mid_end];
                    nodes[mid_city].right = &nodes[right_child];
                    nodes[right_child].parent = &nodes[mid_city];
                    self(self, mid + 1, mid_end, end);
                }
            };
            build_tree(build_tree, 0, n / 2, n);
            root = path[n / 2];
        }
        
        void splay(size_t city) {
            splay_subtree(city, 0);
            root = city;
        }
        
        // 指定された高さまでSplayする
        void splay_subtree(size_t city, size_t height) {
            nodes[city].apply_reverse(working_memory);
            size_t current_height = working_memory.size() - 1;
            Node* splay_target = &nodes[city];
            while (height < current_height) { // 目的の高さよりも高い場合
                if (current_height == height + 1) { // 祖父母がいない場合(2段上ると目的の高さを超えてしまうとき)
                    splay_target->rotate();
                    current_height -= 1;
                    break;
                } else {
                    bool is_left = splay_target->is_left_child();
                    bool parent_is_left = splay_target->parent->is_left_child();
                    if (is_left == parent_is_left) {
                        // zig-zig
                        splay_target->parent->rotate();
                        splay_target->rotate();
                    } else {
                        // zig-zag
                        splay_target->rotate();
                        splay_target->rotate();
                    }
                    current_height -= 2;
                }
            }
        }
        
        size_t get_next(size_t city) {
            splay(city);
            Node* next_city = nodes[city].get_next();
            if (next_city == nullptr) {
                return nodes[root].get_leftmost()->city;
            }
            return next_city->city;
        }
        
        size_t get_prev(size_t city) {
            splay(city);
            Node* prev_city = nodes[city].get_prev();
            if (prev_city == nullptr) {
                return nodes[root].get_rightmost()->city;
            }
            return prev_city->city;
        }
        
        void reverse_range(size_t prev_L, size_t next_R) {
            splay(prev_L);
            splay_subtree(next_R, 1);
            auto& prev_L_node = nodes[prev_L];
            auto& next_R_node = nodes[next_R];

            if (prev_L_node.right == &next_R_node) {
                // ? - prev_L - L - ? - R - next_R - ? のつながり方のとき
                // L ~ R の部分を逆順にする
                auto& reverse_range = *next_R_node.left;
                reverse_range.reversed = !reverse_range.reversed;
            } else {
                if (prev_L_node.right == nullptr) {
                    // L - ? - R - next_R - ? - prev_L のつながり方の時
                    // L - ? - R の部分を逆順にする
                    auto& reverse_range = *next_R_node.left;
                    reverse_range.reversed = !reverse_range.reversed;
                } else if (next_R_node.left == nullptr) {
                    // next_R - ? - prev_L - L - ? - R のつながり方の時
                    // L - ? - R の部分を逆順にする
                    auto& reverse_range = *prev_L_node.right;
                    reverse_range.reversed = !reverse_range.reversed;
                } else {
                    // ? - R - next_R - ? - prev_L - L - ? のつながり方の時
                    // ? - R と L - ? の部分を交換して逆順にする
                    auto& L_seg = *prev_L_node.right;
                    auto& R_seg = *next_R_node.left;

                    prev_L_node.right = &R_seg;
                    R_seg.parent = &prev_L_node;
                    
                    next_R_node.left = &L_seg;
                    L_seg.parent = &next_R_node;
                    
                    R_seg.reversed = !R_seg.reversed;
                    L_seg.reversed = !L_seg.reversed;
                }
            }
        }
        
        /**
         * @pre func は PathTree の状態を変更しないこと
         */
        template <typename Func>
            requires std::invocable<Func, Node&>
        void for_each(Func&& func) {
            working_memory.clear();
            working_memory.push_back(&nodes[root]);
            while (!working_memory.empty()) {
                Node* current = working_memory.back();
                current->apply_reverse_single();
                if (current->left != nullptr) {
                    working_memory.push_back(current->left);
                } else {
                    do {
                        if (working_memory.empty())
                            return;
                        current = working_memory.back(); // 最初の一回は current == working_memory.back() になる
                        func(*current);
                        working_memory.pop_back();
                    } while (current->right == nullptr);
                    // 右子ノードが存在しないノードを実行しながら、スタックを戻っていく
                    working_memory.push_back(current->right);
                }
            }
        }
        
    private:
        std::vector<Node> nodes;
        std::vector<Node*> working_memory;
        size_t root;
    };

    double time_a = 0.0;
    
    void apply_neighbor_2opt(
        std::vector<size_t>& path,
        const std::vector<std::vector<int64_t>>& distance_matrix,
        const std::vector<std::vector<std::pair<int64_t, size_t>>>& nearest_neighbors,
        const std::vector<std::vector<size_t>>& near_cities,
        size_t near_range,
        std::mt19937::result_type seed
    ){
        std::mt19937 rng(seed);
        // 平衡二分木を構築
        const size_t n = path.size();
        PathTree tree(path);

        std::vector<uint8_t> is_active(n, true);
        
        std::uniform_int_distribution<size_t> dist(0, n - 1);
        bool improved = true;
        while (improved) {
            improved = false;
            size_t start = dist(rng);
            size_t prev_city = tree.get_prev(start);
            size_t current_city = start;
            do {
                size_t next_city = tree.get_next(current_city);
                if (!is_active[current_city]) {
                    prev_city = current_city;
                    current_city = next_city;
                    continue;
                }

                for (size_t i = 0; i < near_range; ++i) {
                    size_t neighbor_city = nearest_neighbors[current_city][i].second;
                    size_t neighbor_prev_city = tree.get_prev(neighbor_city);
                    
                    int64_t length_diff = distance_matrix[current_city][prev_city] - distance_matrix[current_city][neighbor_city];
                    if (length_diff > 0) {
                        length_diff += distance_matrix[neighbor_city][neighbor_prev_city] - distance_matrix[prev_city][neighbor_prev_city];
                        if (length_diff > 0) {
                            // 2-optスワップする
                            tree.reverse_range(prev_city, neighbor_city);
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

                for (size_t i = 0; i < near_range; ++i) {
                    size_t neighbor_city = nearest_neighbors[current_city][i].second;
                    size_t neighbor_next_city = tree.get_next(neighbor_city);
                    
                    int64_t length_diff = distance_matrix[current_city][next_city] - distance_matrix[current_city][neighbor_city];
                    if (length_diff > 0) {
                        length_diff += distance_matrix[neighbor_city][neighbor_next_city] - distance_matrix[next_city][neighbor_next_city];
                        if (length_diff > 0) {
                            // 2-optスワップする
                            tree.reverse_range(current_city, neighbor_next_city);
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
        tree.for_each([&path](Node& node) {
            path.push_back(node.city);
        });
        
    }
    
    void apply_global_2opt(
        std::vector<size_t>& path,
        const std::vector<std::vector<int64_t>>& distance_matrix,
        const std::vector<std::vector<std::pair<int64_t, size_t>>>& nearest_neighbors,
        std::mt19937::result_type seed
    ) {
        std::mt19937 rng(seed);
        // 平衡二分木を構築
        const size_t n = path.size();
        const size_t NN_list_size = nearest_neighbors[0].size();
        PathTree tree(path);
        
        std::uniform_int_distribution<size_t> dist(0, n - 1);
        bool improved = true;
        while (improved) {
            improved = false;
            size_t start = dist(rng);
            size_t prev_city = tree.get_prev(start);
            size_t current_city = start;
            do {
                for (size_t i = 0; i < NN_list_size; ++i) {
                    size_t neighbor_city = nearest_neighbors[current_city][i].second;
                    size_t neighbor_prev_city = tree.get_prev(neighbor_city);
                    
                    int64_t length_diff = distance_matrix[current_city][prev_city] - distance_matrix[current_city][neighbor_city];
                    if (length_diff > 0) {
                        length_diff += distance_matrix[neighbor_city][neighbor_prev_city] - distance_matrix[prev_city][neighbor_prev_city];
                        if (length_diff > 0) {
                            // 2-optスワップする
                            tree.reverse_range(prev_city, neighbor_city);

                            improved = true;
                            break;
                        }
                    } else break;
                }
                
                if (improved) break;
                size_t next_city = tree.get_next(current_city);

                for (size_t i = 0; i < NN_list_size; ++i) {
                    size_t neighbor_city = nearest_neighbors[current_city][i].second;
                    size_t neighbor_next_city = tree.get_next(neighbor_city);
                    
                    int64_t length_diff = distance_matrix[current_city][next_city] - distance_matrix[current_city][neighbor_city];
                    if (length_diff > 0) {
                        length_diff += distance_matrix[neighbor_city][neighbor_next_city] - distance_matrix[next_city][neighbor_next_city];
                        if (length_diff > 0) {
                            // 2-optスワップする
                            tree.reverse_range(current_city, neighbor_next_city);

                            improved = true;
                            break;
                        }
                    } else break;
                }
                
                if (improved) break;
                
                prev_city = current_city;
                current_city = next_city;

            } while (current_city != start);
        }

        // 最後に木を走査してパスを更新
        path.clear();
        tree.for_each([&path](Node& node) {
            path.push_back(node.city);
        });
        
    }

    void inversion(std::vector<size_t>& tour, std::vector<size_t>& pos, size_t left, size_t right) {
        if(left <= right) {
            for(size_t i = 0; i < (right - left + 1) / 2; ++i) {
                std::swap(tour[left + i], tour[right - i]);
                pos[tour[left + i]] = left + i;
                pos[tour[right - i]] = right - i;
            }
        } else {
            for(size_t i = 0; i < (tour.size() - (left - right) + 1) / 2; ++i) {
                std::swap(tour[(left + i) % tour.size()], tour[(right - i + tour.size()) % tour.size()]);
                pos[tour[(left + i) % tour.size()]] = (left + i) % tour.size();
                pos[tour[(right - i + tour.size()) % tour.size()]] = (right - i + tour.size()) % tour.size();
            }
        }
    }

    void apply_soft_2opt(
        std::vector<size_t>& path,
        const std::vector<std::vector<int64_t>>& distance_matrix,
        const std::vector<std::vector<std::pair<int64_t, size_t>>>& nearest_neighbors,
        size_t near_range
    ) {
        const size_t n = path.size();
        std::vector<size_t> tour = path;
        std::vector<size_t> pos(n);
        for (size_t i = 0; i < n; ++i) {
            pos[tour[i]] = i;
        }

        double improve = 1.;
        while (improve > 0.) {
            improve = 0.;
            for(size_t i = 1; i < n; ++i) {
                std::pair<size_t, size_t> swap_pos;
                size_t u1 = tour[i];
                size_t v1 = tour[(i + 1) % n];
                for(size_t d = 0; d < near_range; ++d) {
                    size_t u2 = tour[pos[nearest_neighbors[u1][d].second]];
                    size_t v2 = tour[(pos[nearest_neighbors[u1][d].second] + 1) % n];
                    if((distance_matrix[u1][v1] + distance_matrix[u2][v2]) - (distance_matrix[u1][u2] + distance_matrix[v1][v2]) > improve) {
                        improve = (distance_matrix[u1][v1] + distance_matrix[u2][v2]) - (distance_matrix[u1][u2] + distance_matrix[v1][v2]);
                        swap_pos = std::make_pair((i + 1) % n, pos[nearest_neighbors[u1][d].second]);
                    }
                }
                if(swap_pos != std::pair<size_t, size_t>()) {
                    inversion(tour, pos, swap_pos.first, swap_pos.second);
                }
            }
        }
        path = tour;
    }
}

namespace eax {

void print_2opt_time() {
    std::cout << "Time: " << time_a << " seconds" << std::endl;
}

TwoOpt::TwoOpt(const std::vector<std::vector<int64_t>> &distance_matrix, const std::vector<std::vector<std::pair<int64_t, size_t>>> &nearest_neighbors, size_t near_range)
    : distance_matrix(distance_matrix), nearest_neighbors(nearest_neighbors), near_range(std::min(near_range, nearest_neighbors[0].size()))
{
    size_t n = distance_matrix.size();
    if (near_range >= nearest_neighbors[0].size()) { // 近傍の範囲が距離行列のサイズを超える場合は、近傍はすべての都市なので
                                                     // 近傍の都市のベクターを作る必要はない
        return;
    }
    near_cities.resize(n);
    for (size_t i = 0; i < n; ++i) {
        for (size_t j = 0; j < near_range; ++j) {
            auto& [distance, neighbor_index] = nearest_neighbors[i][j];
            near_cities[neighbor_index].push_back(i);
        }
    }
}

void TwoOpt::apply(std::vector<size_t>& path, std::mt19937::result_type seed)
{
    auto start_time = std::chrono::high_resolution_clock::now();

    if (near_range >= nearest_neighbors[0].size()) {
        apply_global_2opt(path, distance_matrix, nearest_neighbors, seed);
    } else {
        apply_neighbor_2opt(path, distance_matrix, nearest_neighbors, near_cities, near_range, seed);
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    time_a += std::chrono::duration<double>(end_time - start_time).count();
}

//TODO: SoftTwoOptの実装
SoftTwoOpt::SoftTwoOpt(const std::vector<std::vector<int64_t>>& distance_matrix, const std::vector<std::vector<std::pair<int64_t, size_t>>>& nearest_neighbors, size_t near_range)
    : distance_matrix(distance_matrix), nearest_neighbors(nearest_neighbors), near_range(near_range)
{}

void SoftTwoOpt::apply(std::vector<size_t>& path)
{
    auto start_time = std::chrono::high_resolution_clock::now();

    apply_soft_2opt(path, distance_matrix, nearest_neighbors, near_range);

    auto end_time = std::chrono::high_resolution_clock::now();
    time_a += std::chrono::duration<double>(end_time - start_time).count();
}

}
