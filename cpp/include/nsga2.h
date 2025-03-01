#pragma once

#include "individual.h"
#include "utils.h"
#include <cstddef>
#include <cstdint>
#include <random>
#include <unordered_map>

/**
 * @namespace nsga2
 * @brief Namespace for NSGA-II (Non-dominated Sorting Genetic Algorithm II)
 * implementation.
 */
namespace nsga2 {

    using individual_t = individual::individual_t; // a bitarray
    using population_t = individual::population_t; // vector of individuals
    using fn_t = objective::fn_t;                  // objective function
    using val_t = objective::val_t;                // value type

    using index_t = std::size_t;                          // index of an individual in a population
    using rank_t = std::size_t;                           // rank of an individual in a population
    using front_t = std::vector<index_t>;                 // front of the same rank
    using fronts_t = std::vector<front_t>;                // list of fronts
    using scores_t = std::unordered_map<index_t, double>; // crowding distance

    using criterion_t = end_criteria::criterion_t; // callable termination condition

    class NSGA2 {
      public:
        /**
         * @brief NSGA2 constructor.
         */
        NSGA2(const size_t individual_size, const size_t objective_size,
              const size_t population_size, const fn_t &f, const double mutation_rate,
              const uint32_t seed);

        /**
         * @brief NSGA2 constructor with mutation rate set to 1/population_size.
         */
        NSGA2(const size_t individual_size, const size_t objective_size,
              const size_t population_size, const fn_t f,
              const uint32_t seed = std::random_device()());

        /**
         * @brief Run the NSGA-II algorithm.
         * 
         * @details Equivalent pseudo-code:
         * ```
         * init_population()
         * while (termination criteria not met) {
         *   population = mutate(population)
         *   ranks = non_dominated_sort(population)
         *   select(population, ranks)
         * }
         * ```
         */
        population_t run(criterion_t criterion);

        // Note: A destructor is not necessary since all objects are stack
        // allocated.

      private:
        const size_t individual_size;
        const size_t population_size;
        const size_t objective_size;
        const double mutation_rate;
        const fn_t f;

        population_t population;

        /**
         * @brief init the population uniformly.
         *
         * @param individual_size
         * @param population_size
         */
        void init_population(const size_t individual_size, const size_t population_size);

        void mutate(population_t &population);

        fronts_t non_dominated_sort(const population_t &population);

        /**
         * @brief Calculate the crowding distance for each individual in the front.
         *
         * @param population The total population.
         * @param front the list of indices of the individuals in the front.
         * @return scores_t
         */
        scores_t crowding_distance(const population_t &population, front_t &indices);

        /**
         * @brief returns the next generation of individuals based on the fronts.
         *
         * @param population
         * @param fronts
         * @return population_t
         */
        population_t crowding_distance_select(population_t &population, fronts_t &fronts);

        // Random number generator
        std::mt19937 gen;
        std::bernoulli_distribution dist;

        // Random bit generator: Bernoulli(mutation_rate)
        // Count of successful mutations
        int successful_mutations = 0;
        // Total number of mutation attempts
        int mutation_attempts = 0;

        uint8_t generate_mutation_bit();

        // for sanity check
        double mutation_ratio();
    };
} // namespace nsga2