#pragma once

#include "eax_normal.hpp"
#include "tabu_ab_cycle_finder.hpp"

#include "eax_rand.hpp"
#include "eax_n_ab.hpp"
#include "eax_uniform.hpp"

namespace eax {

template <typename E_Set_Assembler_Builder, typename Subtour_Merger = SubtourMerger>
using EAX_tabu = EAX_normal<E_Set_Assembler_Builder, Subtour_Merger, TabuABCycleFinder>;

using EAX_tabu_Rand = EAX_tabu<Rand_e_set_assembler_builder>;
using EAX_tabu_N_AB = EAX_tabu<N_AB_e_set_assembler_builder>;
using EAX_tabu_UNIFORM = EAX_tabu<uniform_e_set_assembler_builder>;

}