#include "Stdafx.h"
#include "DLA_2d.h"

DLA_2d::DLA_2d(const double& _coeff_stick) : DLAContainer(_coeff_stick), 
	aggregate_pq(utl::distance_comparator(attractor_type::POINT, 1U)) {}

DLA_2d::DLA_2d(lattice_type ltt, attractor_type att, std::size_t att_size, const double& _coeff_stick) : DLAContainer(ltt, att, att_size, _coeff_stick),
	aggregate_pq(utl::distance_comparator(att, att_size)) {	initialise_attractor_structure(); }

DLA_2d::DLA_2d(const DLA_2d& other) noexcept : DLAContainer(other),
	aggregate_map(other.aggregate_map), aggregate_pq(other.aggregate_pq), attractor_set(other.attractor_set),
		buffer(other.buffer) {}

DLA_2d::DLA_2d(DLA_2d&& other) noexcept : DLAContainer(std::move(other)),
	aggregate_map(std::move(other.aggregate_map)), aggregate_pq(std::move(other.aggregate_pq)),
	attractor_set(std::move(other.attractor_set)), buffer(std::move(other.buffer)) {}

std::size_t DLA_2d::size() const noexcept {
	return aggregate_map.size();
}

const DLA_2d::aggregate2d_buffer_vector& DLA_2d::aggregate_buffer() const noexcept {
	return buffer;
}

void DLA_2d::set_attractor_type(attractor_type attr, std::size_t att_size) {
	// invalid attractor type for 2D lattice
	if (attr == attractor_type::PLANE)
		throw std::invalid_argument("Cannot set attractor type of 2D DLA to PLANE.");
	DLAContainer::set_attractor_type(attr, att_size);
	aggregate_pq.comparator().att = attr;	// get handle to comparator of pq and alter its attractor_type field
	aggregate_pq.comparator().att_size = attractor_size;
	if (!aggregate_pq.empty()) aggregate_pq.reheapify(); // perform reordering of pq based on new attractor_type
}

void DLA_2d::initialise_attractor_structure() {
	attractor_set.clear();	// clear any current attractor
	attractor_set.reserve(attractor_size);	// pre-allocate memory buckets
	switch (attractor) {
	case attractor_type::POINT:	// insert single point at origin to attractor_set
		attractor_set.insert(std::make_pair(0, 0));
		break;
	case attractor_type::LINE:	// insert line extending from [-att_size/2, +att_size/2] to attractor_set
		for (int i = -static_cast<int>(attractor_size) / 2; i < static_cast<int>(attractor_size) / 2; ++i)
			attractor_set.insert(std::make_pair(i, 0));
		break;
	case attractor_type::CIRCLE: // insert circle of radius att_size to attractor_set
		for (double theta = 0.0; theta <= 2.0*M_PI; theta += M_PI / 180.0) {
			attractor_set.insert(std::make_pair(
				static_cast<int>(attractor_size*std::cos(theta)),
				static_cast<int>(attractor_size*std::sin(theta))
			));
		}
		break;
	}
}

void DLA_2d::clear() {
	DLAContainer::clear();
	aggregate_map.clear();
	aggregate_pq.clear();
	aggregate_pq.shrink_to_fit();
	buffer.clear();
	buffer.shrink_to_fit();
}

void DLA_2d::generate(std::size_t n) {
	// compute attractor geometry inserting points to attractor_set
	initialise_attractor_structure();
	aggregate_map.reserve(n);	// pre-allocate n memory slots in agg map
	aggregate_pq.reserve(n); // pre-allocate n capacity to underlying container of priority_queue
	buffer.reserve(n);	// pre-allocate storage for buffer vector to avoid expensive reallocations
	std::size_t count = 0U;
	// initialise current and previous co-ordinate containers
	std::pair<int, int> current = std::make_pair(0, 0);
	std::pair<int, int> prev = current;
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

double DLA_2d::estimate_fractal_dimension() const {
	if (aggregate_pq.empty()) return 0.0;
	// find radius which minimally bounds the aggregate
	double bounding_radius = std::abs(utl::tuple_distance_t<
		decltype(aggregate_pq.top()), 
		2>::tuple_distance(aggregate_pq.top(), attractor, attractor_size));
	if (attractor == attractor_type::CIRCLE) bounding_radius = std::sqrt(bounding_radius - attractor_size);
	if (attractor == attractor_type::POINT) bounding_radius = std::sqrt(bounding_radius);
	// compute fractal dimension via ln(N)/ln(rmin)
	return std::log(aggregate_map.size()) / std::log(bounding_radius);
}

std::ostream& DLA_2d::write(std::ostream& os, bool sort_by_gen_order) const {
	using utl::operator<<;
	// sort by order particles were added to the aggregate
	if (sort_by_gen_order) {
		// std::vector container to store aggregate_map values
		std::vector<std::pair<std::size_t, std::pair<int, int>>> agg_vec;
		agg_vec.reserve(size());	// pre-reserve space for performance
		for (const auto& el : aggregate_map) agg_vec.push_back(std::make_pair(el.second, el.first));
		// sort agg_vec using a lambda based on order of particle generation
		std::sort(agg_vec.begin(), agg_vec.end(), [](auto& lhs, auto& rhs) {return lhs.first < rhs.first; });
        // write sorted data to stream
		for (const auto& el : agg_vec) os << el.second << '\n';
	}
	// output aggregate data "as-is" without sorting
	else {
		for (const auto& el : aggregate_map) os << el.second << '\t' << el.first << '\n';
	}
	return os;
}

void DLA_2d::spawn_particle(std::pair<int,int>& spawn_pos, int& spawn_diam) noexcept {
	const int boundary_offset = 8;
	// generate random double in [0,1]
	double placement_pr = pr_gen();
	// set diameter of spawn zone to double the maximum of the largest distance co-ordinate
	// pair currently in the aggregate structure plus an offset to avoid direct sticking spawns
	switch (attractor) {
	case attractor_type::POINT:
		spawn_diam = (aggregate_pq.empty() ? 0 : 2 * static_cast<int>(std::hypot(aggregate_pq.top().first, aggregate_pq.top().second))) 
			+ boundary_offset;
		if (is_spawn_source_above && is_spawn_source_below) {
			if (placement_pr < 0.5) {	// spawn on upper or lower line of lattice boundary
				spawn_pos.first = static_cast<int>(spawn_diam*(pr_gen() - 0.5));
				spawn_pos.second = (placement_pr < 0.25) ? spawn_diam / 2 : -spawn_diam / 2;
			}
			else {	// spawn on left/right line of lattice boundary
				spawn_pos.first = (placement_pr < 0.75) ? spawn_diam / 2 : -spawn_diam / 2;
				spawn_pos.second = static_cast<int>(spawn_diam*(pr_gen() - 0.5));
			}
		}
		else {
			if (placement_pr < 0.5) {	// spawn on upper : lower line of lattice boundary
				spawn_pos.first = static_cast<int>(spawn_diam*(pr_gen() - 0.5));
				spawn_pos.second = (is_spawn_source_above) ? spawn_diam / 2 : -spawn_diam / 2;
			}
			else {	// spawn on left/right line of lattice boundary in upper : lower region
				spawn_pos.first = (placement_pr < 0.75) ? spawn_diam / 2 : -spawn_diam / 2;
				spawn_pos.second = ((is_spawn_source_above) ? 1 : -1) * static_cast<int>(spawn_diam*(pr_gen()*0.5));
			}
		}
		break;
	case attractor_type::LINE:
		spawn_diam = (aggregate_pq.empty() ? 0 : std::abs(aggregate_pq.top().second)) + boundary_offset;
		spawn_pos.first = static_cast<int>(attractor_size*(pr_gen() - 0.5));
		if (is_spawn_source_above && is_spawn_source_below) 			
			spawn_pos.second = (placement_pr < 0.5) ? spawn_diam : -spawn_diam; // upper : lower 		
		else 
			spawn_pos.second = (is_spawn_source_above) ? spawn_diam : -spawn_diam; // upper : lower
		break;
	case attractor_type::CIRCLE:
		spawn_diam = 2*static_cast<int>((aggregate_pq.empty() ? attractor_size : std::sqrt(utl::tuple_distance_t<
			decltype(aggregate_pq.top()), 2>::tuple_distance(aggregate_pq.top(), attractor, attractor_size)))) + boundary_offset;
		if (is_spawn_source_above && is_spawn_source_below) {
			if (placement_pr < 0.5) { // spawn at origin
				spawn_pos.first = 0;
				spawn_pos.second = 0;
			}
			else {	// spawn on bounding box boundary
				if (placement_pr < 0.75) { // upper/lower
					spawn_pos.first = static_cast<int>(spawn_diam*(pr_gen() - 0.5));
					spawn_pos.second = (placement_pr < 0.625) ? spawn_diam / 2 : -spawn_diam / 2;
				}
				else { // left/right
					spawn_pos.first = (placement_pr < 0.875) ? spawn_diam / 2 : -spawn_diam / 2;
					spawn_pos.second = static_cast<int>(spawn_diam*(pr_gen() - 0.5));
				}
			}
		}
		else if (is_spawn_source_above) { // spawn on bounding box boundary
			if (placement_pr < 0.5) {	// spawn on upper or lower line of lattice boundary
				spawn_pos.first = static_cast<int>(spawn_diam*(pr_gen() - 0.5));
				spawn_pos.second = (placement_pr < 0.25) ? spawn_diam / 2 : -spawn_diam / 2;
			}
			else {	// spawn on left/right line of lattice boundary
				spawn_pos.first = (placement_pr < 0.75) ? spawn_diam / 2 : -spawn_diam / 2;
				spawn_pos.second = static_cast<int>(spawn_diam*(pr_gen() - 0.5));
			}
		}
		else if (is_spawn_source_below) { // spawn at origin
			spawn_pos.first = 0;
			spawn_pos.second = 0;
		}
		break;
	}
}

void DLA_2d::push_particle(const std::pair<int, int>& p, std::size_t count) {
	aggregate_map.insert(std::make_pair(p, count));
	aggregate_pq.push(p);
	buffer.push_back(p);
}

bool DLA_2d::aggregate_collision(const std::pair<int,int>& current, const std::pair<int,int>& previous, const double& sticky_pr, std::size_t& count) {
	// particle did not stick to aggregate, increment aggregate_misses counter
	if (sticky_pr > coeff_stick) ++aggregate_misses_;
	// else, if current co-ordinates of particle exist in aggregate
	// or attractor then collision and successful sticking occurred
	else if (aggregate_map.find(current) != aggregate_map.end() || attractor_set.find(current) != attractor_set.end()) {
		// insert previous position of particle to aggregrate_map and aggregrate priority queue
		push_particle(previous, ++count);
		aggregate_span = aggregate_pq.empty() ? 0 : utl::tuple_distance_t<
			decltype(aggregate_pq.top()),
			2>::tuple_distance(aggregate_pq.top(), attractor, attractor_size) - 
			(attractor == attractor_type::CIRCLE ? attractor_size : 0);
		return true;
	}
	return false;
}
