#include "Stdafx.h"
#include "DLA_2d.h"

DLA_2d::DLA_2d(const double& _coeff_stick) : DLAContainer(_coeff_stick) {}

DLA_2d::DLA_2d(LatticeType _lattice_type, AttractorType _attractor_type, const double& _coeff_stick) : DLAContainer(_lattice_type, _attractor_type, _coeff_stick) {}

DLA_2d::DLA_2d(const DLA_2d& _other) : DLAContainer(_other) {
	// deep copy fields of _other to this
	aggregate_map = _other.aggregate_map;
	aggregate_pq = _other.aggregate_pq;
}

DLA_2d::DLA_2d(DLA_2d&& _other) : DLAContainer(_other) {
	// deep copy fields of _other to this
	aggregate_map = _other.aggregate_map;
	aggregate_pq = _other.aggregate_pq;
	// set _other container fields to default values
	_other.aggregate_map = std::unordered_map<std::pair<int, int>, std::size_t, pair_hash>();
	_other.aggregate_pq = std::priority_queue<std::pair<int, int>, std::vector<std::pair<int, int>>, distance_comparator>();
}

DLA_2d::~DLA_2d() {}

std::size_t DLA_2d::size() const noexcept {
	return aggregate_map.size();
}

std::queue<std::pair<int,int>>& DLA_2d::batch_queue_handle() noexcept {
	return batch_queue;
}

void DLA_2d::set_attractor_type(AttractorType _attractor_type) {
	// invalid attractor type for 2D lattice
	if (_attractor_type == AttractorType::PLANE)
		throw std::invalid_argument("Cannot set attractor type of 2D DLA to PLANE.");
	DLAContainer::set_attractor_type(_attractor_type);
}

void DLA_2d::clear() {
	DLAContainer::clear();
	aggregate_map.clear();
	aggregate_pq = std::priority_queue<std::pair<int, int>, std::vector<std::pair<int, int>>, distance_comparator>();
	batch_queue = std::queue<std::pair<int, int>>();
}

void DLA_2d::generate(size_t _n) {
	// push original aggregate point to map and priority queue
	// TODO: alter original sticky seed code for different attractor types (2D)
	std::size_t count = 0;
	std::pair<int, int> origin_sticky = std::make_pair(0, 0);
	aggregate_map.insert(std::make_pair(origin_sticky, count));
	aggregate_pq.push(origin_sticky);
	batch_queue.push(origin_sticky);
	// initialise current and previous co-ordinate containers
	std::pair<int, int> current = { 0, 0 };
	std::pair<int, int> prev = current;
	bool has_next_spawned = false;
	// variable to store current allowed size of bounding
	// box spawning zone
	int spawn_diameter = 0;
	// uniform distribution in [0,1] for probability generation
	std::uniform_real_distribution<> dist(0.0, 1.0);
	// aggregate generation loop 
	while (size() < _n) {
		if (abort_signal) {
			abort_signal = false;
			return;
		}
		// spawn the next particle if previous particle
		// successfully stuck to aggregate structure
		if (!has_next_spawned) {
			spawn_particle(current, spawn_diameter, dist);
			has_next_spawned = true;
		}
		// generate random double in [0,1] for movement choice direction
		double movement_choice = dist(mt_eng);
		prev = current;
		// update position of particle via unbiased random walk
		update_particle_position(current, movement_choice);
		// check for collision with bounding walls and reflect if true
		lattice_boundary_collision(current, prev, spawn_diameter);
		double stick_pr = dist(mt_eng);
		// check for collision with aggregate structure and add particle to 
		// the aggregate (both to map and pq) if true, set flag ready for
		// next particle spawn
		if (aggregate_collision(current, prev, stick_pr, count)) {
			has_next_spawned = false;
		}
	}
}

double DLA_2d::estimate_fractal_dimension() const {
	// find radius which minimally bounds the aggregate
	double bounding_radius = std::hypot(aggregate_pq.top().first, aggregate_pq.top().second);
	// compute fractal dimension via ln(N)/ln(rmin)
	return std::log(aggregate_map.size()) / std::log(bounding_radius);
}

std::ostream& DLA_2d::write(std::ostream& _os, bool _sort_by_map_value) const {
	using utl::operator<<;
	// sort by order particles were added to the aggregate
	if (_sort_by_map_value) {
		// std::vector container to store aggregate_map values
		std::vector<std::pair<std::size_t, std::pair<int, int>>> agg_vec;
		// deep copy elements of aggregate_map to agg_vec
		for (auto it = aggregate_map.cbegin(); it != aggregate_map.cend(); ++it) {
			agg_vec.push_back(std::make_pair(it->second, it->first));
		}
		// lambda for sorting aggregate via order in which particles were generated
		auto sort_agg = [](const std::pair<std::size_t, std::pair<int, int>>& _lhs, const std::pair<std::size_t, std::pair<int, int>>& _rhs) {return _lhs.first < _rhs.first; };
		// sort agg_vec using the lambda sort_agg
		std::sort(agg_vec.begin(), agg_vec.end(), sort_agg);
        // write sorted data to stream
		for (auto it = agg_vec.cbegin(); it < agg_vec.cend(); ++it) {
			_os << it->second << "\n";
		}
	}
	// output aggregate data "as-is" without sorting
	else {
		for (auto it = aggregate_map.cbegin(); it != aggregate_map.cend(); ++it) {
			_os << it->second << "\t" << it->first << "\n";
		}
	}
	return _os;
}

void DLA_2d::spawn_particle(std::pair<int,int>& _spawn_pos, int& _spawn_diam, std::uniform_real_distribution<>& _dist) noexcept {
	const int boundary_offset = 16;
	// set diameter of spawn zone to double the maximum of the largest distance co-ordinate
	// pair currently in the aggregate structure plus an offset to avoid direct sticking spawns
	_spawn_diam = 2 * static_cast<int>(std::hypot(aggregate_pq.top().first, aggregate_pq.top().second)) + boundary_offset;
	// generate random double in [0,1]
	double placement_pr = _dist(mt_eng);
	// spawn on upper line of lattice boundary
	if (placement_pr < 0.25) {
		_spawn_pos.first = static_cast<int>(_spawn_diam*(_dist(mt_eng) - 0.5));
		_spawn_pos.second = _spawn_diam / 2;
	}
	// spawn on lower line of lattice boundary
	else if (placement_pr >= 0.25 && placement_pr < 0.5) {
		_spawn_pos.first = static_cast<int>(_spawn_diam*(_dist(mt_eng) - 0.5));
		_spawn_pos.second = -_spawn_diam / 2;
	}
	// spawn on right line of lattice boundary
	else if (placement_pr >= 0.5 && placement_pr < 0.75) {
		_spawn_pos.first = _spawn_diam / 2;
		_spawn_pos.second = static_cast<int>(_spawn_diam*(_dist(mt_eng) - 0.5));
	}
	// spawn on left line of lattice boundary
	else {
		_spawn_pos.first = -_spawn_diam / 2;
		_spawn_pos.second = static_cast<int>(_spawn_diam*(_dist(mt_eng) - 0.5));
	}
}

bool DLA_2d::aggregate_collision(const std::pair<int,int>& _current, const std::pair<int,int>& _previous, const double& _sticky_pr, std::size_t& _count) {
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
		aggregate_radius_ = std::hypot(aggregate_pq.top().first, aggregate_pq.top().second);
		return true;
	}
	return false;
}
