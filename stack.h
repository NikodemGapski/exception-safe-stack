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

        stack& operator=(stack) noexcept;

        void push(const K&, const V&);
        void pop();
        void pop(const K&);

        std::pair<const K&, V&> front();
        std::pair<const K&, const V&> front() const;
        V& front(const K&);
        const V& front(const K&) const;

        size_t size() const noexcept;
        size_t count(const K&) const;

        void clear();

        class const_iterator;
        const_iterator cbegin() const noexcept;
        const_iterator cend() const noexcept;

    private:
        class stack_data;
        std::shared_ptr<stack_data> data;
        bool is_unsharable;

        // Normally, we'd make this a free function,
        // but there is no mention of swap in the specification,
        // so we have to leave it hidden.
        void swap(stack&, stack&) noexcept;

        using new_state_t = std::pair<std::shared_ptr<stack_data>, bool>;
        
        stack_data& get_data() const noexcept;
        stack_data& get_data(const new_state_t&) const noexcept;
        void assume_state(const new_state_t&) noexcept;

        new_state_t make_copy_if_needed(bool) const;
    };

    template<class K, class V>
    class stack<K, V>::stack_data {
    public:
        // Types.
        struct element_t;
        struct value_data_t;
        using value_list_t = std::list<element_t>;
        using map_t = std::map<K, std::shared_ptr<value_data_t>>;
        using stack_list_t = std::list<std::weak_ptr<value_data_t>>;
        struct element_t {
            V value;
            // List iterators are stable.
            stack_list_t::iterator it;
            element_t() = default;
            element_t(const V&, stack_list_t::iterator);
        };
        struct value_data_t {
            value_list_t list;
            // Map iterators are stable.
            map_t::iterator it;
        };

        // Member variables.
        stack_list_t stack_list;
        map_t key_map;

        // Member methods.
        stack_data();
        stack_data(stack_data&&) noexcept;
        stack_data(const stack_data&);

        void push(const K&, const V&);
        void pop();
        void pop(const K&);

        // Const V members are not needed as stack_data
        // is never const and so the stack methods
        // can perform the appropriate conversion.
        std::pair<const K&, V&> front();
        V& front(const K&);

        void clear() noexcept;
        size_t size() noexcept;
    };

    // A wrapper for the map's const iterator.
    template<class K, class V>
    class stack<K, V>::const_iterator {
        using map_t_it = stack_data::map_t::const_iterator;
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = K;
        using difference_type = map_t_it::difference_type;

        const_iterator() = default;
        const_iterator(const const_iterator&) noexcept;

        const_iterator& operator=(const const_iterator&) noexcept;

        const_iterator& operator++() noexcept;
        const_iterator operator++(int) noexcept;

        const K& operator*() const;
        const K& operator->() const;

        bool operator==(const const_iterator&) const noexcept;
        bool operator!=(const const_iterator&) const noexcept;

    private:
        map_t_it it;

        const_iterator(map_t_it) noexcept;

        // Can construct the iterator by providing a map_t_it.
        friend const_iterator stack::cbegin() const noexcept;
        friend const_iterator stack::cend() const noexcept;
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
            data = std::make_shared<stack_data>(other.get_data());
        } else {
            // Add another reference.
            data = other.data;
        }
    }

    template <class K, class V>
    stack<K, V>& stack<K, V>::operator=(stack other) noexcept {
        // Since the stack is a temporary copy,
        // we can now safely swap our contents.
        swap(*this, other);
        return *this;
    }

    template <class K, class V>
    void stack<K, V>::push(const K& key, const V& value) {
        auto new_state = make_copy_if_needed(false);
        get_data(new_state).push(key, value);
        assume_state(new_state);
    }

    template <class K, class V>
    void stack<K, V>::pop() {
        auto new_state = make_copy_if_needed(false);
        get_data(new_state).pop();
        assume_state(new_state);
    }

    template <class K, class V>
    void stack<K, V>::pop(const K& k) {
        auto new_state = make_copy_if_needed(false);
        get_data(new_state).pop(k);
        assume_state(new_state);
    }

    template <class K, class V>
    std::pair<const K&, V&> stack<K, V>::front() {
        assume_state(make_copy_if_needed(true));
        return get_data().front();
    }

    template <class K, class V>
    std::pair<const K&, const V&> stack<K, V>::front() const {
        return get_data().front();
    }

    template <class K, class V>
    V& stack<K, V>::front(const K& k) {
        assume_state(make_copy_if_needed(true));
        return get_data().front(k);
    }

    template <class K, class V>
    const V& stack<K, V>::front(const K& k) const {
        return get_data().front(k);
    }

    template <class K, class V>
    size_t stack<K, V>::size() const noexcept {
        return get_data().size();
    }

    template <class K, class V>
    size_t stack<K, V>::count(const K& key) const {
        const auto& res = get_data().key_map.find(key);

        if (res == get_data().key_map.end()) return 0;
        // Return the size of the corresponding value list.
        return res->second->list.size();
    }

    template <class K, class V>
    void stack<K, V>::clear() {
        if (data.use_count() > 1) {
            // Release the resource and create a new empty one.
            data = std::make_shared<stack_data>();
        } else {
            get_data().clear();
        }
        is_unsharable = false;
    }

    template <class K, class V>
    stack<K, V>::const_iterator stack<K, V>::cbegin() const noexcept {
        return const_iterator(get_data().key_map.cbegin());
    }

    template <class K, class V>
    stack<K, V>::const_iterator stack<K, V>::cend() const noexcept {
        return const_iterator(get_data().key_map.cend());
    }

    template <class K, class V>
    void stack<K, V>::swap(stack& a, stack& b) noexcept {
        // This swap omits the need for move assignment
        // in stack<K, V>.
        std::swap(a.data, b.data);
        std::swap(a.is_unsharable, b.is_unsharable);
    }

    template <class K, class V>
    stack<K, V>::stack_data& stack<K, V>::get_data() const noexcept {
        return *data;
    }

    template <class K, class V>
    stack<K, V>::stack_data&
    stack<K, V>::get_data(const new_state_t& state) const noexcept {
        return *state.first;
    }

    template <class K, class V>
    void stack<K, V>::assume_state(const new_state_t& state) noexcept {
        std::tie(data, is_unsharable) = state;
    }

    template <class K, class V>
    stack<K, V>::new_state_t
    stack<K, V>::make_copy_if_needed(bool mark_unshared) const {
        if (data.use_count() > 1) {
            return {std::make_shared<stack_data>(get_data()), mark_unshared};
        }
        // The problem specification states that we
        // must *always* assume mark_unshared, even if
        // we run at risk of having a rogue reference
        // to multiple data instances (it's the user's
        // responsibility not to use such reference).
        return {data, mark_unshared};
    }

    // -- stack_data -- //

    template <class K, class V>
    stack<K, V>::stack_data::stack_data() {}

    template <class K, class V>
    stack<K, V>::stack_data::stack_data(stack_data&& other) noexcept
            : stack_list(std::move(other.stack_list))
            , key_map(std::move(other.key_map)) {}

    template <class K, class V>
    stack<K, V>::stack_data::stack_data(const stack_data& other) {
        // We have to make a deep copy.
        std::map<K, typename value_list_t::iterator> next_value;
        for (auto& el : other.stack_list) {
            // Retrieve the (key, value) pair.
            auto& value_data = *el.lock();
            const K& key = value_data.it->first;
            auto it = next_value.find(key);
            if (it == next_value.end()) {
                it = next_value.insert({key, value_data.list.begin()}).first;
            }
            push(key, it->second->value);
            ++it->second;
        }
    }

    template <class K, class V>
    void stack<K, V>::stack_data::push(const K& key, const V& value) {
        // Append to stack list [member modified].
        stack_list.push_back(std::weak_ptr<value_data_t>());
        auto stack_it = stack_list.end(); --stack_it;
        try {
            // Create a new value element.
            element_t new_el(value, stack_it);
            // Create a new value data object
            // if one doesn't already exist.
            auto it = key_map.find(key);
            std::shared_ptr<value_data_t> ptr;
            if (it == key_map.end()) {
                ptr = std::make_shared<value_data_t>();
                ptr->list.push_back(new_el);
                // Insert to key_map [member modified].
                ptr->it = key_map.insert({key, ptr}).first;
            } else {
                ptr = it->second;
                // Insert new element to list [member modified].
                ptr->list.push_back(new_el);
            }
            *stack_it = std::weak_ptr<value_data_t>(ptr); // nothrow
        } catch(...) {
            // Rollback stack_list change.
            stack_list.pop_back();
            throw;
        }
    }

    template <class K, class V>
    void stack<K, V>::stack_data::pop() {
        if (size() == 0)
            throw std::invalid_argument("Tried to use pop() on empty stack.");

        // Otherwise we're good to go and no exceptions will be thrown.
        auto last = stack_list.back().lock();
        last->list.pop_back();
        if (last->list.empty()) {
            // Remove the key from the map
            // (also deallocates the value_data).
            key_map.erase(last->it->first);
        }
        stack_list.pop_back();
    }

    template <class K, class V>
    void stack<K, V>::stack_data::pop(const K& k) {
        if (size() == 0)
            throw std::invalid_argument("Tried to use pop(const K& k) on empty stack.");

        auto last_with_key = key_map.find(k);
        if(last_with_key == key_map.end())
            throw std::invalid_argument("Tried to use pop(const K& k) on stack with no key k.");

        // Otherwise we're good to go and no exceptions will be thrown.
        auto last = last_with_key->second;
        auto to_del = last->list.back().it;
        last->list.pop_back();
        if (last->list.empty()) {
            // Remove the key from the map
            // (also deallocates the value_data).
            key_map.erase(last_with_key->second->it->first);
        }
        stack_list.erase(to_del);
    }

    template <class K, class V>
    std::pair<const K&, V&> stack<K, V>::stack_data::front() {
        if (size() == 0)
            throw std::invalid_argument("Tried to use front() on empty stack.");

        // Otherwise we're good to go and no exceptions will be thrown.
        auto last = stack_list.back().lock();
        return {last->it->first, last->list.back().value};
    }

    template <class K, class V>
    V& stack<K, V>::stack_data::front(const K& k) {
        if (size() == 0)
            throw std::invalid_argument("Tried to use pop(const K& k) on empty stack.");

        auto last_with_key = key_map.find(k);
        if(last_with_key == key_map.end())
            throw std::invalid_argument("Tried to use pop(const K& k) on stack with no key k.");

        // Otherwise we're good to go and no exceptions will be thrown.
        return last_with_key->second->list.back().value;
    }

    template <class K, class V>
    void stack<K, V>::stack_data::clear() noexcept {
        stack_list.clear();
        key_map.clear();
    }

    template <class K, class V>
    size_t stack<K, V>::stack_data::size() noexcept {
        return stack_list.size();
    }

    template <class K, class V>
    stack<K, V>::stack_data::element_t::element_t(const V& value, stack_list_t::iterator it)
            : value(value)
            , it(it) {}

    // -- const_iterator -- //

    template <class K, class V>
    stack<K, V>::const_iterator::const_iterator(map_t_it it) noexcept
            : it(it) {}

    template <class K, class V>
    stack<K, V>::const_iterator::const_iterator(const const_iterator& other) noexcept
            : it(other.it) {}

    template <class K, class V>
    stack<K, V>::const_iterator&
    stack<K, V>::const_iterator::operator=(const const_iterator& iter) noexcept {
        it = iter.it;
    }

    template <class K, class V>
    stack<K, V>::const_iterator&
    stack<K, V>::const_iterator::operator++() noexcept {
        ++it;
        return *this;
    }

    template <class K, class V>
    stack<K, V>::const_iterator
    stack<K, V>::const_iterator::operator++(int) noexcept {
        const_iterator tmp(*this);
        operator++();
        return tmp;
    }

    template <class K, class V>
    bool stack<K, V>::const_iterator::operator==(const const_iterator& iter) const noexcept {
        return it == iter.it;
    }

    template <class K, class V>
    bool stack<K, V>::const_iterator::operator!=(const const_iterator& iter) const noexcept {
        return !operator==(iter);
    }

    template <class K, class V>
    const K& stack<K, V>::const_iterator::operator*() const {
        return it->first;
    }

    template <class K, class V>
    const K& stack<K, V>::const_iterator::operator->() const {
        return operator*();
    }
}