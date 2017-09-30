#ifndef ACMAP_H
#define ACMAP_H

#include <utility>
#include <algorithm>
#include <functional>

#include <set>
#include <vector>
#include <array>
#include <queue>

#define CHAR_START ('a')
#define CHAR_END ('z')
#define CHAR_SIZE (CHAR_END - CHAR_START + 1)

#define init_state (0)

#define DEFAULT_RESERVE_SIZE (65536)


namespace ahocorasick {
    enum State {
        final = -1, init, normal, out = 10, terminal
    };

    using index_type = int;
    using index_unsinged_type = size_t;
    using element_type = char;                                  // element type
    using pattern_type = std::string;                           // pattern type
    using node_type = std::array<index_type, CHAR_SIZE>;        // node type
    using block_type = std::vector<node_type>;                  // block type
    using multi_index = std::pair<index_type, element_type>;    // multi indexer
    using result_type = std::queue<int>;                        // result type

    inline bool operator==(const index_type& index, State state) {
        if (index == 0 || state == init)
            return index == 0 && state == init;
        return !((state > 0) ^ (index > 0));
    }

    inline bool operator!=(const index_type& index, State state) {
        if (index == 0 || state == init)
            return index != 0 || state != init;
        return (state > 0) ^ (index > 0);
    }

    class Map {
    public:

        class tracer {
        public:
            using value_type = multi_index;
            using pattern_reference = element_type*;
            using pointer = value_type*;
            using reference = value_type&;

            tracer(Map& map, const element_type * pattern, const index_type state = init_state)
                : map(map), pattern(pattern), state(state) {}
            tracer& operator++() {
                state = map.const_at(state, *pattern);
                if (!(*pattern)) {
                    pattern = '\0';
                }
                else {
                    pattern += 1;
                }

                return *this;
            }
            tracer operator++(int temp) {
                tracer prev = *this;
                ++(*this);
                return prev;
            }
            index_type operator*() {
                return state;
            }
            inline bool operator==(const State& _state) {
                switch (_state) {
                case out:
                    return pattern == '\0' || state == State::init;
                case terminal:
                    return *pattern == '\0' && state == State::final;
                default:
                    return state == _state;
                }
            }
            inline bool operator!=(const State& _state) {
                switch (_state) {
                case out:
                    return pattern != '\0' && state != State::init;
                case terminal:
                    return *pattern != '\0' || state != State::final;
                default:
                    return state != _state;
                }
            }
            inline bool operator==(const tracer& rhs) {
                return &map == &rhs.map && pattern == rhs.pattern && state == rhs.state;
            }
            inline bool operator!=(const tracer& rhs) {
                return &map == &rhs.map || pattern != rhs.pattern || state != rhs.state;
            }
        private:
            const Map& map;
            const element_type* pattern;
            index_type state = 0;
        };

        Map() { init(DEFAULT_RESERVE_SIZE); }
        Map(size_t size) { init(size); }
        ~Map() {}

        tracer begin(const element_type* pattern) {
            return tracer(*this, pattern + 1, const_at(init_state, *pattern));
        }

        State end() const {
            return State::terminal;
        }

        template<template<typename T, typename All = std::allocator<T>> typename _container, typename _item>
        _container<index_type>& insert(const _container<_item>& container) {
            _container<index_type> results;
            for (const auto& pattern : container) {
                results.emplace(-(_insert(pattern) + 1));
            }
            return results;
        }

        index_type insert(const pattern_type& pattern) {
            return -(_insert(pattern) + 1);
        }

        template<template<typename T, typename All = std::allocator<T>> typename _container, typename _item>
        void erase(const _container<_item>& container) {
            for (const auto& pattern : container) {
                erase(pattern);
            }
        }

        void erase(const pattern_type& pattern) {
            index_type state = init_state;
            size_t length = pattern.size();
            std::vector<index_type> states(length);

            for (size_t i = 0; i < length; ++i) {
                states[i] = state;
                state = at(state, pattern[i]);
            }

            for (size_t i = length - 1; i + 1; --i)
                if ((i < length - 1 && const_at(states[i], pattern[i]) == State::final) ||
                    !pop(states[i], pattern[i]))
                    break;

            // TODO: check which is faster, front -base or back -pop
            // THINK: how about insert same as erased pattern
        }

        void resize(size_t size) {
            _resize(fstates, size);
            _resize(nstates, size);
        }

        void reserve(size_t size) {
            _reserve(fstates, size);
            _reserve(nstates, size);
        }

        const index_type& const_at(const index_type& index, const element_type& element) const {
            return const_at(index)[element - (element >= CHAR_START ? CHAR_START : 0)];
        }

        const index_type& const_at(const multi_index& index) const {
            return const_at(index.first)[index.second - (index.second >= CHAR_START ? CHAR_START : 0)];
        }

        const node_type& const_at(const index_type& index) const {
            if (index < 0) {
                return fstates[-(index + 1)];
            } else if (index > 0) {
                return nstates[index - 1];
            } else {
                return istates;
            }
        }

        index_type& at(const index_type& index, const element_type& element) {
            return at(index)[element - (element >= CHAR_START ? CHAR_START : 0)];
        }

        index_type& at(const multi_index& index) {
            return at(index.first)[index.second - (index.second >= CHAR_START ? CHAR_START : 0)];
        }

        node_type& at(const index_type& index) {
            if (index < 0) {
                return fstates[-(index + 1)];
            } else if (index > 0) {
                if (index >= nstates.size()) {
                    std::cout << "wtf" << '\n';
                    std::cout << index << ", " << nstates.size() << '\n';
                }
                return nstates[index - 1];
            } else {
                return istates;
            }
        }

        index_type& operator[](const multi_index& index) {
            return at(index.first)[index.second - (index.second >= CHAR_START ? CHAR_START : 0)];
        }

        node_type& operator[](const index_type& index) {
            if (index < 0) {
                return fstates[-(index + 1)];
            } else if (index > 0) {
                return nstates[index - 1];
            } else {
                return istates;
            }
        }

        index_unsinged_type size(State state) const {
            switch (state) {
            case (State::normal):
                return node_size;
            case (State::final):
                return final_size;
            default:
                return 0;
            }
        }

    private:
        block_type fstates;
        block_type nstates;
        node_type istates;
        index_unsinged_type node_size;
        index_unsinged_type final_size;
        std::queue<index_type> node_empty;
        std::queue<index_type> final_empty;

        void init(size_t size) {
            istates = std::array<index_type, CHAR_SIZE>();

            node_size = final_size = 0;

            reserve(size);
            resize(size);
        }

        index_type next(State state) {
            if (final_size >= fstates.size())
                _resize(fstates, final_size * 2);

            if (node_size >= nstates.size()) {
                _resize(nstates, node_size * 2);
            }

            switch (state) {
            case (State::normal): {
                if (!node_empty.empty()) {
                    index_type index = node_empty.front();
                    node_empty.pop();
                    return index;
                }
                return ++node_size;
            }
            case (State::final): {
                if (!final_empty.empty()) {
                    index_type index = final_empty.front();
                    final_empty.pop();
                    return index;
                }
                return -1 * (++final_size);
            }
            default:
                return 0;
            }
        }

        index_type pop(index_type index, element_type element) {
            index_type index_next = at(index, element);
            if (index_next == State::final) {
                final_empty.push(index_next);
                if (!_is_empty(fstates[-(index_next + 1)])) {
                    index_type new_next = next(State::normal);
                    std::swap(nstates[new_next - 1], fstates[-(index_next + 1)]);
                    at(index, element) = new_next;
                    return false;
                }
                at(index, element) = init_state;
                return true;
            } else if (index_next == State::normal) {
                if (!_is_empty(nstates[index_next - 1]))
                    return false;
                at(index, element) = init_state;
                node_empty.push(index_next);
            } else {
                istates[element] = init_state;
                return false;
            }
        }

        index_type _insert(const pattern_type& pattern) {
            index_type state = init_state;
            index_type pre_state = init_state;
            element_type pre_elem = 0;

            for (const auto& element : pattern) {
                if (const_at(state, element) == init_state) {
                    state = at(pre_state = state, pre_elem = element) = next(State::normal);
                } else {
                    state = at(pre_state = state, pre_elem = element);
                }
            }

            index_type ret = at(pre_state, pre_elem) = next(State::final);
            std::swap(at(at(pre_state, pre_elem)), at(state));

            // TODO: check this is reall need?
            std::fill(at(state).begin(), at(state).end(), init_state);
            return ret;
        }

        bool _is_empty(const node_type& node, element_type flag = init_state) {
            return std::all_of(node.begin(), node.end(), [](element_type element) {
                return element == init_state;
            });
        }

        void _resize(block_type& block, size_t size) {
            block.resize(size, std::array<index_type, CHAR_SIZE>());
        }

        void _reserve(block_type& block, size_t size) {
            block.reserve(size);
        }
    };

    class Operator {
    public:
        Operator() {
            patterns.reserve(DEFAULT_RESERVE_SIZE);
            checker.reserve(DEFAULT_RESERVE_SIZE);

            patterns.resize(DEFAULT_RESERVE_SIZE);
        }

        result_type& match(const pattern_type& pattern) {

            if (checker.size() < map.size(State::final)) {
                checker.resize(map.size(State::final), false);
            }

            for (index_unsinged_type i = 0; i < pattern.length(); ++i) {
                for (auto it = map.begin(pattern.c_str() + i); it != State::out; it++) {
                    if (it == State::final) {
                        if (!checker[-(*it) - 1]) {
                            results.push(-(*it) - 1);
                            checker[-(*it) - 1] = true;
                        }
                    }
                }
            }

            std::fill(checker.begin(), checker.end(), false);
            return results;
        }

        bool wrapper(result_type& request, std::function<void(const std::string)> task) {
            if (request.empty())
                return false;

            while (!request.empty()) {
                task(patterns[request.front()]);
                request.pop();
            }
            return true;
        }

        void insert(const pattern_type& pattern) {
            if (uniques.find(pattern) == uniques.end()) {
                index_unsinged_type state = map.insert(pattern);
                while (state > patterns.size())
                    patterns.resize(pattern.size() * 2);
                patterns[state] = std::string(pattern);
                uniques.emplace(patterns[state]);
            }
        }

        void erase(const pattern_type& pattern) {
            if (uniques.find(pattern) != uniques.end()) {
                map.erase(pattern);
                uniques.erase(pattern);
            }
        }

    private:
        Map map;
        result_type results;
        std::vector<bool> checker;
        std::vector<std::string> patterns;
        std::set<std::string> uniques;
    };
}

#endif
