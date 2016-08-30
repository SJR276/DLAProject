#include "Stdafx.h"
#include "DLA_3d.h"

DLA_3d::DLA_3d(const double& _coeff_stick) : DLAContainer(_coeff_stick) {}

DLA_3d::DLA_3d(LatticeType _lattice_type, AttractorType _attractor_type, const double& _coeff_stick) : DLAContainer(_lattice_type, _attractor_type, _coeff_stick) {}

DLA_3d::DLA_3d(const DLA_3d& _other) : DLAContainer(_other),
	aggregate_map(_other.aggregate_map), aggregate_pq(_other.aggregate_pq), batch_queue(_other.batch_queue) {}

DLA_3d::DLA_3d(DLA_3d&& _other) : DLAContainer(std::move(_other)),
	aggregate_map(std::move(_other.aggregate_map)), aggregate_pq(std::move(_other.aggregate_pq)), batch_queue(std::move(_other.batch_queue)) {}

DLA_3d::~DLA_3d() {}

size_t DLA_3d::size() const noexcept {
	return aggregate_map.size();
}

std::queue<utl::triple<int, int, int>>& DLA_3d::batch_queue_handle() noexcept {
	return batch_queue;
}

void DLA_3d::clear() {
	DLAContainer::clear();
	aggregate_map.clear();
	aggregate_pq = std::priority_queue<utl::triple<int, int, int>, std::vector<utl::triple<int, int, int>>, distance_comparator_3d>();
	batch_queue = std::queue<utl::triple<int, int, int>>();
}

void DLA_3d::generate(size_t _n) {
	// push original sticky point to map and priority queue
	// TODO: alter original sticky seed code for different attractor types (3D)
	std::size_t count = 0;
	utl::triple<int, int, int> origin_sticky = utl::make_triple(0, 0, 0);
	aggregate_map.insert(std::make_pair(origin_sticky, count));
	aggregate_pq.push(origin_sticky);
	batch_queue.push(origin_sticky);
	// initialise current and previous co-ordinate containers
	utl::triple<int, int, int> current = { 0,0,0 };
	utl::triple<int, int, int> prev = current;
	bool has_next_spawned = false;
	// variable to store current allowed size of bounding
	// box spawning zone
	int spawn_diameter = 0;
	// aggregate generation loop
	while (size() < _n || continuous) {
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
		if (aggregate_collision(current, prev, pr_gen(), count)) 
			has_next_spawned = false;
	}
}

double DLA_3d::estimate_fractal_dimension() const {
	// find radius which minimally bounds the aggregate
	int rmax_sqd = aggregate_pq.top().first*aggregate_pq.top().first + aggregate_pq.top().second*aggregate_pq.top().second + aggregate_pq.top().third*aggregate_pq.top().third;
	double bounding_radius = std::sqrt(rmax_sqd);
	// compute fractal dimension via ln(N)/ln(rmin)
	return std::log(aggregate_map.size()) / std::log(bounding_radius);
}

std::ostream& DLA_3d::write(std::ostream& _os, bool _sort_by_map_value) const {
	using utl::operator<<;
	// sort by order particles were added to the aggregate
	if (_sort_by_map_value) {
		// std::vector container to store aggregate map values
		std::vector<std::pair<std::size_t, utl::triple<int, int, int>>> agg_vec;
		// deep copy elements of aggregate_map to agg_vec
		for (const auto& el : aggregate_map)
			agg_vec.push_back(std::make_pair(el.second, el.first));
		// sort agg_vec using a lambda based on order of particle generation
		std::sort(agg_vec.begin(), agg_vec.end(), [](auto& _lhs, auto& _rhs) {return _lhs.first < _rhs.first; });
		// write sorted data to stream
		for (const auto& el : agg_vec)
			_os << el.second << '\n';
	}
	// output aggregate data "as-is" without sorting
	else {
		for (const auto& el : aggregate_map)
			_os << el.second << '\t' << el.first << '\n';
	}
	return _os;
}

void DLA_3d::spawn_particle(utl::triple<int,int,int>& _current, int& _spawn_diam) noexcept {
	const int boundary_offset = 16;
	// set diameter of spawn zone to double the maximum of the largest distance co-ordinate
	// triple currently in the aggregate structure plus an offset to avoid direct sticking spawns
	int rmax_sqd = aggregate_pq.top().first*aggregate_pq.top().first + aggregate_pq.top().second*aggregate_pq.top().second + aggregate_pq.top().third*aggregate_pq.top().third;
	_spawn_diam = 2 * static_cast<int>(std::sqrt(rmax_sqd)) + boundary_offset;
	double placement_pr = pr_gen();
	// Spawn on negative constant z plane of bounding box
	if (placement_pr < 1.0 / 6.0) {
		_current.first = static_cast<int>(_spawn_diam*(pr_gen() - 0.5));
		_current.second = static_cast<int>(_spawn_diam*(pr_gen() - 0.5));
		_current.third = -_spawn_diam / 2;
	}
	// Spawn on positive constant z plane of bounding box
	else if (placement_pr >= 1.0 / 6.0 && placement_pr < 2.0 / 6.0) {
		_current.first = static_cast<int>(_spawn_diam*(pr_gen() - 0.5));
		_current.second = static_cast<int>(_spawn_diam*(pr_gen() - 0.5));
		_current.third = _spawn_diam / 2;
	}
	// Spawn on negative constant x plane of bounding box
	else if (placement_pr >= 2.0 / 6.0 && placement_pr < 3.0 / 6.0) {
		_current.first = -_spawn_diam / 2;
		_current.second = static_cast<int>(_spawn_diam*(pr_gen() - 0.5));
		_current.third = static_cast<int>(_spawn_diam*(pr_gen() - 0.5));
	}
	// Spawn on positive constant x plane of bounding box
	else if (placement_pr >= 3.0 / 6.0 && placement_pr < 4.0 / 6.0) {
		_current.first = _spawn_diam / 2;
		_current.second = static_cast<int>(_spawn_diam*(pr_gen() - 0.5));
		_current.third = static_cast<int>(_spawn_diam*(pr_gen() - 0.5));
	}
	// Spawn on negative constant y plane of bounding box
	else if (placement_pr >= 4.0 / 6.0 && placement_pr < 5.0 / 6.0) {
		_current.first = static_cast<int>(_spawn_diam*(pr_gen() - 0.5));
		_current.second = -_spawn_diam / 2;
		_current.third = static_cast<int>(_spawn_diam*(pr_gen() - 0.5));
	}
	// Spawn on positive constant z plane of bounding box
	else if (placement_pr >= 5.0 / 6.0 && placement_pr < 1.0) {
		_current.first = static_cast<int>(_spawn_diam*(pr_gen() - 0.5));
		_current.second = _spawn_diam / 2;
		_current.third = static_cast<int>(_spawn_diam*(pr_gen() - 0.5));
	}
}

bool DLA_3d::aggregate_collision(const utl::triple<int,int,int>& _current, const utl::triple<int,int,int>& _previous, const double& _sticky_pr, std::size_t& _count) {
	// particle did not stick to aggregate, increment aggregate_misses counter
	if (_sticky_pr > coeff_stick)
		++aggregate_misses_;
	// else, if current co-ordinates of particle exist in aggregate
	// then collision and successful sticking occurred
	else if (aggregate_map.find(_current) != aggregate_map.end()) {
		// insert previous position of particle to aggregrate_map and aggregrate priority queue
		aggregate_map.insert(std::make_pair(_previous, ++_count));
		aggregate_pq.push(_previous);
		batch_queue.push(_previous);
		utl::triple<int,int,int> max_dist = aggregate_pq.top();
		aggregate_radius_sqd_ = max_dist.first*max_dist.first + max_dist.second*max_dist.second + max_dist.third*max_dist.third;
		return true;
	}
	return false;
}