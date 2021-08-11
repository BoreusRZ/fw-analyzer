#pragma once
#include <ostream>
#include <gmpxx.h>
namespace gmp {
	using BigInt = mpz_class;
	using BigFloat = mpf_class;
}
inline std::ostream& operator<<(std::ostream& out, const gmp::BigInt& val){
	return out << val.get_str();
}
