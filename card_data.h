#ifndef CARD_DATA_H_
#define CARD_DATA_H_

struct card_data {
	uint32 code{ 0 };
	uint32 alias{ 0 };
	uint64 setcode{ 0 };
	uint32 type{ 0 };
	uint32 level{ 0 };
	uint32 attribute{ 0 };
	uint32 race{ 0 };
	int32 attack{ 0 };
	int32 defense{ 0 };
	uint32 lscale{ 0 };
	uint32 rscale{ 0 };
	uint32 link_marker{ 0 };

	void clear() {
		code = 0;
		alias = 0;
		setcode = 0;
		type = 0;
		level = 0;
		attribute = 0;
		race = 0;
		attack = 0;
		defense = 0;
		lscale = 0;
		rscale = 0;
		link_marker = 0;
	}
};

#endif /* CARD_DATA_H_ */
