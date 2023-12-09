#pragma once
#include <list>
#include <map>
#include <memory>

namespace cxx {

	template <class K, class V>
	class stack {
	public:
		stack();
		stack(const stack&);
		stack(stack&&) noexcept;
		
		// This technically is noexcept, but
		// the copying may throw, so we don't
		// specify it as such for user clarity.
		stack& operator=(stack);

		void push(const K&, const V&);
		void pop();
		// void pop(const K&);

		// std::pair<const K&, V&> front() const;
		// std::pair<const K&, const V&> front() const;
		// V& front(const K&) const;
		// const V& front(const K&) const;

		size_t size() const noexcept;
		size_t count(const K&) const noexcept;

		void clear();

	private:
		class stack_data;

		std::shared_ptr<stack_data> data;
		bool is_unsharable;

		template <class A, class B>
		friend void swap(stack<A, B>&, stack<A, B>&) noexcept;
		stack_data& get_data() const noexcept;
	};

	template<class K, class V>
	class stack<K, V>::stack_data {
	public:
		struct element_t;
		struct value_data_t;
		using value_list_t = std::list<element_t>;
		using map_t = std::map<K, std::shared_ptr<value_data_t>>;
		using stack_list_t = std::list<std::weak_ptr<value_data_t>>;
		struct element_t {
			V value;
			// List iterators are stable.
			stack_list_t::iterator it;
		};
		struct value_data_t {
			value_list_t list;
			// Map iterators are stable.
			map_t::iterator it;
		};

		stack_data();
		stack_data(stack_data&&) noexcept;
		stack_data(const stack_data&);

		stack_list_t stack_list;
		map_t key_map;
		size_t size;
	};

	// ---------- Implementations ---------- //

	// -- stack -- //
	
	template <class K, class V>
	stack<K, V>::stack()
	: data(std::make_shared<stack_data>())
	, is_unsharable(false) {}
	
	template <class K, class V>
	stack<K, V>::stack(stack&& other) noexcept
	: data(std::move(other.data))
	, is_unsharable(other.is_unsharable) {}

	template <class K, class V>
	stack<K, V>::stack(const stack& other)
	: data(nullptr)
	, is_unsharable(false) {
		if (other.is_unsharable) {
			// Make a deep copy (should the constructor throw,
			// no memory will be leaked).
			data = std::make_shared(other.get_data());
		} else {
			// Add another reference.
			data = other.data;
		}
	}

	template <class K, class V>
	stack<K, V>& stack<K, V>::operator=(stack other) {
		// Since the stack is a temporary copy,
		// we can now safely swap our contents.
		swap(*this, &other);
	}

	template <class K, class V>
	void stack<K, V>::push(const K& key, const V& value) {

	}

	template <class K, class V>
	void stack<K, V>::pop() {
		if (size() == 0)
			throw std::invalid_argument("Tried to use pop() on empty stack.");
		// Otherwise we're good to go and no exceptions will be thrown.
		auto& last = *get_data().stack_list.back().get();
		last.list.pop();
		if (last.list.empty()) {
			// Remove the key from the map
			// (also deallocates the value_data).
			get_data().key_map.erase(last.it->first);
		}
		get_data().stack_list.pop_back();
	}

	template <class K, class V>
	size_t stack<K, V>::size() const noexcept {
		return get_data().size;
	}

	template <class K, class V>
	size_t stack<K, V>::count(const K& key) const noexcept {
		const auto& res = get_data().key_map.find(key);

		if (res == get_data().key_map.end()) return 0;
		// Return the size of the corresponding value list.
		return res->second.get()->list.size();
	}

	template <class K, class V>
	void stack<K, V>::clear() {
		if (data.use_count() > 1) {
			// Release the resource and create a new empty one.
			data = std::make_shared<stack_data>();
		} else {
			get_data() = stack_data();
		}
	}

	template <class K, class V>
	void swap(stack<K, V>& a, stack<K, V>& b) noexcept {
		// Enable ADL.
		using std::swap;
		swap(a.data, b.data);
		swap(a.is_unsharable, b.is_unsharable);
	}

	template <class K, class V>
	stack<K, V>::stack_data& stack<K, V>::get_data() const noexcept {
		return *data.get();
	}

	// -- stack_data -- //

	template <class K, class V>
	stack<K, V>::stack_data::stack_data()
	: size(0) {}

	template <class K, class V>
	stack<K, V>::stack_data::stack_data(stack_data&& other) noexcept
	: stack_list(std::move(other.stack_list))
	, key_map(std::move(other.key_map))
	, size(other.size) {}

	template <class K, class V>
	stack<K, V>::stack_data::stack_data(const stack_data& other)
	: stack_list(other.stack_list)
	, key_map(other.key_map)
	, size(other.size) {}
	
}