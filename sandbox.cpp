#include "stack.h"
#include <iostream>
#include <assert.h>
#include <vector>
using namespace cxx;

class A {
public:
	void func() {
		std::cout<<"A::func()\n";
	}
};
class B {
public:
	void gunc() {
		std::cout<<"B::gunc()\n";
	}
};

template <class T>
concept hasfunc = requires(T t) {
	t.func();
};

template <class T>
void fgunc(T t) {
	if constexpr (hasfunc<T>) {
		std::cout<<"has func!\n";
		t.func();
	} else {
		std::cout<<"no func here!\n";
		t.gunc();
	}
}

template <class A, class B, class C, class D>
bool peq(std::pair<A, B> x, std::pair<C, D> y) {
	return x.first == y.first && x.second == y.second;
}

bool has_elements(stack<int, int>& s, const std::vector<std::pair<int, int>>& vals) {
	if (s.size() != vals.size()) return false;
	for (int i = vals.size() - 1; i >= 0; --i) {
		auto top = s.front();
		if (!peq(top, vals[i])) return false;
		s.pop();
	}
	return true;
}

int main() {

	// ----------------------------------------------------------------------------
	stack<int, int> s;
	assert(s.size() == 0 && s.count(0) == 0 && s.count(1) == 0);
	s.push(1, 2);
	s.push(1, 3);
	s.push(2, 5);
	assert(s.size() == 3 && s.count(1) == 2 && s.count(2) == 1);
	s.pop();
	assert(s.size() == 2 && s.count(1) == 2 && s.count(2) == 0);
	s.pop();
	assert(s.size() == 1 && s.count(1) == 1 && s.count(2) == 0);
	s.pop();
	assert(s.size() == 0 && s.count(1) == 0 && s.count(2) == 0);
	s.push(2, 5);
	s.push(2, 5);
	s.push(2, 5);
	assert(s.size() == 3 && s.count(2) == 3);
	s.clear();
	assert(s.size() == 0 && s.count(2) == 0);

	s.push(1, 1);
	s.push(1, 2);
	s.push(2, 1);
	assert(s.front().first == 2 && s.front().second == 1);
	std::pair<const int&, const int&> cref = s.front();
	assert(cref.first == 2 && cref.second == 1);

	const stack<int, int> s2(s);
	assert(s2.front().first == 2 && s2.front().second == 1);

	std::pair<const int&, int&> ref = s.front();
	ref.second = 3;
	assert(s.front().first == 2 && s.front().second == 3);

	assert(s.front(1) == 2);
	assert(s.front(2) == 3);

	s.push(3, 1);
	s.push(2, 4);
	s.push(1, 1);
	s.push(0, 1);
	auto it = s.cbegin();
	assert(s.count(*it) == 1);
	++it;
	assert(s.count(*it) == 3);
	auto it2 = it++;
	assert(s.count(*it2) == 3);
	assert(s.count(*it) == 2);
	++it2; ++it2;
	assert(s.count(*it2) == 1);
	++it2;
	assert(it2 == s.cend());

	// ----------------------------------------------------------------------------
	int sum = 0;
	for (auto it = s.cbegin(); it != s.cend(); ++it) {
		sum += *it;
	}
	assert(sum == 6);

	// ----------------------------------------------------------------------------
	static_assert(std::forward_iterator<stack<int, int>::const_iterator>);

	// ----------------------------------------------------------------------------
	stack<int, int> stack66;
	stack66.push(1,1);
	stack stack77 = stack66;
	stack66.push(2,1);
	assert(has_elements(stack66, {{1, 1}, {2, 1}}));
	assert(has_elements(stack77, {{1, 1}}));

	// ----------------------------------------------------------------------------
	stack<int, int> stuck1;
	stuck1.push(1, 1);
	stuck1.push(1, 2);
	stuck1.push(1, -1);
	stuck1.push(-2, -2);
	stack<int, int> stuck2;
	stuck2 = stuck1;
	assert(has_elements(stuck2, {{1, 1}, {1, 2}, {1, -1}, {-2, -2}}));
	stack<int, int> stuck3(stuck1);
	assert(has_elements(stuck3, {{1, 1}, {1, 2}, {1, -1}, {-2, -2}}));
	stack<int, int> stuck11(stuck1);
	stack<int, int> stuck4 = std::move(stuck1);
	assert(has_elements(stuck4, {{1, 1}, {1, 2}, {1, -1}, {-2, -2}}));
	stack<int, int> stuck5;
	std::swap(stuck11, stuck5);
	assert(has_elements(stuck5, {{1, 1}, {1, 2}, {1, -1}, {-2, -2}}));
	assert(has_elements(stuck11, {}));
	// -----------------------------------------------------------------------------
	stack<int, int> steck1{};

	for (int i = 0; i < 10; i++) {
		try {
		steck1.push(i, i);
		} catch(...) {}
	}
	int ctr = 9;
	for (stack<int, int>::const_iterator it = steck1.cbegin(); it != steck1.cend(); ++it) {
		try {
		auto front = steck1.front();
		assert(front.first == front.second && front.first == ctr--);
		steck1.pop();
		} catch(...) {}
	}
	assert(ctr == 4);
	return 0;
}