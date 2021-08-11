#pragma once
#include <ostream>
#include <ranges>
#include <cstring>
#include <cassert>

namespace bor {
	template<typename T>
	class vector {
	private:
		T* _data = nullptr;
		size_t capacity_m = 0;
		size_t length = 0;

		template<bool init>
		void grow(){
			capacity_m *= 2;
			if(capacity_m == 0)capacity_m = 8;
			T* new_data;
			/* if constexpr(init){ */
				new_data = new T[capacity_m];
			/* }else{ */
			/* 	new_data = reinterpret_cast<T*>(malloc(sizeof(T) * capacity)); */
			/* } */
			/* memcpy(new_data,_data,sizeof(T)*length); */
			for(size_t i = 0; i < length; ++i)new_data[i] = _data[i];
			T* temp = _data;
			_data = new_data;
			if(temp != nullptr)delete[] temp;
		}
	public:
		using value_type = T;
		using self_t = vector<T>;

		vector() = default;
		vector(std::initializer_list<T> list) {
			if(std::empty(list))return;
			/* _data = (T*)malloc(sizeof(T)*list.size()); */
			_data = new T[list.size()];
			capacity_m = list.size();
			for(const auto& val : list)push_back(val);
		}
		vector(size_t size) {
			resize(size);
		}
		vector(T* _data, size_t size) : _data(_data), length(size), capacity_m(size) {}

		vector(bor::vector<T>&& other) : _data(other._data), capacity_m(other.capacity_m), length(other.length){
			other._data = nullptr;
			other.capacity_m = 0;
			other.length = 0;
		}
		template<std::ranges::sized_range range>
		vector(const range& other) : capacity_m(other.size()), length(other.size()){
			_data = new T[capacity_m];
			/* _data = (T*)malloc(sizeof(T)*other.length); */
			/* memcpy(_data,other._data,sizeof(T)*length); */
			for(size_t i = 0; i < length; ++i)_data[i] = other[i];
		}
		vector(const vector<T>& other) : capacity_m(other.length), length(other.length){
			_data = new T[capacity_m];
			/* _data = (T*)malloc(sizeof(T)*other.length); */
			/* memcpy(_data,other._data,sizeof(T)*length); */
			for(size_t i = 0; i < length; ++i)_data[i] = other[i];
		}
		~vector(){
			if(_data != nullptr)delete[] _data;
		}
		void push_back(const T& t){
			if(length == capacity_m)grow<false>();
			_data[length++] = t;
		}
		template<typename ...Args>
		void emplace_back(Args ... args){
			if(length == capacity_m)grow<false>();
			_data[length++] = T(std::forward<Args>(args)...);
		}

		size_t capacity() const{
			return capacity_m;
		}
		size_t size() const{
			return length;
		}
		const T& operator[](size_t index) const {
			assert(index < length);
			return _data[index];
		}
		T& operator[](size_t index) {
			assert(index < length);
			return _data[index];
		}

		template<std::ranges::sized_range range>
		bool operator==(const range& other) const{
			if(other.size() != size()) return false;
			for(size_t i = 0; i < size(); ++i){
				if(other[i] != (*this)[i])return false;
			}
			return true;
		}
		auto begin() const{
			return _data;
		}
		auto end() const{
			return _data+length;
		}
		auto begin(){
			return _data;
		}
		auto end(){
			return _data+length;
		}
		void reserve(size_t size){
			if(capacity_m < size){
				capacity_m = size/2+1;
				grow<false>();
			}
		}
		void clear(){
			length = 0;
		}
		void resize_no_init(size_t size){
			reserve(size);
			length = size;
		}
		void resize(size_t size){
			if(capacity_m < size){
				capacity_m = size/2+1;
				grow<true>();
			}
			length = size;
		}
		self_t& operator=(self_t&& other){
			if(_data != nullptr)delete[] _data;
			_data = other._data;
			length = other.length;
			capacity_m = other.capacity_m;
			other._data = nullptr;
			other.capacity_m = 0;
			other.length = 0;
			return *this;
		}
		self_t& operator=(const self_t& other){
			if(capacity_m < other.size()){
				delete[] _data;
				_data = new T[other.size()];
				capacity_m = other.size();
			}
			length = other.size();
			for(size_t i = 0; i < other.size(); ++i){
				_data[i] = other[i];
			}
			return *this;
		}
		bool empty() const {
			return length == 0;
		}
		void pop_back(){
			assert(length != 0);
			--length;
		}
		const value_type& back() const{
			assert(length != 0);
			return _data[length-1];
		}
		value_type& back(){
			assert(length != 0);
			return _data[length-1];
		}
		const auto data() const{
			return _data;
		}
		auto data(){
			return _data;
		}
		void insert_back(auto start, auto end){
			while(start != end){
				push_back(*start);
				++start;
			}
		}

	};
}
template<typename R>
std::ostream& operator<<(std::ostream& out, const bor::vector<R>& vec){
	for(const auto& val : vec){
		out << val << ",";
	}
	return out;
}
