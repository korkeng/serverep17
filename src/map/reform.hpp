#ifndef REFORM_HPP
#define REFORM_HPP

#include "itemdb.hpp"

#include <unordered_map>
#include <vector>

struct s_item_reform_entry {
	t_itemid baseItemId;
	uint16 baseGroupId;
	bool isGroup;
	std::shared_ptr<s_item_group_db> baseGroupData;
	t_itemid resultItemId;
	int needRefineMin;
	int needRefineMax;
	int needOptionNumMin;
	bool requireEmptySocket;
	int changeRefineValue;
	std::weak_ptr<s_random_opt_group> randomOptGroup;
	bool preserveSockets;
	bool preserveGrade;
	bool preserveItem;
	std::unordered_map<t_itemid, int> materials;

	~s_item_reform_entry() {

	}
};

struct s_item_reform_initiator_data {
	t_itemid nameid;
	std::unordered_map<t_itemid, std::shared_ptr<s_item_reform_entry>> itemEntries;
	std::unordered_map<uint16, std::shared_ptr<s_item_reform_entry>> groupEntries;

	std::shared_ptr<s_item_reform_entry> getReformEntryFromItemId(t_itemid nameid);
	std::shared_ptr<s_item_reform_entry> getReformEntryFromItemGroupId(uint16 group_id);
	std::shared_ptr<s_item_reform_entry> getFirstPossibleReformForId(t_itemid nameid);

	void clearEntries() {
		itemEntries.clear();
		groupEntries.clear();
	}
	~s_item_reform_initiator_data() {
		clearEntries();
	}
};

class ItemReformDatabase : public TypesafeCachedYamlDatabase<t_itemid, s_item_reform_initiator_data> {
public:
	ItemReformDatabase() : TypesafeCachedYamlDatabase("ITEM_REFORM_DB", 1) {

	}

	const std::string getDefaultLocation();
	uint64 parseBodyNode(const ryml::NodeRef& node) override;
	void loadingFinished();
};

extern ItemReformDatabase item_reform_db;

void reform_do_reform(map_session_data* sd, t_itemid initiatorId, uint16 index);

void do_init_reform();
void do_final_reform();

#endif // !REFORM_HPP