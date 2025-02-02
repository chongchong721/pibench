#ifndef __KEY_GENERATOR_HPP__
#define __KEY_GENERATOR_HPP__

#include "selfsimilar_int_distribution.hpp"
#include "zipfian_int_distribution.hpp"

#include <cmath>
#include <cstdint>
#include <cstring>
#include <random>
#include <string>

namespace PiBench
{

/**
 * @brief Class used to generate random keys of a given size belonging to a
 * keyspace of given size.
 *
 * The generated keys are composed of two parts:
 * |----- prefix (optional) -----||---- id -----|
 * if time-based, the keys are in three parts
 * |----- prefix (optional) -----||---- tid ----||---- id -----|
 * The generated 'ids' are 8 Byte unsigned integers. The 'ids' are then hashed
 * to scramble the keys across the keyspace.
 *
 * If the specified key size is smaller than 8 Bytes, the higher bits are
 * discarded. If the specified key size is larger than 8 Bytes, zeroes are
 * preppended to match the size.
 *
 */
class key_generator_t
{
public:
    /**
     * @brief Construct a new key_generator_t object
     *
     * @param N size of key space.
     * @param size size in Bytes of keys to be generated (excluding prefix).
     * @param benchmark_mode PiBench's running mode(operation based / time based)
     * @param prefix prefix to be prepended to every key.
     */
    key_generator_t(size_t N, size_t size, uint16_t thread_num, bool tid_prefix, const std::string& prefix = "");

    virtual ~key_generator_t() = default;

    /**
     * @brief Generate next key in op mode.
     *
     * Setting 'in_sequence' to true is useful when initially loading the data
     * structure, so no repeated keys are generated and we can guarantee the
     * amount of unique records inserted.
     *
     * Finally, a pointer to buf_ is returned. Since next() overwrites buf_, the
     * pointer returned should not be used across calls to next().
     *
     * @param in_sequence if @true, keys are generated in sequence,
     *                    if @false keys are generated randomly.
     * @return const char* pointer to beginning of key.
     */
    virtual const char* next(bool negative_access, bool in_sequence = false) final;

    /**
     * @brief Generate next key in time based mode
     *
     * Setting 'in_sequence' to true is useful when initially loading the data
     * structure, so no repeated keys are generated and we can guarantee the
     * amount of unique records inserted.
     *
     * Finally, a pointer to buf_ is returned. Since next() overwrites buf_, the
     * pointer returned should not be used across calls to next().
     *
     * @param in_sequence if @true, keys are generated in sequence,
     *                    if @false keys are generated randomly.
     * @param tid is thread id, which acted as a prefix of the key
     * @param negative_access is used to generate id without range
     * @return const char* pointer to beginning of key.
     */
    virtual const char* next(uint8_t tid, bool negative_access, bool in_sequence = false) final;

    /**
     * @brief Returns total key size (including prefix and tid(time-based mode)).
     *
     * @return size_t
     */
    size_t size() const noexcept { return tid_prefix ? prefix_.size() + size_ + 1 : prefix_.size() + size_ ; }

    /**
     * @brief Returns size of keyspace in number of elements..
     *
     * @return size_t
     */
    size_t keyspace() const noexcept { return N_; }

    /**
     * @brief Set the seed object.
     *
     * @param seed
     */
    static void set_seed(uint32_t seed)
    {
        seed_ = seed;
        generator_.seed(seed_);
    }

    /**
     * @brief Get the seed object.
     *
     * @return uint32_t
     */
    static uint32_t get_seed() noexcept { return seed_; }

    static constexpr uint32_t KEY_MAX = 128;

    static thread_local uint64_t current_id_;

    /// Storing the number of inserts with different thread ID (used as current ID)
    uint64_t* thread_stat;

protected:
    virtual uint64_t next_id() = 0;
    virtual uint64_t next_id(uint64_t upper_bound) = 0;

    /// Engine used for generating random numbers.
    static thread_local std::default_random_engine generator_;

private:
    /// Format generated ID
    void bits_shift(char *buf_ptr, uint64_t id);

    /// Seed used for generating random numbers.
    static thread_local uint32_t seed_;

    /// Space to materialize the keys (avoid allocation).
    static thread_local char buf_[KEY_MAX];

    /// Size of keyspace to generate keys.
    const size_t N_;

    /// Size in Bytes of keys to be generated (excluding prefix).
    const size_t size_;

    /// Prefix to be preppended to every key.
    const std::string prefix_;

    //uint64_t current_id_ = 0;

    /// Flag: whether tid become a part of the prefix
    bool tid_prefix;

};

class uniform_key_generator_t final : public key_generator_t
{
public:
    uniform_key_generator_t(size_t N, size_t size, uint16_t thread_num, bool tid_prefix, const std::string& prefix = "")
        : dist_(1, N),
          key_generator_t(N, size, thread_num, tid_prefix, prefix) {}

protected:
    virtual uint64_t next_id() override
    {
        return dist_(generator_);
    }

    virtual uint64_t next_id(uint64_t upper_bound) override
    {
        std::uniform_int_distribution<uint64_t> tmp_dist(1,upper_bound);
        return tmp_dist(generator_);
    }

private:
    std::uniform_int_distribution<uint64_t> dist_;
};

class selfsimilar_key_generator_t final : public key_generator_t
{
public:
    selfsimilar_key_generator_t(size_t N, size_t size, uint16_t thread_num, bool tid_prefix, const std::string& prefix = "", float skew = 0.2)
        : dist_(1, N, skew),
          key_generator_t(N, size, thread_num, tid_prefix, prefix),
          skew_(skew)
    {
    }

    virtual uint64_t next_id() override
    {
        return dist_(generator_);
    }

    virtual uint64_t next_id(uint64_t upper_bound) override
    {
        selfsimilar_int_distribution<uint64_t> tmp_dist(1,upper_bound,skew_);
        return tmp_dist(generator_);
    }

private:
    selfsimilar_int_distribution<uint64_t> dist_;
    float skew_;
};

class zipfian_key_generator_t final : public key_generator_t
{
public:
    zipfian_key_generator_t(size_t N, size_t size, uint16_t thread_num, bool tid_prefix, const std::string& prefix = "", float skew = 0.99)
        : dist_(1, N, skew),
          key_generator_t(N, size, thread_num, tid_prefix, prefix),
          skew_(skew)
    {
    }

    virtual uint64_t next_id() override
    {
        return dist_(generator_);
    }

    virtual uint64_t next_id(uint64_t upper_bound) override
    {
        zipfian_int_distribution<uint64_t> tmp_dist(1,upper_bound,skew_);
        return tmp_dist(generator_);
    }
private:
    zipfian_int_distribution<uint64_t> dist_;
    float skew_;
};
} // namespace PiBench
#endif