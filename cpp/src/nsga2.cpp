#include "nsga2.h"
#include "graph.h"
#include "individual.h"
#include "utils.h"
#include <algorithm>
#include <cstddef>
#include <random>
#include <unordered_map>

const double eps = 1e-8;

using end_criteria::criterion_t; // callable terminating condition

namespace nsga2 {

    NSGA2::NSGA2(const size_t individual_size, const size_t objective_size,
                 const size_t population_size, const objective::fn_t &f, const double mutation_rate,
                 const uint32_t seed)
        : individual_size(individual_size), objective_size(objective_size),
          population_size(population_size), mutation_rate(mutation_rate), dist(mutation_rate),
          gen(seed), f(f) {
        individual_t dummy_individual(individual_size);
        std::cout << "Initializing NSGA2 with the following parameters:" << std::endl;
        std::cout << "Individual Size: " << individual_size << std::endl;
        std::cout << "Objective Size: " << objective_size << std::endl;
        std::cout << "Population Size: " << population_size << std::endl;
        std::cout << "Mutation Rate: " << mutation_rate << std::endl;
        std::cout << "Seed: " << seed << std::endl;
    }

    NSGA2::NSGA2(const size_t individual_size, const size_t objective_size,
                 const size_t population_size, const objective::fn_t &f, const uint32_t seed)
        : NSGA2(individual_size, objective_size, population_size, f, 1.0 / (double)individual_size,
                seed) {}

    void NSGA2::mutate(population_t &population) {
        population.resize(population_size * 2);
        for (int i = population_size; i < population_size * 2; i++) {
            population[i] = population[i - population_size];
            for (int j = 0; j < individual_size; j++) {
                if (generate_mutation_bit()) {
                    population[i][j] = !population[i][j];
                }
            }
        }
    }

    fronts_t NSGA2::non_dominate_sort(const population_t &population) {
        // TODO Performance improvements
        Graph<index_t> graph;
        size_t size = population.size();
        // O(N) N = population size
        for (index_t i = 0; i < size; i++)
            graph.addNode(i);
        // O(N^2) Is there a clever way to do this? e.g. dynamic pruning?
        // The graph could be dense here
        for (index_t i = 0; i < size; i++)
            for (index_t j = 0; j < size; j++)
                if (individual::strictly_dominates(population[i], population[j], f))
                    graph.addEdge(i, j);
        // O(N^2)
        return graph.popAndGetFronts();
    }

    /**
     * @brief Calculate the crowding distance for each individual in the front.
     *
     * @param population The total population.
     * @param front the list of indices of the individuals in the front.
     * @return scores_t
     */
    scores_t NSGA2::crowding_distance(const population_t &population, front_t &indices) {
        // TODO Test & Performance improvements
        size_t size = indices.size();
        assert(size > 0);
        scores_t distances;
        std::unordered_map<index_t, val_t> values;
        // initialize the distances
        for (index_t idx : indices) {
            distances[idx] = 0.0;
            values[idx] = f(population[idx]);
        }
        // for each objective
        for (size_t m = 0; m < objective_size; m++) {
            // sort the front based on the objective by ascending order of the values O(NlogN)
            std::sort(indices.begin(), indices.end(),
                      [&](index_t a, index_t b) { return values[a][m] < values[b][m]; });
            // set the boundary points to infinity
            distances[indices[0]] = std::numeric_limits<double>::infinity();
            distances[indices[size - 1]] = std::numeric_limits<double>::infinity();
            // Added eps to avoid division by zero
            double d = values[indices[size - 1]][m] - values[indices[0]][m] + eps;
            // O(N), recall that `distances` is a hash map
            for (size_t j = 1; j < size - 1; j++) {
                if (distances[indices[j]] == std::numeric_limits<double>::infinity())
                    continue;
                distances[indices[j]] +=
                    (values[indices[j + 1]][m] - values[indices[j - 1]][m]) / d;
            }
        }
        return distances;
    }

    /**
     * @brief returns the next generation of individuals based on the fronts.
     *
     * @param population
     * @param fronts
     * @return population_t
     */
    population_t NSGA2::crowding_distance_select(population_t &population, fronts_t &fronts) {
        // TODO Test & Performance improvements
        population_t new_population;
        size_t target_size = population_size;
        size_t front_idx = 0;

        // select low ranked fronts until the target size is reached
        for (front_idx = 0; front_idx < fronts.size(); front_idx++) {
            front_t &front = fronts[front_idx];
            if (new_population.size() + front.size() > target_size)
                break;
            for (index_t idx : front)
                new_population.push_back(std::move(population[idx]));
        }
        if (new_population.size() == target_size)
            return new_population;

        // crowding distance selection
        front_t &front = fronts[front_idx];
        // O(mNlogN) N is the size of the front, m is the number of objectives
        scores_t scores = std::move(crowding_distance(population, front));
        size_t remaining = target_size - new_population.size();
        // O(NlogN) in the worst case
        std::partial_sort(front.begin(), front.begin() + remaining, front.end(),
                          [&](index_t a, index_t b) { return scores[a] > scores[b]; });
        // O(N) in the worst case: select the individuals with the highest crowding distance
        for (size_t i = 0; i < remaining; i++) {
            new_population.push_back(std::move(population[front[i]]));
        }
        if (new_population.size() != target_size) {
            throw std::runtime_error("new_population.size() != target_size. "
                                     "check if there is a bug.");
        }
        return new_population;
    }

    /**
     * @brief init the population uniformly.
     *
     * @param individual_size
     * @param population_size
     */
    void NSGA2::init_population(const size_t individual_size,
                                const size_t population_size) { // Tested
        std::cout << "Initializing population" << std::endl;
        std::bernoulli_distribution distribution(0.5);
        population.resize(population_size);

        int mutation_cnt = 0;
        for (auto &individual : population) {
            individual.resize(individual_size);
            for (auto &gene : individual) {
                gene = distribution(this->gen);
                mutation_cnt += gene;
            }
        }
        std::cout << "Mutation success rate(~0.5): "
                  << (double)mutation_cnt / (individual_size * population_size) << std::endl;
    }

    population_t NSGA2::run(criterion_t criterion) {
        std::cout << "running NSGA2" << std::endl;
        std::cout << "mutation rate: " << mutation_rate << std::endl;
        std::cout << "individual size: " << individual_size << std::endl;
        std::cout << "population size: " << population_size << std::endl;
        std::cout << "objective size: " << objective_size << std::endl;

        init_population(individual_size, population_size);
        size_t iter = 0;
        fronts_t fronts;
        while (!criterion(population, iter)) {
            mutate(population);
            fronts = std::move(non_dominate_sort(population));
            population = std::move(crowding_distance_select(population, fronts));
            iter++;
        }
        return population;
    }

    uint8_t NSGA2::generate_mutation_bit() {
        mutation_attempts++;
        if (dist(gen)) {
            successful_mutations++;
            return 1;
        }
        return 0;
    }

    double NSGA2::mutation_ratio() {
        return (double)successful_mutations / (mutation_attempts + eps);
    }
} // namespace nsga2