#ifndef ADS_SET_H
#define ADS_SET_H

#include <functional>
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <limits>

template <typename Key, size_t N = 7>
class ADS_set {
public:
	class Iterator;
	using value_type = Key;
	using key_type = Key;
	using reference = key_type & ;
	using const_reference = const key_type &;
	using size_type = size_t;
	using difference_type = std::ptrdiff_t;
	using iterator = Iterator;
	using const_iterator = Iterator;
	using key_compare = std::less<key_type>;   // B+-Tree
	using key_equal = std::equal_to<key_type>; // Hashing
	using hasher = std::hash<key_type>;        // Hashing

private:
	struct element {
		element(const key_type& key, element *next_element) {
			this->key = key;
			this->next_element = next_element;
		}

		key_type key;
		element *next_element = nullptr; //next
	};

	struct linked_list {
		element *first_element = nullptr; // Kopf
		size_type sz = 0;

		~linked_list() {
			clear();
		}

		void clear() {
			element* current = first_element;

			while (current != nullptr) {
				element* next = current->next_element;
				delete current;
				current = next;
			}

			first_element = nullptr;
			sz = 0;
		}

		void add(const key_type& key) {
			if (first_element == nullptr) {
				first_element = new element(key, nullptr);
			}
			else {
				element *helper = new element(key, first_element);
				first_element = helper;
			}

			++sz;
		}

		bool erase(const key_type& key) {
			element* current = first_element;
			element* previous = nullptr;

			while (current != nullptr) {
				const auto next = current->next_element;

				if (!key_equal{}(key, current->key)) {
					previous = current;
					current = next;
					continue;
				}

				if (current == first_element) {
					first_element = next;
				}
				else {
					previous->next_element = next;
				}

				delete current;
				sz--;
				return true;
			}

			return false;
		}

		element* contains(const key_type& key) const {
			element* curElement = first_element;
			while (curElement != nullptr) {
				if (key_equal{}(curElement->key, key)) {
					return curElement;
				}
				curElement = curElement->next_element;
			}

			return nullptr;
		}

		element* getElementByIndex(size_type index) const {
			element* curElement = first_element;
			for (size_type i = 0; i < index; i++) {
				if (curElement == nullptr) {
					return nullptr;
				}
				curElement = curElement->next_element;
			}

			return curElement;
		}
	};

	linked_list *table = nullptr;
	size_type max_load_factor{ 70 };

	size_type sz{ 0 };
	size_type max_sz{ N };

	size_type hash(const key_type& key) const { return hasher{}(key) % max_sz; }

	bool isOverloaded() const {
		return 100 * sz / max_sz > max_load_factor;
	}

	void rehash(size_type n) {
		linked_list *newTable = new linked_list[n]();
		size_type oldSize = max_sz;
		max_sz = n;

		for (size_type i = 0; i < oldSize; i++) {
			const linked_list& ll = table[i];
			element* curElement = ll.first_element;
			while (curElement != nullptr) {
				auto newIndex = hash(curElement->key);
				newTable[newIndex].add(curElement->key);
				curElement = curElement->next_element;
			}
		}

		delete[] table;
		table = newTable;
	}

	size_type findNextNonEmptyLinkedList(size_type startIndex) const {
		for (size_type i = startIndex; i < max_sz; i++) {
			if (table[i].sz > 0) {
				return i;
			}
		}

		// Maximale mogliche nummer fur size_type (4'294'967'295)
		return std::numeric_limits<size_type>::max();
	}

	bool insertPrivate(const key_type& key) {
		if (find(key) != end()) {
			return false;
		}

		auto index = hash(key);
		table[index].add(key);
		++sz;

		if (isOverloaded()) {
			rehash(max_sz * 2);
		}

		return true;
	}

public:
	ADS_set() {
		table = new linked_list[N]();
	}

	~ADS_set() {
		delete[] table;
		table = nullptr;
	}

	ADS_set(std::initializer_list<key_type> ilist) : ADS_set() { insert(ilist); }

	template<typename InputIt> ADS_set(InputIt first, InputIt last) : ADS_set() { insert(first, last); }

	ADS_set(const ADS_set &other) : ADS_set() {
		insert(other.begin(), other.end());
	}

	ADS_set &operator=(const ADS_set &other) {
		clear();
		insert(other.begin(), other.end());
		return *this;
	}
	ADS_set &operator=(std::initializer_list<key_type> ilist) {
		clear();
		insert(ilist);
		return *this;
	}

	size_type size() const { return sz; }
	bool empty() const { return sz == 0; }

	size_type count(const key_type &key) const {
		return find(key) != end() ? 1 : 0;
	}

	iterator find(const key_type &key) const {
		const auto hashIndex = hash(key);
		const auto elementPtr = table[hashIndex].contains(key);

		return elementPtr == nullptr ? end() : iterator(((ADS_set<Key, N>*)this), hashIndex, elementPtr);
	}

	void clear() {
		for (size_type i = 0; i < max_sz; i++) {
			table[i].clear();
		}

		sz = 0;
	}

	void swap(ADS_set &other) {
		std::swap(table, other.table);
		std::swap(sz, other.sz);
		std::swap(max_sz, other.max_sz);
	}

	void insert(std::initializer_list<key_type> ilist)
	{
		for (const auto& key : ilist)
			insertPrivate(key);
	}

	template<typename InputIt>
	void insert(InputIt beginIt, InputIt endIt) {
		while (beginIt != endIt) {
			insertPrivate(*beginIt++);
		}
	}

	std::pair<iterator, bool> insert(const key_type &key) {
		const auto it = find(key);
		if (it != end()) {
			return std::make_pair(it, false);
		}

		insertPrivate(key);
		return std::make_pair(find(key), true);
	}

	size_type erase(const key_type &key) {
		if (find(key) == end()) {
			return 0;
		}

		const auto hashIndex = hash(key);
		table[hashIndex].erase(key);
		sz--;
		return 1;
	}

	const_iterator begin() const
	{
		if (sz == 0) {
			return end();
		}

		auto linkedListIndex = findNextNonEmptyLinkedList(0);
		return iterator(((ADS_set<Key, N>*)this), linkedListIndex, table[linkedListIndex].first_element);
	}

	const_iterator end() const
	{
		return iterator(((ADS_set<Key, N>*)this), std::numeric_limits<size_type>::max(), nullptr);
	}

	void dump(std::ostream &o = std::cerr) const {
		bool firstOutput = true;
		o << "[";
		for (const auto& value : *this)
		{
			if (!firstOutput) {
				o << "->";
			}
			o << value;
			firstOutput = false;
		}
		o << "]" << std::endl;
	}

	void dump2(std::ostream &o = std::cerr) const {
		for (size_type i = 0; i < max_sz; i++) {
			o << i << ": ";
			auto cur_element = table[i].first_element;
			while (cur_element != nullptr) {
				o << cur_element->key << " ";
				cur_element = cur_element->next_element;
			}
			o << std::endl;
		}
	}

	





	friend bool operator==(const ADS_set &lhs, const ADS_set &rhs) {
		if (lhs.sz != rhs.sz) {
			return false;
		}

		auto it = lhs.begin();
		while (it != lhs.end()) {
			const auto it2 = rhs.find(*it);
			if (it2 == rhs.end()) {
				return false;
			}

			it++;
		}

		return true;
	}

	friend bool operator!=(const ADS_set &lhs, const ADS_set &rhs) {
		return !(lhs == rhs);
	}
};

template <typename Key, size_t N>
class ADS_set<Key, N>::Iterator {
public:
	using value_type = Key;
	using difference_type = std::ptrdiff_t;
	using reference = const value_type &;
	using pointer = const value_type *;
	using iterator_category = std::forward_iterator_tag;

	ADS_set<Key, N>* adsSet_;
	ADS_set<Key, N>::size_type linkedListIndex_;
	ADS_set<Key, N>::element* element_;

	explicit Iterator()
	{
		adsSet_ = nullptr;
		linkedListIndex_ = std::numeric_limits<size_type>::max();
		element_ = nullptr;
	}

	//Iterator constructor
	explicit Iterator(ADS_set<Key, N>* adsSet, size_type linkedListIndex, element* elementPtr)
		: adsSet_(adsSet)
		, linkedListIndex_(linkedListIndex)
		, element_(elementPtr)
	{
	}

	reference operator*() const {
		return element_->key;
	}

	pointer operator->() const {
		return &element_->key;
	}

	Iterator &operator++() // prefix
	{
		if (adsSet_ == nullptr || element_ == nullptr) {
			return *this;
		}

		if (element_->next_element != nullptr) {
			element_ = element_->next_element;
			return *this;
		}

		linkedListIndex_ = adsSet_->findNextNonEmptyLinkedList(linkedListIndex_ + 1);
		if (linkedListIndex_ == std::numeric_limits<size_type>::max()) {
			element_ = nullptr;
			return *this;
		}

		element_ = adsSet_->table[linkedListIndex_].first_element;
		return *this;
	}

	Iterator operator++(int) // postfix
	{
		auto currentIterator = *this;
		++(*this);
		return currentIterator;
	}

	friend bool operator==(const Iterator &lhs, const Iterator &rhs) {
		return lhs.adsSet_ == rhs.adsSet_ && lhs.linkedListIndex_ == rhs.linkedListIndex_ && lhs.element_ == rhs.element_;
	}

	friend bool operator!=(const Iterator &lhs, const Iterator &rhs) {
		return !(lhs == rhs);
	}
};

template <typename Key, size_t N> void swap(ADS_set<Key, N> &lhs, ADS_set<Key, N> &rhs) { lhs.swap(rhs); }

#endif // ADS_SET_H