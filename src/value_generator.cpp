#include "value_generator.hpp"

namespace PiBench
{
thread_local uint32_t value_generator_t::seed_;
thread_local foedus::assorted::UniformRandom value_generator_t::uni_dist;
}