#pragma once
#include <tuple>
#include <iostream>
#include <concepts>
#include <unordered_map>
#include <fmt/core.h>

namespace util {
	/**
	 * @brief maps inserted objects of type T to unique ids of type index_t
	 */
	template<typename T, typename index_t = uint8_t>
	class indexer {
	public:
		auto getIndex(const T& t) -> index_t{
			auto iter = lookup.find(t);
			if(iter != lookup.end())return iter->second;
			else {
				lookup[t] = lookup.size();
				return lookup.size()-1;
			}
		}
		auto operator[](const T& t){
			return getIndex(t);
		}
	private:
		std::unordered_map<T,index_t> lookup;
	};

	/**
	 * prints any Type T in its binary representation
	 */
	template<typename T>
	void printBinary(const T& t){
		using byte = unsigned char;
		byte * bytes = (byte*)(&t);
		for(int i = 0; i < sizeof(t); ++i){
			if(i%4 == 0)std::cout << std::endl;
			else std::cout << ' ';
			for(int bit = 7; bit >= 0; --bit){
				std::cout << ((bytes[i]&(1<<bit)) != 0 ? '1' : '0');
			}
		}
		std::cout << std::endl;
	}




	/**
	 * takes an input-iterator
	 * @return a tuple with @b amount values in the iterator\n
	 * the iterator will be advanced by how many elements have been taken
	 */
	template<size_t amount>
	constexpr auto unpack(auto&& iter){
		if constexpr(amount == 0){
		}if constexpr (amount == 1){
			return std::make_tuple(*iter);
		}else{
			auto ret = *iter;
			return std::tuple_cat(std::make_tuple(std::move(ret)),unpack<amount-1>(++iter));
		}
	}

	/**
	 * iterator factory class for getting substrings\n
	 * in the input around a given delimiter\n
	 * e.g. "abc,d,efg" with delimiter "," will return a range representing
	 * ["abc","d","efg"]
	 */
	class split_range{
		const std::string_view text, delim;
		public:
		split_range(std::string_view sv, std::string_view delim);

		using value_type = std::string_view;
		struct iterator{
			using value_type = std::string_view;
			const std::string_view text, delim;

			size_t start = 0;
			size_t end = 0;
			bool end_flag = false;

			iterator() = default;
			iterator(const split_range& parent);

			constexpr bool operator==(const iterator& other) const {
				return (end_flag && other.end_flag) || (!(end_flag || other.end_flag) && end == other.end);
			};
			constexpr bool operator!=(const iterator& other) const {
				return !(*this == other);
			}
			auto operator++(int) -> iterator {
				auto ret = *this;
				++*this;
				return ret;
			}
			constexpr auto operator++() -> iterator& {
				end += delim.size();
				start = end;
				end = text.find(delim,end);
				if(end == std::string::npos){
					end = text.size();
					if(start > end){
						end_flag = true;
						start = end;
					}
				}
				return *this;
			}
			constexpr auto operator*() -> std::string_view {
				return std::string_view(std::begin(text)+start,end-start);
			}
		};
		
		iterator begin() const;
		iterator end() const;
	};
	auto split(std::string_view text, std::string_view delim) -> split_range;
		

	//!@returns the memory usage of a vectors capacity in bytes
	template<typename T>
	auto getMemoryUsage_raw(const T& container) -> size_t {
		return sizeof(typename T::value_type) * container.capacity();
	}
	//!@returns the memory usage of a vector as a string 
	//!the amount will be converted to KB,MB,GB acordingly
	template<typename T>
	auto getMemoryUsage(const T& container) -> std::string {
		double bytes = getMemoryUsage_raw(container);
		auto names = std::to_array({
				 "B","KB","MB","GB","TB"
			});
		size_t i = 0;
		for(; i < names.size() && bytes >= 1024; ++i){
			bytes /= 1024;
		}
		return fmt::format("{:.2f} {}",bytes,names[i]);
	}

	template <auto Start, auto End, auto Inc, class F>
	constexpr void constexpr_for(F&& f)
	{
		if constexpr (Start < End)
		{
			f(std::integral_constant<decltype(Start), Start>());
			constexpr_for<Start + Inc, End, Inc>(f);
		}
	}

	auto isTTY() -> bool;
}
