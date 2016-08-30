#pragma once
#include <ostream>
#include <random>
#include <type_traits>
#include <tuple>
#include <utility>
#include <vector>

namespace utl {
	/**
	 * \struct tuple_distance_compute
	 *
	 * \brief Helper function object for computing squared radius of a tuple
	 *        co-ordinate from the origin.
	 */
	template<class Tuple, std::size_t N>
	struct tuple_distance_compute {
		static auto distance_sqd(const Tuple& t) {
			return tuple_distance_compute<Tuple, N - 1>::distance_sqd(t)
				+ std::get<N - 1>(t)*std::get<N - 1>(t);
		}
	};
	template<class Tuple>
	struct tuple_distance_compute<Tuple, 1> {
		static auto distance_sqd(const Tuple& t) {
			return std::get<0>(t)*std::get<0>(t);
		}
	};
	/**
	 * \struct distance_comparator
	 *
	 * \brief Defines a distance comparator function object for a `std::tuple` (incl. `std::pair`)
	 *        of generic types, used for determining tuple which has greater distance from origin.
	 */
	struct distance_comparator {
		template<class... Types>
		bool operator()(const std::tuple<Types...>& lhs, const std::tuple<Types...>& rhs) const {
			return tuple_distance_compute<decltype(lhs), sizeof...(Types)>::distance_sqd(lhs)
				< tuple_distance_compute<decltype(rhs), sizeof...(Types)>::distance_sqd(rhs);
		}
		template<class Ty1, class Ty2>
		bool operator()(const std::pair<Ty1, Ty2>& lhs, const std::pair<Ty1, Ty2>& rhs) const {
			return tuple_distance_compute<decltype(lhs), 2>::distance_sqd(lhs)
				< tuple_distance_compute<decltype(rhs), 2>::distance_sqd(rhs);
		}
	};
	/**
	 * \struct tuple_hash_compute
	 *
	 * \brief Helper function object for computing hash function of a tuple instance.
	 */
	template<class Tuple, std::size_t N>
	struct tuple_hash_compute {
		static std::size_t hash_compute(const Tuple& t) {
			//return tuple_hash_compute<Tuple, N - 1>::hash_compute(t) + std::hash<std::tuple_element<N-1, decltype(t)>::type>()(std::get<N - 1>(t));
			return tuple_hash_compute<Tuple, N - 1>::hash_compute(t) + std::hash<int>()(std::get<N - 1>(t));
		}
	};
	template<class Tuple>
	struct tuple_hash_compute<Tuple, 1> {
		static std::size_t hash_compute(const Tuple& t) {
			//return 51 + std::hash<std::tuple_element<N - 1, decltype(t)>::type>()(std::get<0>(t)) * 51;
			return 51 + std::hash<int>()(std::get<0>(t)) * 51;
		}
	};
	/**
	 * \struct triple_hash
	 *
	 * \brief Defines a hash function object for a `std::tuple` (incl. `std::pair`)
	 *        of generic types.
	 */
	struct tuple_hash {
		template<class... Types>
		std::size_t operator()(const std::tuple<Types...>& t) const {
			return tuple_hash_compute<decltype(t), sizeof...(Types)>::hash_compute(t);
		}
		template<class Ty1, class Ty2>
		std::size_t operator()(const std::pair<Ty1, Ty2>& p) const {
			return 51 + std::hash<Ty1>()(p.first) * 51 + std::hash<Ty2>()(p.second);
		}
	};


	// utl::triple
	/**
	 * \struct triple
	 *
	 * \brief Defines a triplet of generic types.
	 */
	template<typename _Ty1, typename _Ty2, typename _Ty3> struct triple {
		_Ty1 first;
		_Ty2 second;
		_Ty3 third;
		/**
		 * \brief Default constructor, initialises the container with default-constructed values.
		 */
		template<class _Uty1 = _Ty1,
			class _Uty2 = _Ty2,
			class _Uty3 = _Ty3,
			class = std::enable_if_t<std::is_default_constructible<_Uty1>::value
				&& std::is_default_constructible<_Uty2>::value
				&& std::is_default_constructible<_Uty3>::value> 
		> constexpr triple() : first(), second(), third() {}
		/**
		 * \brief Construct a `triple` object from specified values using copy construction.
		 */
		template<class _Uty1 = _Ty1,
			class _Uty2 = _Ty2,
			class _Uty3 = _Ty3,
			class = std::enable_if_t<std::is_copy_constructible<_Uty1>::value
				&& std::is_copy_constructible<_Uty2>::value
				&& std::is_copy_constructible<_Uty3>::value> 
		> triple(const _Ty1& _first, const _Ty2& _second, const _Ty3& _third) : first(_first), second(_second), third(_third) {}
		/**
		 * \brief Construct a `triple` object from specified values using move construction.
		 */
		template<class _Uty1 = _Ty1,
			class _Uty2 = _Ty2,
			class _Uty3 = _Ty3,
			class = std::enable_if_t<std::is_move_constructible<_Uty1>::value
				&& std::is_move_constructible<_Uty2>::value
				&& std::is_move_constructible<_Uty3>::value> 
		> triple(_Ty1&& _first, _Ty2&& _second, _Ty3&& _third) : first(std::move(_first)), second(std::move(_second)), third(std::move(_third)) {}
	};
	/**
	 * \brief Makes a `triple` object with given values.
	 *
	 * \param _val1 Value with which to initialise first field of `triple`.
	 * \param _val2 Value with which to initialise second field of `triple`.
	 * \param _val3 Value with which to initialise third field of `triple`.
	 * \return `triple` object comprised of specified parameters.
	 */
	template<typename _Ty1, typename _Ty2, typename _Ty3> 
	inline constexpr triple<typename std::_Unrefwrap<_Ty1>::type, typename std::_Unrefwrap<_Ty2>::type, typename std::_Unrefwrap<_Ty3>::type> make_triple(_Ty1&& _val1, _Ty2&& _val2, _Ty3&& _val3) {
		typedef triple<typename std::_Unrefwrap<_Ty1>::type, typename std::_Unrefwrap<_Ty2>::type, typename std::_Unrefwrap<_Ty3>::type> _ret_triple;
		return _ret_triple(std::forward<_Ty1>(_val1), std::forward<_Ty2>(_val2), std::forward<_Ty3>(_val3));
	}
	/**
	 * \brief Equality operator for `triple`.
	 *
	 * \param lhs First instance of `triple`.
	 * \param rhs Second instance of `triple`.
	 * \return `true` if `lhs == rhs`, `false` otherwise.
	 */
	template<typename _Ty1, typename _Ty2, typename _Ty3> 
	bool operator==(const triple<_Ty1, _Ty2, _Ty3>& lhs, const triple<_Ty1, _Ty2, _Ty3>& rhs) {
		return lhs.first == rhs.first && lhs.second == rhs.second && lhs.third == rhs.third;
	}
	/**
	 * \brief Inequality operator for `triple`.
	 *
	 * \param lhs First instance of `triple`.
	 * \param rhs Second instance of `triple`.
	 * \return `true` if `lhs != rhs`, `false` otherwise.
	 */
	template<typename _Ty1, typename _Ty2, typename _Ty3> 
	bool operator!=(const triple<_Ty1, _Ty2, _Ty3>& lhs, const triple<_Ty1, _Ty2, _Ty3>& rhs) {
		return !(lhs == rhs);
	}
	/**
	 * \brief Stream insertion operator for `triple`.
	 *
	 * \param os Instance of stream type to insert to.
	 * \param trp `triple` instance to insert to data stream.
	 * \return Modified `std::ostream` instance `os` containing data of `trp`.
	 */
	template<typename _Ty1, typename _Ty2, typename _Ty3> 
	std::ostream& operator<<(std::ostream& os, const triple<_Ty1, _Ty2, _Ty3>& trp) {
		os << trp.first << "\t" << trp.second << "\t" << trp.third;
		return os;
	}
	// std::pair INSERTION OPERATOR
	/**
	 * \brief Stream insertion operator for `std::pair`.
	 *
	 * \param os Instance of stream type to insert to.
	 * \param p `std::pair` instance to insert to data stream.
	 * \return Modified `std::ostream` instance `os` containing data of `p`.
	 */
	template<typename _Ty1, typename _Ty2> 
	std::ostream& operator<<(std::ostream& os, const std::pair<_Ty1, _Ty2>& p) {
		os << p.first << "\t" << p.second;
		return os;
	}
	/*************** std::vector INSERTION OPERATOR ***************/
	/**
	 * \brief Stream insertion operator for `std::vector`.
	 *
	 * \param os Instance of stream type to insert to.
	 * \param vec `std::vector` instance to insert to data stream.
	 * \return Modified `std::ostream` instance `os` containing data of `vec`.
	 */
	template<typename _Ty1, typename _Ty2> 
	std::ostream& operator<<(std::ostream& os, const std::vector<std::pair<_Ty1, _Ty2>>& vec) {
		for (const auto& it : vec)
			os << it << '\n';
		return os;
	}
#ifndef RANDOM_NUMBER_GENERATOR_H
#define RANDOM_NUMBER_GENERATOR_H
	/**
	 * \class random_number_generator
	 *
	 * \brief Pseudo-random number generator for random values over a specified `Distribution`
	 *        using a given `Generator` engine.
	 *
	 * A convenience wrapper around a generator engine and random number distribution used for
	 * generating random values quickly and simply. Any pre-defined generator from the C++ `<random>`
	 * header may be used as the `Generator` type-param and any distribution from this header
	 * may be used for the `Distribution` type-param. The next value in the random distribution
	 * is generated via a call to `random_number_generator::operator()`. Resetting the internal
	 * state of the distribution such that the next generating call is not dependent upon the last
	 * call is achieved via a call to `random_number_generator::reset_distribution_state()`.
	 *
	 * Note that the type `Ty` (referenced as `result_type` in the class API) must match the type of
	 * distribution `Distribution` (referenced as `distribution_type` in the class API) used, e.g.
  	 * if the `result_type` is `int` then it is undefined behaviour to use a distribution type
	 * intended for floating point types.
	 * 
	 * \tparam Ty The type of the values to generate, must satisfy `std::is_arithmetic<Ty>`. Defaults
	 *         to the integral type `int`.
	 * \tparam Generator The type of the generator engine to use for pseudo-random generation, must
	 *         meet the requirement of `UniformRandomBitGenerator` (see C++ Concepts). Defaults to
	 *         the engine type `std::mt19937`.
	 * \tparam Distribution The type of distribution over which to calculate the random numbers. The
	 *         value type of the distribution must match the value type `Ty` of this class. Must meet
	 *         the requirement of `RandomNumberDistribution` (see C++ Conecpts). Defaults to the
 	 *         distribution type `std::uniform_int_distribution<Ty>`.
	 */
	template<class Ty = int,
		class Generator = std::mt19937,
		class Distribution = std::uniform_int_distribution<Ty>,
		class = std::enable_if_t<std::is_arithmetic<Ty>::value>
	> class random_number_generator {
	public:
		// PUBLIC API TYPE DEFINITIONS
		typedef Ty result_type;
		typedef Generator generator_type;
		typedef Distribution distribution_type;
		// CONSTRUCTION/ASSIGNMENT
		/**
		 * \brief Move constructs the generator with the values of the engine `_eng` and the
		 *        distribution `_dist`. This is also the default constructor.
		 *
		 * \param _eng Generator engine to use.
		 * \param _dist Distribution for random numbers.
		 */
		explicit random_number_generator(Generator&& _eng = Generator{ std::random_device{}() }, Distribution&& _dist = Distribution())
			: eng(std::move(_eng)), dist(std::move(_dist)) {
		}
		/**
		 * \brief Constructs the generator with a copy of the values of the engine `_eng` and
		 *        the distribution `_dist`.
		 *
		 * \param _eng Generator engine to use.
		 * \param _dist Distribution for random numbers.
		 */
		explicit random_number_generator(const Generator& _eng, const Distribution& _dist)
			: eng(_eng), dist(_dist) {
		}
		/**
		 * \brief Copy constructor. Constructs the generator with a copy of the fields of `other`.
		 *
		 * \param other `random_number_generator` instance to use as data source.
		 */
		random_number_generator(const random_number_generator& other)
			: eng(other.eng), dist(other.dist) {
		}
		/**
		 * \brief Move constructor. Constructs the generator with the fields of `other` using
		 *        move-semantics such that `other` is left in a valid but unspecified state.
		 *
		 * \param other `random_number_generator` instance to use as data source.
		 */
		random_number_generator(random_number_generator&& other)
			: eng(std::move(other.eng)), dist(std::move(other.dist)) {
		}
		/**
		 * \brief Copy-assignment operator. Replaces the generator with a copy of the fields of `other`.
		 * \param other `random_number_generator` instance to use as data source.
		 * \return `*this`.
		 */
		random_number_generator& operator=(const random_number_generator& other) {
			if (this != &other)
				random_number_generator(other).swap(*this); // copy-and-swap
			return *this;
		}
		/**
		 * \brief Move-assignment operator. Replaces the generator with the field of `other`
		 *        using move-semantics.
		 * \param other `random_number_generator` instance to use as data source.
		 * \return `*this`.
		 */
		random_number_generator& operator=(random_number_generator&& other) {
			if (this != &other)
				swap(*this, std::move(other));
			return *this;
		}
		// GENERATING OPERATOR()
		/**
		 * \brief Generates the next random number in the distribution.
		 *
		 * \return The generated random number.
		 */
		result_type operator()() { return dist(eng); }
		// GENERATOR AND DISTRIBUTION OBJECT ACCESS
		/**
		 * \brief Returns a copy of the underlying generator engine.
		 * \return A copy of the underlying engine used for random number generation.
		 */
		generator_type get_generator() const noexcept { return eng; }
		/**
		 * \brief Returns a copy of the underlying distribution.
		 * \return A copy of the underlying distribution over which the random
		 *         numbers are generated.
		 */
		distribution_type get_distribution() const noexcept { return dist; }
		// PROPERTIES
		/**
		 * \brief Returns the minimum potentially generated value.
		 * \return The minimum value potentially generated by the underlying distribution.
		 */
		result_type min() const { return dist.min(); }
		/**
		 * \brief Returns the maximum potentially generated value.
		 * \return The maximum value potentially generated by the underlying distribution.
	     */
		result_type max() const { return dist.max(); }
		// MODIFIERS
		/**
		 * \brief Resets the internal state of the underlying distribution object. After calling this function,
		 *        the next call to `operator()` on the generator will not be dependent upon previous calls
		 *        to `operator()`.
		 */
		void reset_distribution_state() { dist.reset(); }
		/**
		 * \brief Exchanges the fields of the generator with those of `other`.
		 *
		 * \param other `random_number_generator` object to swap with.
		 */
		void swap(random_number_generator& other) {
			std::swap(eng, other.eng);
			std::swap(dist, other.dist);
		}
		/**
		 * \brief Exchanges the fields of the generator `lhs` with those of `rhs`.
		 *
		 * \param lhs `random_number_generator` object to swap with `rhs`.
		 * \param rhs `random_number_generator` object to swap with `lhs`.
		 */
		static void swap(random_number_generator& lhs, random_number_generator& rhs) { lhs.swap(rhs); }
	private:
		generator_type eng;
		distribution_type dist;
	};
#endif // !RANDOM_NUMBER_GENERATOR_H
#ifndef UNIFORM_RANDOM_PROBABILITY_GENERATOR_H
#define UNIFORM_RANDOM_PROBABILITY_GENERATOR_H
	/**
	 * \class uniform_random_probability_generator
	 *
	 * \brief Pseudo-random number generator for random floating point values distributed
	 *        uniformly over the range [0.0, 1.0] using a given `Generator` engine.
	 *
	 * A convenience wrapper around a template specialisation of `crsc::random_number_generator` providing
	 * a class to produce uniformly distributed floating point values in the range [0.0, 1.0]. Any pre-defined
	 * generator from the C++ `<random>` header may be used as the `Generator` type-param. The next value in
	 * the random distribution is generated via a call to `uniform_random_probability_generator::operator()`.
	 * Resetting the internal state of the distribution such that the next generating call is not dependent upon
	 * the last call is achieved via a call to `uniform_random_probability_generator::reset_distribution_state()`.
	 *
	 * \tparam FloatType The type of the probabilities to generator, must satisfy `std::is_floating_point<FloatType>`.
	 *         Defaults to the type `double`.
	 * \tparam Generator The type of the generator engine to use for pseudo-random generation, must
	 *         meet the requirement of `UniformRandomBitGenerator` (see C++ Concepts). Defaults to
	 *         the engine type `std::mt19937`.
	 */
	template<class FloatType = double,
		class Generator = std::mt19937,
		class = std::enable_if_t<std::is_floating_point<FloatType>::value>
	> class uniform_random_probability_generator {
		typedef random_number_generator<FloatType, Generator, std::uniform_real_distribution<FloatType>> uniform_pr_gen;
	public:
		// PUBLIC API TYPE DEFINITIONS
		typedef typename uniform_pr_gen::result_type result_type;
		typedef typename uniform_pr_gen::generator_type generator_type;
		typedef typename uniform_pr_gen::distribution_type distribution_type;
		// CONSTRUCTION/ASSIGNMENT
		explicit uniform_random_probability_generator(Generator&& engine = Generator{ std::random_device{}() })
			: generator(std::move(engine)) {
		}
		explicit uniform_random_probability_generator(const Generator& engine)
			: generator(engine, distribution_type()) {
		}
		uniform_random_probability_generator(const uniform_random_probability_generator& other)
			: generator(other.generator) {
		}
		uniform_random_probability_generator(uniform_random_probability_generator&& other)
			: generator(std::move(other.generator)) {
		}
		uniform_random_probability_generator& operator=(const uniform_random_probability_generator& other) {
			if (this != &other)
				generator = other.generator;
			return *this;
		}
		uniform_random_probability_generator& operator=(uniform_random_probability_generator&& other) {
			if (this != &other)
				generator = std::move(other.generator);
			return *this;
		}
		// GENERATING OPERATOR()
		result_type operator()() { return generator(); }
		// GENERATOR AND DISTRIBUTION OBJECT ACCESS
		generator_type get_generator() const noexcept { return generator.get_generator(); }
		distribution_type get_distribution() const noexcept { return generator.get_distribution(); }
		// PROPERTIES
		constexpr result_type min() const { return generator.min(); }
		constexpr result_type max() const { return generator.max(); }
		// MODIFIERS
		void reset_distribution_state() { generator.reset_distribution_state(); }
		void swap(uniform_random_probability_generator& other) { generator.swap(other.generator); }
		static void swap(uniform_random_probability_generator& lhs, uniform_random_probability_generator& rhs) { lhs.swap(rhs); }
	private:
		uniform_pr_gen generator;
	};
#endif // !UNIFORM_RANDOM_PROBABILITY_GENERATOR_H
}