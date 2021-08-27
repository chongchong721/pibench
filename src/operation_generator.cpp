#include "operation_generator.hpp"

namespace PiBench
{
thread_local uint32_t operation_generator_t::seed_;
thread_local foedus::assorted::UniformRandom operation_generator_t::uni_dist;
}