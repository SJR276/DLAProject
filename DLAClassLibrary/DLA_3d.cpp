#include "Stdafx.h"
#include "DLA_3d.h"

DLA_3d::DLA_3d(const double& _coeff_stick) : DLAContainer(_coeff_stick),
	aggregate_pq(utl::distance_comparator(attractor_type::POINT, 1U)) {}

DLA_3d::DLA_3d(lattice_type ltt, attractor_type att, std::size_t att_size, const double& _coeff_stick) : DLAContainer(ltt, att, att_size, _coeff_stick),
	aggregate_pq(utl::distance_comparator(att, att_size)) { initialise_attractor_structure(); }

DLA_3d::DLA_3d(const DLA_3d& other) : DLAContainer(other),
	aggregate_map(other.aggregate_map), aggregate_pq(other.aggregate_pq), attractor_set(other.attractor_set),
	buffer(other.buffer) {}

DLA_3d::DLA_3d(DLA_3d&& other) noexcept : DLAContainer(std::move(other)),
	aggregate_map(std::move(other.aggregate_map)), aggregate_pq(std::move(other.aggregate_pq)),
	attractor_set(std::move(other.attractor_set)), buffer(std::move(other.buffer)) {}

std::size_t DLA_3d::size() const noexcept {
	return aggregate_map.size();
}

const DLA_3d::aggregate3d_buffer_vector& DLA_3d::aggregate_buffer() const noexcept {
	return buffer;
}

void DLA_3d::set_attractor_type(attractor_type attr, std::size_t att_size) {
	DLAContainer::set_attractor_type(attr, att_size);
	aggregate_pq.comparator().att = attr;	// get handle to comparator of pq and alter its attractor_type field
	aggregate_pq.comparator().att_size = attractor_size;
	if (!aggregate_pq.empty()) aggregate_pq.reheapify();	// perform reordering of pq based on new attractor_type
}

void DLA_3d::initialise_attractor_structure() {
	attractor_set.clear();	// clear any current attractor
	attractor_set.reserve(attractor_size);	// pre-allocate memory buckets
	switch (attractor) {
	case attractor_type::POINT: // insert single point at origin to attractor_set
		attractor_set.insert(std::make_tuple(0, 0, 0));
		break;
	case attractor_type::LINE: // insert line extending from [-att_size/2, +att_size/2] to attractor_set
		for (int i = -static_cast<int>(attractor_size) / 2; i < static_cast<int>(attractor_size) / 2; ++i)
			attractor_set.insert(std::make_tuple(i, 0, 0));
		break;
	case attractor_type::PLANE: // insert plane extending from [-att_size/2, +att_size/2] in both x,y to attractor_set
		for (int i = -static_cast<int>(attractor_size) / 2; i < static_cast<int>(attractor_size) / 2; ++i) {
			for (int j = -static_cast<int>(attractor_size) / 2; j < static_cast<int>(attractor_size) / 2; ++j)
				attractor_set.insert(std::make_tuple(i, j, 0));
		}
		break;
	case attractor_type::CIRCLE:	// insert circle of radius att_size to attractor_set
		for (double theta = 0.0; theta <= 2.0*M_PI; theta += M_PI / 180.0) {
			attractor_set.insert(std::make_tuple(
				static_cast<int>(attractor_size*std::cos(theta)),
				static_cast<int>(attractor_size*std::sin(theta)),
				0
			));
		}
		break;
	}
}

void DLA_3d::clear() {
	DLAContainer::clear();
	aggregate_map.clear();
	aggregate_pq.clear();
	aggregate_pq.shrink_to_fit();
	buffer.clear();
	buffer.shrink_to_fit();
}

void DLA_3d::generate(std::size_t n) {
	// compute attractor geometry inserting points to attractor_set
	initialise_attractor_structure();
	aggregate_map.reserve(n);	// pre-allocate n memory slots in agg map
	aggregate_pq.reserve(n); // pre-allocate n capacity to underlying container of priority_queue
	buffer.reserve(n);	// pre-allocate storage for buffer vector to avoid expensive reallocations
	std::size_t count = 0U;
	// initialise current and previous co-ordinate containers
	std::tuple<int, int, int> current = std::make_tuple(0,0,0);
	std::tuple<int, int, int> prev = current;
	bool has_next_spawned = false;
	// variable to store current allowed size of bounding
	// box spawning zone
	int spawn_diameter = 0;
	// aggregate generation loop
	while (size() < n || continuous) {
		if (abort_signal) {
			abort_signal = false;
			return;
		}
		// spawn the next particle if previous particle
		// successfully stuck to aggregate structure
		if (!has_next_spawned) {
			spawn_particle(current, spawn_diameter);
			has_next_spawned = true;
		}
		prev = current;
		// update position of particle via unbiased random walk
		update_particle_position(current, pr_gen());
		// check for collision with bounding walls and reflect if true
		lattice_boundary_collision(current, prev, spawn_diameter);
		// check for collision with aggregate structure and add particle to 
		// the aggregate (both to map and pq) if true, set flag ready for
		// next particle spawn
		if (aggregate_collision(current, prev, pr_gen(), count)) has_next_spawned = false;
	}
}

double DLA_3d::estimate_fractal_dimension() const {
	if (aggregate_pq.empty()) return 0.0;
	// find radius which minimally bounds the aggregate
	double bounding_radius = std::abs(utl::tuple_distance_t<
		decltype(aggregate_pq.top()), 
		3>::tuple_distance(aggregate_pq.top(), attractor, attractor_size));
	if (attractor == attractor_type::POINT || attractor == attractor_type::LINE
		|| attractor == attractor_type::CIRCLE) bounding_radius = std::sqrt(bounding_radius);
	// compute fractal dimension via ln(N)/ln(rmin)
	return std::log(aggregate_map.size()) / std::log(bounding_radius);
}

std::ostream& DLA_3d::write(std::ostream& os, bool sort_by_gen_order) const {
	using utl::operator<<;
	// sort by order particles were added to the aggregate
	if (sort_by_gen_order) {
		// std::vector container to store aggregate map values
		std::vector<std::pair<std::size_t, std::tuple<int, int, int>>> agg_vec;
		agg_vec.reserve(size());
		for (const auto& el : aggregate_map) agg_vec.push_back(std::make_pair(el.second, el.first));
		// sort agg_vec using a lambda based on order of particle generation
		std::sort(agg_vec.begin(), agg_vec.end(), [](auto& _lhs, auto& _rhs) {return _lhs.first < _rhs.first; });
		// write sorted data to stream
		for (const auto& el : agg_vec) os << el.second << '\n';
	}
	// output aggregate data "as-is" without sorting
	else {
		for (const auto& el : aggregate_map) os << el.second << '\t' << el.first << '\n';
	}
	return os;
}

void DLA_3d::spawn_particle(std::tuple<int,int,int>& current, int& spawn_diam) noexcept {
	const int boundary_offset = 8;
	// generate random double in [0,1]
	double placement_pr = pr_gen();
	// set diameter of spawn zone to double the maximum of the largest distance co-ordinate
	// triple currently in the aggregate structure plus an offset to avoid direct sticking spawns
	switch (attractor) {
	case attractor_type::POINT:
		spawn_diam = (aggregate_pq.empty() ? 0 : 2 * static_cast<int>(std::sqrt(utl::tuple_distance_t<
			decltype(aggregate_pq.top()), 3>::tuple_distance(aggregate_pq.top(), attractor, attractor_size)))) + boundary_offset;
		if (is_spawn_source_above && is_spawn_source_below) {
			if (placement_pr < 1.0 / 3.0) {	// positive/negative z-plane of boundary
				std::get<0>(current) = static_cast<int>(spawn_diam*(pr_gen() - 0.5));
				std::get<1>(current) = static_cast<int>(spawn_diam*(pr_gen() - 0.5));
				std::get<2>(current) = (placement_pr < 1.0 / 6.0) ? spawn_diam / 2 : -spawn_diam / 2;
			}
			else if (placement_pr >= 1.0 / 3.0 && placement_pr < 2.0 / 3.0) { // positive/negative x-plane of boundary
				std::get<0>(current) = (placement_pr < 0.5) ? spawn_diam / 2 : -spawn_diam / 2;
				std::get<1>(current) = static_cast<int>(spawn_diam*(pr_gen() - 0.5));
				std::get<2>(current) = static_cast<int>(spawn_diam*(pr_gen() - 0.5));
			}
			else {	// positive/negative y-plane of boundary
				std::get<0>(current) = static_cast<int>(spawn_diam*(pr_gen() - 0.5));
				std::get<1>(current) = (placement_pr < 5.0 / 6.0) ? spawn_diam / 2 : -spawn_diam / 2;
				std::get<2>(current) = static_cast<int>(spawn_diam*(pr_gen() - 0.5));
			}
		}
		else {
			if (placement_pr < 1.0 / 3.0) { // positive/negative z-plane
				std::get<0>(current) = static_cast<int>(spawn_diam*(pr_gen() - 0.5));
				std::get<1>(current) = static_cast<int>(spawn_diam*(pr_gen() - 0.5));
				std::get<2>(current) = (is_spawn_source_above) ? spawn_diam / 2 : -spawn_diam / 2;
			}
			else if (placement_pr >= 1.0 / 3.0 && placement_pr < 2.0 / 3.0) { // positive/negative x-plane of boundary
				std::get<0>(current) = (placement_pr < 0.5) ? spawn_diam / 2 : -spawn_diam / 2;
				std::get<1>(current) = static_cast<int>(spawn_diam*(pr_gen() - 0.5));
				std::get<2>(current) = ((is_spawn_source_above) ? 1 : -1) * static_cast<int>(spawn_diam*(pr_gen()*0.5));
			}
			else {	// positive/negative y-plane of boundary
				std::get<0>(current) = static_cast<int>(spawn_diam*(pr_gen() - 0.5));
				std::get<1>(current) = (placement_pr < 5.0 / 6.0) ? spawn_diam / 2 : -spawn_diam / 2;
				std::get<2>(current) = ((is_spawn_source_above) ? 1 : -1) * static_cast<int>(spawn_diam*(pr_gen()*0.5));
			}
		}
		break;
	case attractor_type::LINE:
		spawn_diam = (aggregate_pq.empty() ? 0 : 2*static_cast<int>(std::sqrt(utl::tuple_distance_t<
			decltype(aggregate_pq.top()), 3>::tuple_distance(aggregate_pq.top(), attractor, attractor_size)))) + boundary_offset;
		std::get<0>(current) = static_cast<int>(attractor_size*(pr_gen() - 0.5));
		if (is_spawn_source_above && is_spawn_source_below) {
			if (placement_pr < 0.5) {	// positive/negative z-plane of boundary
				std::get<1>(current) = (pr_gen() < 0.5) ? spawn_diam / 2 : -spawn_diam / 2;
				std::get<2>(current) = (placement_pr < 0.25) ? spawn_diam / 2 : -spawn_diam / 2;
			}
			else {	// positive/negative y-plane of boundary			
				std::get<1>(current) = (placement_pr < 0.75) ? spawn_diam / 2 : -spawn_diam / 2;
				std::get<2>(current) = (pr_gen() < 0.5) ? spawn_diam / 2 : -spawn_diam / 2;
			}
		}
		else {
			if (placement_pr < 0.5) { // positive/negative z-plane of boundary
				std::get<1>(current) = (pr_gen() < 0.5) ? spawn_diam / 2 : -spawn_diam / 2;
				std::get<2>(current) = (is_spawn_source_above) ? spawn_diam / 2 : -spawn_diam / 2;
			}
			else { // postive/negative y-plane of boundary
				std::get<1>(current) = (placement_pr < 0.75) ? spawn_diam / 2 : -spawn_diam / 2;
				std::get<2>(current) = (is_spawn_source_above) ? spawn_diam / 2 : -spawn_diam / 2;
			}
		}
		break;
	case attractor_type::PLANE:
		spawn_diam = (aggregate_pq.empty() ? 0 : std::abs(std::get<2>(aggregate_pq.top()))) + boundary_offset;
		std::get<0>(current) = static_cast<int>(attractor_size*(pr_gen() - 0.5));
		std::get<1>(current) = static_cast<int>(attractor_size*(pr_gen() - 0.5));
		if (is_spawn_source_above && is_spawn_source_below)
			std::get<2>(current) = (placement_pr < 0.5) ? spawn_diam : -spawn_diam; // positive : negative z-plane
		else
			std::get<2>(current) = (is_spawn_source_above) ? spawn_diam : -spawn_diam; // positive : negative z-plane
		break;
	case attractor_type::CIRCLE:
		spawn_diam = 2 * static_cast<int>((aggregate_pq.empty() ? attractor_size : std::sqrt(utl::tuple_distance_t<
			decltype(aggregate_pq.top()), 3>::tuple_distance(aggregate_pq.top(), attractor, attractor_size)))) + boundary_offset;
		if (is_spawn_source_above && is_spawn_source_below) {
			if (placement_pr < 0.5) {
				std::get<0>(current) = 0;
				std::get<1>(current) = 0;
				std::get<2>(current) = 0;
			}
			else {
				if (placement_pr < 2.0 / 3.0) {	// positive/negative z-plane of boundary
					std::get<0>(current) = static_cast<int>(spawn_diam*(pr_gen() - 0.5));
					std::get<1>(current) = static_cast<int>(spawn_diam*(pr_gen() - 0.5));
					std::get<2>(current) = (placement_pr < 7.0 / 12.0) ? spawn_diam / 2 : -spawn_diam / 2;
				}
				else if (placement_pr >= 2.0 / 3.0 && placement_pr < 5.0 / 6.0) { // positive/negative x-plane of boundary
					std::get<0>(current) = (placement_pr < 9.0/12.0) ? spawn_diam / 2 : -spawn_diam / 2;
					std::get<1>(current) = static_cast<int>(spawn_diam*(pr_gen() - 0.5));
					std::get<2>(current) = static_cast<int>(spawn_diam*(pr_gen() - 0.5));
				}
				else {	// positive/negative y-plane of boundary
					std::get<0>(current) = static_cast<int>(spawn_diam*(pr_gen() - 0.5));
					std::get<1>(current) = (placement_pr < 11.0 / 12.0) ? spawn_diam / 2 : -spawn_diam / 2;
					std::get<2>(current) = static_cast<int>(spawn_diam*(pr_gen() - 0.5));
				}
			}
		}
		else if (is_spawn_source_above) { // spawn on bounding box boundary
			if (placement_pr < 1.0 / 3.0) {	// positive/negative z-plane of boundary
				std::get<0>(current) = static_cast<int>(spawn_diam*(pr_gen() - 0.5));
				std::get<1>(current) = static_cast<int>(spawn_diam*(pr_gen() - 0.5));
				std::get<2>(current) = (placement_pr < 1.0 / 6.0) ? spawn_diam / 2 : -spawn_diam / 2;
			}
			else if (placement_pr >= 1.0 / 3.0 && placement_pr < 2.0 / 3.0) { // positive/negative x-plane of boundary
				std::get<0>(current) = (placement_pr < 0.5) ? spawn_diam / 2 : -spawn_diam / 2;
				std::get<1>(current) = static_cast<int>(spawn_diam*(pr_gen() - 0.5));
				std::get<2>(current) = static_cast<int>(spawn_diam*(pr_gen() - 0.5));
			}
			else {	// positive/negative y-plane of boundary
				std::get<0>(current) = static_cast<int>(spawn_diam*(pr_gen() - 0.5));
				std::get<1>(current) = (placement_pr < 5.0 / 6.0) ? spawn_diam / 2 : -spawn_diam / 2;
				std::get<2>(current) = static_cast<int>(spawn_diam*(pr_gen() - 0.5));
			}
		}
		break;
	}
}

void DLA_3d::push_particle(const std::tuple<int, int, int>& p, std::size_t count) {
	aggregate_map.insert(std::make_pair(p, count));
	aggregate_pq.push(p);
	buffer.push_back(p);
}

bool DLA_3d::aggregate_collision(const std::tuple<int,int,int>& current, const std::tuple<int,int,int>& previous, const double& sticky_pr, std::size_t& count) {
	// particle did not stick to aggregate, increment aggregate_misses counter
	if (sticky_pr > coeff_stick) ++aggregate_misses_;
	// else, if current co-ordinates of particle exist in aggregate
	// or attractor then collision and successful sticking occurred
	else if (aggregate_map.find(current) != aggregate_map.end() || attractor_set.find(current) != attractor_set.end()) {
		// insert previous position of particle to aggregrate_map and aggregrate priority queue
		push_particle(previous, ++count);
		std::tuple<int,int,int> max_dist = aggregate_pq.top();
		aggregate_span = aggregate_pq.empty() ? 0 : utl::tuple_distance_t<
			decltype(aggregate_pq.top()), 
			3>::tuple_distance(aggregate_pq.top(), attractor, attractor_size);
		return true;
	}
	return false;
}