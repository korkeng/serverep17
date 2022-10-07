#include "enchant.hpp"

#include "clif.hpp"
#include "itemdb.hpp"
#include "log.hpp"
#include "pc.hpp"

#include "../common/random.hpp"

EnchantDatabase enchant_db;

const std::string EnchantDatabase::getDefaultLocation() {
	return std::string(db_path) + "/enchant.yml";
}

void EnchantDatabase::parseRequirements(std::vector<s_enchant_item_requirement>& output, const ryml::NodeRef& node) {
	for (const auto& requirement : node.children()) {
		if (this->nodeExists(requirement, "Clear")) {
			bool actuallyClear;
			this->asBool(requirement, "Clear", actuallyClear);
			if (actuallyClear) {
				output.clear();
			}
		}
		if (!this->nodeExists(requirement, "Name") || !this->nodeExists(requirement, "Amount")) {
			this->invalidWarning(requirement, "Enchant requirement node does not have both Name and Amount property.\n");
			continue;
		}
		std::string aegisName;
		int amount;
		this->asString(requirement, "Name", aegisName);
		this->asInt32(requirement, "Amount", amount);
		auto id = item_db.search_aegisname(aegisName.c_str());
		if (id == nullptr) {
			this->invalidWarning(requirement["Name"], "Unknown Aegis name %s for enchant requirement.\n", aegisName.c_str());
			continue;
		}
		s_enchant_item_requirement s{};
		s.nameid = id->nameid;
		s.amount = amount;
		output.push_back(s);
	}
}

uint64 EnchantDatabase::parseBodyNode(const ryml::NodeRef& node) {
	uint32 tableNumber;

	if (this->nodeExists(node, "TableNumber")) {
		this->asUInt32(node, "TableNumber", tableNumber);
	}
	else {
		this->invalidWarning(node, "Enchant table node does not have TableNumber property.\n");
		return 0;
	}

	auto table = this->find(tableNumber);
	if (table == nullptr) {
		table = std::make_shared<s_enchant_table>();
		table->tableNumber = tableNumber;
	}

	if (this->nodeExists(node, "Items")) {
		const auto& itemsNode = node["Items"];
		for (const auto& itemNode : itemsNode.children()) {
			std::string aegisName;
			itemNode >> aegisName;
			auto id = item_db.search_aegisname(aegisName.c_str());
			if (id == nullptr) {
				this->invalidWarning(itemNode, "Unknown Aegis name for enchant item.\n");
				continue;
			}
			table->items.insert(id->nameid);
		}
	}
	if (this->nodeExists(node, "SlotOrder")) {
		table->slotOrder.clear();
		const auto& slotsNode = node["SlotOrder"];
		for (const auto& slotNode : slotsNode.children()) {
			int slot;
			slotNode >> slot;
			table->slotOrder.push_back(slot);
		}
	}
	if (table->slotOrder.size() == 0) {
		this->invalidWarning(node["SlotOrder"], "Enchant table must have slot order defined.\n");
		return 0;
	}
	if (this->nodeExists(node, "MinGrade")) {
		this->asInt32(node, "MinGrade", table->minGrade);
	}
	if (this->nodeExists(node, "MinRefine")) {
		this->asInt32(node, "MinRefine", table->minRefine);
	}
	if (this->nodeExists(node, "AllowRandomOption")) {
		this->asBool(node, "AllowRandomOption", table->allowRandomOption);
	}
	if (this->nodeExists(node, "Reset")) {
		const auto& resetNode = node["Reset"];
		if (this->nodeExists(resetNode, "Allow")) {
			this->asBool(resetNode, "Allow", table->reset.allow);
		}
		if (this->nodeExists(resetNode, "Chance")) {
			this->asInt32(resetNode, "Chance", table->reset.chance);
		}
		if (this->nodeExists(resetNode, "Zeny")) {
			this->asInt32(resetNode, "Zeny", table->reset.zeny);
		}
		if (this->nodeExists(resetNode, "ItemRequirements")) {
			this->parseRequirements(table->reset.items, resetNode["ItemRequirements"]);
		}
	}
	if (this->nodeExists(node, "ReopenUI")) {
		this->asBool(node, "ReopenUI", table->reopenUi);
	}
	if (this->nodeExists(node, "Slots")) {
		const auto& slotsNode = node["Slots"];
		for (const auto& it : slotsNode.children()) {
			const auto& key = it.key();
			int slot;
			c4::atoi(key, &slot);
			const auto& slotNode = it;
			s_enchant_slot slotData{};
			if (table->slots.find(slot) != table->slots.end()) {
				slotData = table->slots[slot];
			}
			slotData.slot = slot;
			if (this->nodeExists(slotNode, "ItemRequirements")) {
				this->parseRequirements(slotData.requirements.items, slotNode["ItemRequirements"]);
			}
			if (this->nodeExists(slotNode, "Zeny")) {
				this->asInt32(slotNode, "Zeny", slotData.requirements.zeny);
			}
			if (this->nodeExists(slotNode, "Chance")) {
				this->asInt32(slotNode, "Chance", slotData.requirements.chance);
			}
			if (this->nodeExists(slotNode, "Enchants")) {
				const auto& enchantsNode = slotNode["Enchants"];
				for (const auto& enchantNode : enchantsNode.children()) {
					if (this->nodeExists(enchantNode, "Clear")) {
						bool actuallyClear;
						this->asBool(enchantNode, "Clear", actuallyClear);
						if (actuallyClear) {
							slotData.simpleEnchants.clear();
						}
					}

					s_simple_enchant_entry s{};
					if (!this->nodeExists(enchantNode, "Name") || !this->nodeExists(enchantNode, "Rate")) {
						this->invalidWarning(enchantNode, "Enchant node does not have both Name and Rate property.\n");
						continue;
					}
					std::string aegisName;
					this->asString(enchantNode, "Name", aegisName);
					auto id = item_db.search_aegisname(aegisName.c_str());
					if (id == nullptr) {
						this->invalidWarning(enchantNode["Name"], "Unknown Aegis name %s for enchant name.\n", aegisName.c_str());
						continue;
					}
					s.nameid = id->nameid;
					this->asInt32(enchantNode, "Rate", s.rate);
					slotData.simpleEnchantsTotalLot += s.rate;
					slotData.simpleEnchants.push_back(s);
				}
			}
			if (this->nodeExists(slotNode, "PerfectEnchants")) {
				const auto& enchantsNode = slotNode["PerfectEnchants"];
				for (const auto& enchantNode : enchantsNode.children()) {
					if (this->nodeExists(enchantNode, "Clear")) {
						bool actuallyClear;
						this->asBool(enchantNode, "Clear", actuallyClear);
						if (actuallyClear) {
							slotData.perfectEnchants.clear();
						}
					}

					s_perfect_enchant_entry s{};
					if (!this->nodeExists(enchantNode, "Name") || !this->nodeExists(enchantNode, "Zeny")) {
						this->invalidWarning(enchantNode, "Perfect enchant node does not have both Name and Zeny property.\n");
						continue;
					}
					std::string aegisName;
					this->asString(enchantNode, "Name", aegisName);
					auto id = item_db.search_aegisname(aegisName.c_str());
					if (id == nullptr) {
						this->invalidWarning(enchantNode["Name"], "Unknown Aegis name %s for enchant name.\n", aegisName.c_str());
						continue;
					}
					s.nameid = id->nameid;
					this->asInt32(enchantNode, "Zeny", s.zeny);
					this->parseRequirements(s.items, enchantNode["ItemRequirements"]);
					slotData.perfectEnchants[s.nameid] = s;
				}
			}
			if (this->nodeExists(slotNode, "UpgradeEnchants")) {
				const auto& enchantsNode = slotNode["UpgradeEnchants"];
				for (const auto& it : enchantsNode.children()) {
					const auto& enchantNode = it;
					if (this->nodeExists(enchantNode, "Clear")) {
						bool actuallyClear;
						this->asBool(enchantNode, "Clear", actuallyClear);
						if (actuallyClear) {
							slotData.perfectEnchants.clear();
						}
					}

					s_upgrade_enchant_entry s{};
					std::string aegisName;
					c4::from_chars(it.key(), &aegisName);
					auto id = item_db.search_aegisname(aegisName.c_str());
					if (id == nullptr) {
						this->invalidWarning(it, "Unknown Aegis name %s for base enchant name.\n", aegisName.c_str());
						continue;
					}

					if (this->nodeExists(enchantNode, "ResultEnchant")) {
						this->asString(enchantNode, "ResultEnchant", aegisName);
					}
					else {
						this->invalidWarning(enchantNode, "Upgrade enchant must have result enchant.\n", aegisName.c_str());
						continue;
					}
					auto result_id = item_db.search_aegisname(aegisName.c_str());
					if (result_id == nullptr) {
						this->invalidWarning(enchantNode["ResultEnchant"], "Unknown Aegis name %s for result enchant name.\n", aegisName.c_str());
						continue;
					}
					if (this->nodeExists(enchantNode, "Zeny")) {
						this->asInt32(enchantNode, "Zeny", s.zeny);
					}
					if (this->nodeExists(enchantNode, "ItemRequirements")) {
						this->parseRequirements(s.items, enchantNode["ItemRequirements"]);
					}
					s.base = id->nameid;
					s.result = result_id->nameid;
					slotData.upgradeEnchants[s.base] = s;
				}
			}
			table->slots[slot] = slotData;
		}
	}

	this->put(tableNumber, table);
	return 1;
}

void EnchantDatabase::loadingFinished() {

}

bool enchant_validate_index(map_session_data* sd, uint16 index)
{
	// Check if the index is valid
	if (index >= MAX_INVENTORY) {
		return false;
	}

	// Get the item db reference
	struct item_data* id = sd->inventory_data[index];

	// No item data was found
	if (id == NULL) {
		return false;
	}

	// Check the inventory
	struct item* item = &sd->inventory.u.items_inventory[index];

	// No item was found at the given index
	if (item == NULL) {
		return false;
	}

	// Check if the item is identified
	if (!item->identify) {
		return false;
	}

	// Check if the item is broken
	if (item->attribute) {
		return false;
	}

	return true;
}

void enchant_apply_simple(map_session_data* sd, uint32 tableNumber, uint16 index) {
	if (!enchant_validate_index(sd, index)) {
		return;
	}

	struct item_data* id = sd->inventory_data[index];
	struct item* item = &sd->inventory.u.items_inventory[index];
	auto enchantTable = enchant_db.find(tableNumber);

	int slot = -1;
	// Find applicable slot
	for (const auto& _slot : enchantTable->slotOrder) {
		if (item->card[_slot] == 0) {
			slot = _slot;
			break;
		}
	}
	if (slot == -1) {
		return; // No applicable slot
	}

	if (enchantTable->slots.find(slot) == enchantTable->slots.end()) {
		return; // No slot data found
	}

	const auto& slotData = enchantTable->slots[slot];

	// Try to delete the materials
	std::unordered_map<short, int> materialsToDelete;
	for (const auto& material : slotData.requirements.items) {
		short materialIndex;
		if ((materialIndex = pc_search_inventory(sd, material.nameid)) < 0) {
			return;
		}
		if (material.amount > 0 && sd->inventory.u.items_inventory[materialIndex].amount < material.amount) {
			return;
		}
		materialsToDelete[materialIndex] = material.amount;
	}
	for (const auto& material : materialsToDelete) {
		if (material.second > 0 && pc_delitem(sd, material.first, material.second, 0, 0, LOG_TYPE_CONSUME)) {
			return;
		}
	}

	// Try to pay the zeny
	if (pc_payzeny(sd, slotData.requirements.zeny, LOG_TYPE_CONSUME, NULL)) {
		clif_npc_buy_result(sd, e_purchase_result::PURCHASE_FAIL_MONEY); // "You do not have enough zeny."
		return;
	}

	// Roll the base chance
	if ((rnd() % 100000) >= slotData.requirements.chance) {
		clif_enchant_ack(sd, false, 0); // Enchant failed ACK
		return;
	}

	// Roll the actual enchantment
	int roll = rnd() % slotData.simpleEnchantsTotalLot;
	int carry = 0;
	t_itemid resultEnchantId = 0;
	for (const auto& enchant : slotData.simpleEnchants) {
		if (roll < enchant.rate + carry) {
			resultEnchantId = enchant.nameid;
			break;
		}
		else {
			carry += enchant.rate;
		}
	}
	if (resultEnchantId == 0) {
		clif_displaymessage(sd->fd, "Enchantment UI did not yield any enchantment. Please report this.");
		return;
	}

	log_pick_pc(sd, LOG_TYPE_OTHER, -1, item);
	item->card[slot] = resultEnchantId;
	clif_delitem(sd, index, 1, 3);
	clif_additem(sd, index, 1, 0);
	clif_enchant_ack(sd, true, resultEnchantId);
	log_pick_pc(sd, LOG_TYPE_OTHER, 1, item);
	if (enchantTable->reopenUi) {
		clif_open_enchant_ui(sd, tableNumber);
	}
}

void enchant_apply_perfect(map_session_data* sd, uint32 tableNumber, uint16 index, t_itemid selectedEnchantId) {
	if (!enchant_validate_index(sd, index)) {
		return;
	}

	struct item_data* id = sd->inventory_data[index];
	struct item* item = &sd->inventory.u.items_inventory[index];
	auto enchantTable = enchant_db.find(tableNumber);

	int slot = -1;
	// Check for slot eligibility
	for (int i = 0; i < enchantTable->slotOrder.size(); ++i) {
		if (item->card[enchantTable->slotOrder[i]] == 0) {
			slot = enchantTable->slotOrder[i]; // Slot is empty.
			break;
		}
	}
	if (slot == -1) {
		return; // No applicable slot
	}

	if (enchantTable->slots.find(slot) == enchantTable->slots.end()) {
		return; // No slot data found
	}

	const auto& slotData = enchantTable->slots[slot];

	// Try to find the requested enchant
	if (slotData.perfectEnchants.find(selectedEnchantId) == slotData.perfectEnchants.end()) {
		return;
	}

	const auto& perfectEnchant = slotData.perfectEnchants.find(selectedEnchantId);

	// Try to delete the materials
	std::unordered_map<short, int> materialsToDelete;
	for (const auto& material : perfectEnchant->second.items) {
		short materialIndex;
		if ((materialIndex = pc_search_inventory(sd, material.nameid)) < 0) {
			return;
		}
		if (material.amount > 0 && sd->inventory.u.items_inventory[materialIndex].amount < material.amount) {
			return;
		}
		materialsToDelete[materialIndex] = material.amount;
	}
	for (const auto& material : materialsToDelete) {
		if (material.second > 0 && pc_delitem(sd, material.first, material.second, 0, 0, LOG_TYPE_CONSUME)) {
			return;
		}
	}

	// Try to pay the zeny
	if (pc_payzeny(sd, perfectEnchant->second.zeny, LOG_TYPE_CONSUME, NULL)) {
		clif_npc_buy_result(sd, e_purchase_result::PURCHASE_FAIL_MONEY); // "You do not have enough zeny."
		return;
	}

	log_pick_pc(sd, LOG_TYPE_OTHER, -1, item);
	item->card[slot] = perfectEnchant->second.nameid;
	clif_delitem(sd, index, 1, 3);
	clif_additem(sd, index, 1, 0);
	clif_enchant_ack(sd, true, perfectEnchant->second.nameid);
	log_pick_pc(sd, LOG_TYPE_OTHER, 1, item);
	if (enchantTable->reopenUi) {
		clif_open_enchant_ui(sd, tableNumber);
	}
}

void enchant_apply_upgrade(map_session_data* sd, uint32 tableNumber, uint16 index, int slot) {
	if (!enchant_validate_index(sd, index)) {
		return;
	}

	struct item_data* id = sd->inventory_data[index];
	struct item* item = &sd->inventory.u.items_inventory[index];
	auto enchantTable = enchant_db.find(tableNumber);

	if (enchantTable->slots.find(slot) == enchantTable->slots.end()) {
		return; // No slot data found
	}

	const auto& slotData = enchantTable->slots[slot];

	if (item->card[slot] == 0 || slotData.upgradeEnchants.find(item->card[slot]) == slotData.upgradeEnchants.end()) {
		return;
	}

	const auto& upgrade = slotData.upgradeEnchants.find(item->card[slot]);

	// Try to delete the materials
	std::unordered_map<short, int> materialsToDelete;
	for (const auto& material : upgrade->second.items) {
		short materialIndex;
		if ((materialIndex = pc_search_inventory(sd, material.nameid)) < 0) {
			return;
		}
		if (material.amount > 0 && sd->inventory.u.items_inventory[materialIndex].amount < material.amount) {
			return;
		}
		materialsToDelete[materialIndex] = material.amount;
	}
	for (const auto& material : materialsToDelete) {
		if (material.second > 0 && pc_delitem(sd, material.first, material.second, 0, 0, LOG_TYPE_CONSUME)) {
			return;
		}
	}

	// Try to pay the zeny
	if (pc_payzeny(sd, upgrade->second.zeny, LOG_TYPE_CONSUME, NULL)) {
		clif_npc_buy_result(sd, e_purchase_result::PURCHASE_FAIL_MONEY); // "You do not have enough zeny."
		return;
	}

	log_pick_pc(sd, LOG_TYPE_OTHER, -1, item);
	item->card[slot] = upgrade->second.result;
	clif_delitem(sd, index, 1, 3);
	clif_additem(sd, index, 1, 0);
	clif_enchant_ack(sd, true, upgrade->second.result);
	log_pick_pc(sd, LOG_TYPE_OTHER, 1, item);
	if (enchantTable->reopenUi) {
		clif_open_enchant_ui(sd, tableNumber);
	}
}

void enchant_reset(map_session_data* sd, uint32 tableNumber, uint16 index) {
	if (!enchant_validate_index(sd, index)) {
		return;
	}

	struct item_data* id = sd->inventory_data[index];
	struct item* item = &sd->inventory.u.items_inventory[index];
	auto enchantTable = enchant_db.find(tableNumber);

	// Reset not allowed for this table
	if (!enchantTable->reset.allow) {
		return;
	}

	// Check if there is any enchant to reset
	bool hasEnchant = false;
	for (const auto& _slot : enchantTable->slotOrder) {
		if (item->card[_slot] != 0) {
			hasEnchant = true;
			break;
		}
	}
	if (!hasEnchant) {
		return;
	}

	// Try to delete the materials
	std::unordered_map<short, int> materialsToDelete;
	for (const auto& material : enchantTable->reset.items) {
		short materialIndex;
		if ((materialIndex = pc_search_inventory(sd, material.nameid)) < 0) {
			return;
		}
		if (material.amount > 0 && sd->inventory.u.items_inventory[materialIndex].amount < material.amount) {
			return;
		}
		materialsToDelete[materialIndex] = material.amount;
	}
	for (const auto& material : materialsToDelete) {
		if (material.second > 0 && pc_delitem(sd, material.first, material.second, 0, 0, LOG_TYPE_CONSUME)) {
			return;
		}
	}

	// Try to pay the zeny
	if (pc_payzeny(sd, enchantTable->reset.zeny, LOG_TYPE_CONSUME, NULL)) {
		clif_npc_buy_result(sd, e_purchase_result::PURCHASE_FAIL_MONEY); // "You do not have enough zeny."
		return;
	}

	// Roll the base chance
	if ((rnd() % 100000) >= enchantTable->reset.chance) {
		clif_enchant_ack(sd, false, 0); // Enchant failed ACK
		return;
	}

	log_pick_pc(sd, LOG_TYPE_OTHER, -1, item);
	for (const auto& slot : enchantTable->slotOrder) {
		item->card[slot] = 0;
	}
	clif_delitem(sd, index, 1, 3);
	clif_additem(sd, index, 1, 0);
	clif_enchant_ack(sd, true, 0);
	log_pick_pc(sd, LOG_TYPE_OTHER, 1, item);
	if (enchantTable->reopenUi) {
		clif_open_enchant_ui(sd, tableNumber);
	}
}

void do_init_enchant() {
}

void do_final_enchant() {
}