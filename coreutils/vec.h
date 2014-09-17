#ifndef COREUTILS_VEC_H
#define COREUTILS_VEC_H

#include <type_traits>
#include <initializer_list>

namespace utils {

template <class T, int SIZE> struct vbase {
	union {
		T data[SIZE];
		struct {
			T x, y, z, w;
		};
	};
};

// Pointless ?
template <class T> struct vbase<T, 1> {
	union {
		T data[1];
	};
};

template <class T> struct vbase<T, 2> {
	vbase() {}
	vbase(const T& x, const T& y) : data {x,y} {}
	union {
		T data[2];
		struct { T x, y; };
	};

	vbase operator*(const vbase &v) const {
		return vbase(v.x + x, v.y + y);
	}
};

template <class T> struct vbase<T, 3> {
	vbase() {}
	vbase(const T& x, const T& y, const T& z) : data {x,y,z} {}
	union {
		T data[3];
		struct { T x, y, z; };
	};
};

template <class T> struct vbase<T, 4> {
	vbase() {}
	vbase(const T& x, const T& y, const T& z) : data {x,y,z} {}
	union {
		T data[4];
		struct { T x, y, z, w; };
	};
};

template <class T, int SIZE> struct vec : public vbase<T, SIZE> {

	template <typename C, typename R> using is_compound = typename std::enable_if<std::is_compound<C>::value,R>::type;
	template <typename C, typename R> using is_arithmetic = typename std::enable_if<std::is_arithmetic<C>::value,R>::type;
	template <typename C, typename R> using has_index = typename std::conditional<false, decltype(std::declval<C>()[0]), R>::type;

	vec() {}

	vec(const T &x) : vbase<T,SIZE>(x) {}
	vec(const T &x, const T &y) : vbase<T,SIZE>(x, y) {}
	vec(const T &x, const T &y, const T &z) : vbase<T,SIZE>(x, y, z) {}
	//vec(const T &t) : data[0](t) {}

	vec(const std::initializer_list<T> &il) {
		T *ptr = &vbase<T,SIZE>::data[0];
		for(const T& t : il)
			*ptr++ = t;
	}

	bool operator==(const vec &v) const {
		for(int i=0; i<SIZE; i++)
			if(v[i] != vbase<T,SIZE>::data[i]) return false;
		return true;
	}

	template <typename VEC> has_index<VEC, vec> copy(const VEC &v) {

	}

	template <typename VEC> has_index<VEC, vec> operator+(const VEC &v) const {
		vec r;
		for(int i=0; i<SIZE; i++)
			r[i] = vbase<T,SIZE>::data[i] + v[i];
		return r;
	}

	template <typename I> is_arithmetic<I,vec> operator+(const I &t) const {
		vec r;
		for(int i=0; i<SIZE; i++)
			r[i] = vbase<T,SIZE>::data[i] + t;
		return r;
	}

	template <typename VEC> has_index<VEC, vec> operator*(const VEC &v) const {
		vec r;
		for(int i=0; i<SIZE; i++)
			r[i] = vbase<T,SIZE>::data[i] * v[i];
		return r;
	}

	template <typename I> is_arithmetic<I,vec> operator*(const I &t) const {
		vec r;
		for(int i=0; i<SIZE; i++)
			r[i] = vbase<T,SIZE>::data[i] * t;
		return r;
	}

	template <typename VEC> has_index<VEC, vec> operator-(const VEC &v) const {
		vec r;
		for(int i=0; i<SIZE; i++)
			r[i] = vbase<T,SIZE>::data[i] - v[i];
		return r;
	}

	template <typename I> is_arithmetic<I,vec> operator-(const I &t) const {
		vec r;
		for(int i=0; i<SIZE; i++)
			r[i] = vbase<T,SIZE>::data[i] - t;
		return r;
	}

	template <typename VEC> has_index<VEC, vec> operator/(const VEC &v) const {
		vec r;
		for(int i=0; i<SIZE; i++)
			r[i] = vbase<T,SIZE>::data[i] / v[i];
		return r;
	}

	template <typename I> is_arithmetic<I,vec> operator/(const I &t) const {
		vec r;
		for(int i=0; i<SIZE; i++)
			r[i] = vbase<T,SIZE>::data[i] / t;
		return r;
	}

	const T& operator[](const int &i) const {
		return vbase<T,SIZE>::data[i];
	}

	T& operator[](const int &i) {
		return vbase<T,SIZE>::data[i];
	}

	T dist2() {
		T rc;
		for(int i=0; i<SIZE; i++)
			rc += vbase<T,SIZE>::data[i] * vbase<T,SIZE>::data[i];
		return rc;
	}

	T dist() {
		T rc;
		for(int i=0; i<SIZE; i++)
			rc += vbase<T,SIZE>::data[i] * vbase<T,SIZE>::data[i];
		return sqrt(rc);
	}

	template <typename VEC> has_index<VEC, T> dot(const VEC &v) const {
		T r = 0;
		for(int i=0; i<SIZE; i++)
			r += (vbase<T,SIZE>::data[i] * v[i]);
		return r;
	}
};

typedef vec<float, 2> vec2f;
typedef vec<float, 3> vec3f;
typedef vec<float, 4> vec4f;

typedef vec<int, 2> vec2i;
typedef vec<int, 3> vec3i;
typedef vec<int, 4> vec4i;

/*
template <class T, int SIZE> struct mat {

	mat() {
		for(int i=0; i<SIZE*SIZE; i++)
			data[i/SIZE][i%SIZE] = i%(SIZE+1) ? 0.0 : 1.0;
	}

	vec<T, SIZE> operator*(const vec<T, SIZE> &v) const {
		vec<T, SIZE> r;
		for(int i=0; i<SIZE; i++)
			r[i] = data[i].dot(v);
		return r;
	}

	std::string to_string() {
		std::string s;
		for(int i=0; i<SIZE; i++) {
			for(int j=0; j<SIZE; j++) {
				s += std::to_string(data[i][j]);
				s += " ";
			}
			s += "\n";
		}

			return s;
	}

	vec<T, SIZE> data[SIZE];
};
*/


template <class T, class... S> vec<T, sizeof...(S)+1> make_vec(const T& a0, const S& ... args) {
	return vec<T, sizeof...(S)+1>({a0, args...});
};


}
#endif // COREUTILS_VEC_H