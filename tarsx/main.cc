#include <memory>

#include "ThreadDeque.hpp"

int main() {

	tarsx::ThreadDeque<std::unique_ptr<int>,std::deque<std::unique_ptr<int>>> a;
	a.test();
	auto c = a.pop_front();
	return 0;
}