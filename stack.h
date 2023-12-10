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
        void pop(const K&);

        std::pair<const K&, V&> front();
        std::pair<const K&, const V&> front() const;
        V& front(const K&);
        const V& front(const K&) const;

        size_t size() const noexcept;
        size_t count(const K&) const noexcept;

        void clear();

        class const_iterator;
        const_iterator cbegin() const noexcept;
        const_iterator cend() const noexcept;
        
        template <class A, class B>
        friend void swap(stack<A, B>&, stack<A, B>&) noexcept;
    private:
        class stack_data;
        std::shared_ptr<stack_data> data;
        bool is_unsharable;

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
            element_t();
            element_t(const V&, const stack_list_t::iterator);
        };
        struct value_data_t {
            value_list_t list;
            // Map iterators are stable.
            map_t::iterator it;
        };

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

        void clear();

        stack_list_t stack_list;
        map_t key_map;
        size_t size;
    };

    // A wrapper for the map's const iterator.
    template<class K, class V>
    requires std::forward_iterator<K>
    class stack<K, V>::const_iterator {
    public:
        const_iterator();
        const_iterator(stack_data::map_t::const_iterator);
        const_iterator(const const_iterator&);

        const_iterator& operator=(const const_iterator&);

        const_iterator& operator++();
        const_iterator operator++(int);

        const K& operator*() const;
        const K& operator->() const;

        bool operator==(const const_iterator&) const;
        bool operator!=(const const_iterator&) const;

    private:
        stack_data::map_t::const_iterator it;
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
    stack<K, V>& stack<K, V>::operator=(stack other) {
        // Since the stack is a temporary copy,
        // we can now safely swap our contents.
        swap(*this, other);
        return *this;
    }

    template <class K, class V>
    void stack<K, V>::push(const K& key, const V& value) {
        get_data().push(key, value);
    }

    template <class K, class V>
    void stack<K, V>::pop() {
        get_data().pop();
    }

    template <class K, class V>
    void stack<K, V>::pop(const K& k) {
        get_data().pop(k);
    }

    template <class K, class V>
    std::pair<const K&, V&> stack<K, V>::front() {
        return get_data().front();
    }

    template <class K, class V>
    std::pair<const K&, const V&> stack<K, V>::front() const {
        return get_data().front();
    }

    template <class K, class V>
    V& stack<K, V>::front(const K& k) {
        return get_data().front(k);
    }

    template <class K, class V>
    const V& stack<K, V>::front(const K& k) const {
        return get_data().front(k);
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
            get_data().clear();
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
                ptr.get()->list.push_back(new_el);
                // Insert to key_map [member modified].
                key_map.insert({key, ptr});
                ptr.get()->it = key_map.find(key); // noexcept
            } else {
                ptr = it->second;
                // Insert new element to list [member modified].
                ptr.get()->list.push_back(new_el);
            }
            *stack_it = std::weak_ptr<value_data_t>(ptr); // noexcept
        } catch(...) {
            // Rollback stack_list change.
            stack_list.pop_back();
            throw;
        }
        ++size;
    }

    template <class K, class V>
    void stack<K, V>::stack_data::pop() {
        if (size == 0)
            throw std::invalid_argument("Tried to use pop() on empty stack.");

        // Otherwise we're good to go and no exceptions will be thrown.
        auto last = stack_list.back().lock().get();
        last->list.pop_back();
        if (last->list.empty()) {
            // Remove the key from the map
            // (also deallocates the value_data).
            key_map.erase(last->it->first);
        }
        stack_list.pop_back();
        --size;
    }

    template <class K, class V>
    void stack<K, V>::stack_data::pop(const K& k) {
        if (size == 0)
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
        --size;
    }

    template <class K, class V>
    std::pair<const K&, V&> stack<K, V>::stack_data::front() {
        if (size == 0)
            throw std::invalid_argument("Tried to use front() on empty stack.");

        // Otherwise we're good to go and no exceptions will be thrown.
        auto last = stack_list.back().lock().get();
        return {last->it->first, last->list.back().value};
    }

    template <class K, class V>
    V& stack<K, V>::stack_data::front(const K& k) {
        if (size == 0)
            throw std::invalid_argument("Tried to use pop(const K& k) on empty stack.");

        auto last_with_key = key_map.find(k);
        if(last_with_key == key_map.end())
            throw std::invalid_argument("Tried to use pop(const K& k) on stack with no key k.");

        // Otherwise we're good to go and no exceptions will be thrown.
        return last_with_key->second->list.back().value;
    }

    template <class K, class V>
    void stack<K, V>::stack_data::clear() {
        stack_list.clear();
        key_map.clear();
        size = 0;
    }

    template <class K, class V>
    stack<K, V>::stack_data::element_t::element_t() {}

    template <class K, class V>
    stack<K, V>::stack_data::element_t::element_t(const V& value, const stack_list_t::iterator it)
            : value(value)
            , it(it) {}


    // -- const_iterator -- //

    template <class K, class V>
    stack<K, V>::const_iterator::const_iterator() {}

    template <class K, class V>
    stack<K, V>::const_iterator::const_iterator(stack_data::map_t::const_iterator it)
            : it(it) {}

    template <class K, class V>
    stack<K, V>::const_iterator::const_iterator(const const_iterator& other)
            : it(other.it) {}

    template <class K, class V>
    stack<K, V>::const_iterator &
            stack<K, V>::const_iterator::operator=(const const_iterator& iter) {
        it = iter.it;
    }

    template <class K, class V>
    stack<K, V>::const_iterator &
            stack<K, V>::const_iterator::operator++() {
        ++it;
        return *this;
    }

    template <class K, class V>
    stack<K, V>::const_iterator
            stack<K, V>::const_iterator::operator++(int) {
        const_iterator tmp(*this);
        operator++();
        return tmp;
    }

    template <class K, class V>
    bool stack<K, V>::const_iterator::operator==(const const_iterator& iter) const {
        return it == iter.it;
    }

    template <class K, class V>
    bool stack<K, V>::const_iterator::operator!=(const const_iterator& iter) const {
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

    template <class K, class V>
    stack<K, V>::const_iterator stack<K, V>::cbegin() const noexcept {
        return const_iterator(get_data().key_map.cbegin());
    }

    template <class K, class V>
    stack<K, V>::const_iterator stack<K, V>::cend() const noexcept {
        return const_iterator(get_data().key_map.cend());
    }
}

