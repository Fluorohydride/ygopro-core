#ifndef SORT_H_
#define SORT_H_

class card;

struct card_sort {
	bool operator()(card* const& c1, card* const& c2) const;
};

#endif /* SORT_H_ */
