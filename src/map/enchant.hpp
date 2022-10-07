#ifndef ENCHANT_HPP
#define ENCHANT_HPP

#include "itemdb.hpp"

#include "../common/database.hpp"
#include "../common/mmo.hpp"

#include <vector>
#include <unordered_map>
#include <unordered_set>

struct s_enchant_item_requirement {
	t_itemid nameid;
	int amount;
};

struct s_simple_enchant_entry {
	t_itemid nameid;
	int rate;
};

struct s_perfect_enchant_entry {
	t_itemid nameid;
	int zeny;
	std::vector<s_enchant_item_requirement> items;
};

struct s_upgrade_enchant_entry {
	t_itemid base;
	t_itemid result;
	int zeny;
	std::vector<s_enchant_item_requirement> items;
};

struct s_enchant_slot {
	int slot;
	struct s_enchant_requirement {
		int zeny;
		int chance;
		std::vector<s_enchant_item_requirement> items;
	} requirements;
	int simpleEnchantsTotalLot;
	std::vector<s_simple_enchant_entry> simpleEnchants;
	std::unordered_map<t_itemid, s_perfect_enchant_entry> perfectEnchants;
	std::unordered_map<t_itemid, s_upgrade_enchant_entry> upgradeEnchants;
};

struct s_enchant_table {
	uint32 tableNumber;
	std::unordered_set<t_itemid> items;
	std::vector<int> slotOrder;
	int minGrade;
	int minRefine;
	bool allowRandomOption;
	bool reopenUi;
	struct s_enchant_reset_info {
		bool allow;
		int chance;
		int zeny;
		std::vector<s_enchant_item_requirement> items;
	} reset;
	std::unordered_map<int, s_enchant_slot> slots;
};

class EnchantDatabase : public TypesafeCachedYamlDatabase<uint32, s_enchant_table> {
public:
	EnchantDatabase() : TypesafeCachedYamlDatabase("ENCHANT_DB", 1) {

	}
	const std::string getDefaultLocation();
	uint64 parseBodyNode(const ryml::NodeRef& node) override;
	void loadingFinished();

private:
	void parseRequirements(std::vector<s_enchant_item_requirement>& output, const ryml::NodeRef& node);
};
extern EnchantDatabase enchant_db;

void enchant_apply_simple(map_session_data* sd, uint32 tableNumber, uint16 index);
void enchant_apply_perfect(map_session_data* sd, uint32 tableNumber, uint16 index, t_itemid selectedEnchantId);
void enchant_apply_upgrade(map_session_data* sd, uint32 tableNumber, uint16 index, int slot);
void enchant_reset(map_session_data* sd, uint32 tableNumber, uint16 index);

void do_init_enchant();
void do_final_enchant();

#endif // !ENCHANT_HPP