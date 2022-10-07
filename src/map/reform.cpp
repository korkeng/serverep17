#include "reform.hpp"

#include "log.hpp"
#include "pc.hpp"
#include "../common/utils.hpp"

ItemReformDatabase item_reform_db;

const std::string ItemReformDatabase::getDefaultLocation() {
	return std::string(db_path) + "/item_reform.yml";
}

uint64 ItemReformDatabase::parseBodyNode(const ryml::NodeRef& node) {
	t_itemid initiatorId{};

	if (this->nodeExists(node, "Initiator")) {
		std::string aegisName;
		this->asString(node, "Initiator", aegisName);
		auto data = item_db.search_aegisname(aegisName.c_str());

		if (data == nullptr) {
			this->invalidWarning(node["Initiator"], "Could not find item from provided Aegis name.\n");
			return 0;
		}
		initiatorId = data->nameid;
	}
	else {
		this->invalidWarning(node, "Item reform data node does not have Initiator Aegis name.\n");
		return 0;
	}

	std::shared_ptr<s_item_reform_initiator_data> initiatorData = this->find(initiatorId);

	if (initiatorData == nullptr) {
		if (!this->nodeExists(node, "Reforms")) {
			return 0;
		}

		initiatorData = std::make_shared<s_item_reform_initiator_data>();
		initiatorData->nameid = initiatorId;
	}


	if (this->nodeExists(node, "Reforms")) {
		const auto& reformsNode = node["Reforms"];
		for (const auto& reformNode : reformsNode.children()) {
			std::shared_ptr<s_item_reform_entry> entry;
			if (this->nodeExists(reformNode, "Clear")) {
				bool actuallyClear = false;
				this->asBool(reformNode, "Clear", actuallyClear);
				if (actuallyClear) {
					initiatorData->clearEntries();
				}
			}
			if (this->nodeExists(reformNode, "BaseItem")) {
				std::string aegisName;
				this->asString(reformNode, "BaseItem", aegisName);
				auto data = item_db.search_aegisname(aegisName.c_str());

				if (data == nullptr) {
					this->invalidWarning(node["BaseItem"], "Could not find item from provided Aegis name.\n");
					return 0;
				}
				entry = initiatorData->getReformEntryFromItemId(data->nameid);
				if (entry == nullptr) {
					entry = std::make_shared<s_item_reform_entry>();
				}
				entry->baseItemId = data->nameid;
			}
			else if (this->nodeExists(reformNode, "BaseItemGroup")) {
				std::string groupName;
				uint16 groupId;
				int64 tmp;
				this->asString(reformNode, "BaseItemGroup", groupName);
				auto exists = script_get_constant(groupName.c_str(), &tmp);
				groupId = (uint16)tmp;

				if (!exists) {
					this->invalidWarning(reformNode["BaseItemGroup"], "Could not find item group ID from given constant.\n");
					return 0;
				}
				if (!itemdb_group.exists(groupId)) {
					this->invalidWarning(reformNode["BaseItemGroup"], "Item group ID is resolveable from constant but item group does not exist.\n");
					return 0;
				}
				entry = initiatorData->getReformEntryFromItemGroupId(groupId);
				if (entry == nullptr) {
					entry = std::make_shared<s_item_reform_entry>();
				}
				entry->baseGroupId = groupId;
				entry->baseGroupData = itemdb_group.find(groupId);
				entry->isGroup = true;
			}
			else {
				this->invalidWarning(reformNode, "Item reform entry does not have base item nor base item group.\n");
				return 0;
			}
			if (this->nodeExists(reformNode, "PreserveItem")) {
				this->asBool(reformNode, "PreserveItem", entry->preserveItem);
			}
			else if (this->nodeExists(reformNode, "ResultItem")) {
				std::string aegisName;
				this->asString(reformNode, "ResultItem", aegisName);
				auto resultItemData = item_db.search_aegisname(aegisName.c_str());
				if (resultItemData == nullptr) {
					this->invalidWarning(reformNode["ResultItem"], "Could not find result item %s.\n", aegisName.c_str());
					return 0;
				}
				entry->resultItemId = resultItemData->nameid;
			}
			else if (!entry->preserveItem && entry->resultItemId == 0) {
				this->invalidWarning(reformNode, "Reform entry node does not contain either PreserveItem or ResultItem, skipping.\n");
				return 0;
			}
			if (this->nodeExists(reformNode, "NeedRefineMin")) {
				this->asInt32(reformNode, "NeedRefineMin", entry->needRefineMin);
			}
			if (this->nodeExists(reformNode, "NeedRefineMax")) {
				this->asInt32(reformNode, "NeedRefineMax", entry->needRefineMax);
			}
			if (this->nodeExists(reformNode, "NeedOptionNumMin")) {
				this->asInt32(reformNode, "NeedOptionNumMin", entry->needOptionNumMin);
			}
			if (this->nodeExists(reformNode, "IsEmptySocket")) {
				this->asBool(reformNode, "IsEmptySocket", entry->requireEmptySocket);
			}
			if (this->nodeExists(reformNode, "ChangeRefineValue")) {
				this->asInt32(reformNode, "ChangeRefineValue", entry->changeRefineValue);
			}
			if (this->nodeExists(reformNode, "RandomOptionGroup") && !reformNode["RandomOptionGroup"].val_is_null()) {
				std::string randomOptionGroupName;
				uint16 randomOptGroupId;
				this->asString(reformNode, "RandomOptionGroup", randomOptionGroupName);
				if (!random_option_group.option_get_id(randomOptionGroupName, randomOptGroupId)) {
					this->invalidWarning(reformNode, "Reform node contains nonexistent random option group, skipping.\n");
					return 0;
				}
				auto randomOptGroup = random_option_group.find(randomOptGroupId);
				entry->randomOptGroup = randomOptGroup;
			}
			if (this->nodeExists(reformNode, "PreserveSocketItem")) {
				this->asBool(reformNode, "PreserveSocketItem", entry->preserveSockets);
			}
			if (this->nodeExists(reformNode, "PreserveGrade")) {
				this->asBool(reformNode, "PreserveGrade", entry->preserveGrade);
			}
			if (this->nodeExists(reformNode, "Materials")) {
				const auto& materialsNode = reformNode["Materials"];
				for (const auto& materialNode : materialsNode.children()) {
					if (this->nodeExists(materialNode, "Clear")) {
						bool actuallyClear = false;
						this->asBool(materialNode, "Clear", actuallyClear);
						if (actuallyClear) {
							entry->materials.clear();
						}
					}
					if (!this->nodeExists(materialNode, "AegisName") || !this->nodeExists(materialNode, "Amount")) {
						this->invalidWarning(materialNode, "Reform material node requires AegisName and Amount.\n");
						continue;
					}
					std::string aegisName;
					int amount = 0;
					this->asString(materialNode, "AegisName", aegisName);
					this->asInt32(materialNode, "Amount", amount);
					auto material_data = item_db.search_aegisname(aegisName.c_str());
					if (material_data == nullptr) {
						this->invalidWarning(materialNode, "Material node contains invalid Aegis name %s.\n", aegisName.c_str());
						continue;
					}
					entry->materials[material_data->nameid] = amount;
				}
			}
			if (entry->isGroup) {
				initiatorData->groupEntries[entry->baseGroupId] = entry;
			}
			else {
				initiatorData->itemEntries[entry->baseItemId] = entry;
			}
		}
		this->put(initiatorId, initiatorData);
		return 1;
	}
	else {
		this->invalidWarning(node, "Item reform data node does not have any reform data.\n");
		return 0;
	}
}

void ItemReformDatabase::loadingFinished() {

}

std::shared_ptr<s_item_reform_entry> s_item_reform_initiator_data::getReformEntryFromItemId(t_itemid nameid) {
	if (this->itemEntries.find(nameid) == this->itemEntries.end()) {
		return nullptr;
	}

	return this->itemEntries[nameid];
}

std::shared_ptr<s_item_reform_entry> s_item_reform_initiator_data::getReformEntryFromItemGroupId(uint16 group_id) {
	if (this->groupEntries.find(group_id) == this->groupEntries.end()) {
		return nullptr;
	}

	return this->groupEntries[group_id];
}

std::shared_ptr<s_item_reform_entry> s_item_reform_initiator_data::getFirstPossibleReformForId(t_itemid nameid) {
	// First we look up in our item id lookup table
	auto entry = this->getReformEntryFromItemId(nameid);
	if (entry != nullptr) {
		return entry;
	}

	// Look in every group
	for (const auto& group : this->groupEntries) {
		if (itemdb_group.item_exists(group.first, nameid)) {
			return group.second;
		}
	}

	// We exhausted our options
	return nullptr;
}

void reform_do_reform(map_session_data* sd, t_itemid initiatorId, uint16 index) {
	// Check if the index is valid
	if (index >= MAX_INVENTORY) {
		return;
	}

	// Get the item db reference
	struct item_data* id = sd->inventory_data[index];

	// No item data was found
	if (id == NULL) {
		return;
	}

	// Check the inventory
	struct item* item = &sd->inventory.u.items_inventory[index];

	// No item was found at the given index
	if (item == NULL) {
		return;
	}

	// Check if the item is identified
	if (!item->identify) {
		return;
	}

	// Check if the item is broken
	if (item->attribute) {
		return;
	}

	// Check if initiator is valid and retrieve its data
	auto initiatorData = item_reform_db.find(initiatorId);
	if (initiatorData == nullptr) {
		return;
	}
	// Check if item at specified index can be reformed
	auto reformEntry = initiatorData->getFirstPossibleReformForId(id->nameid);
	if (reformEntry == nullptr) {
		return;
	}

	// Refine condition check
	if (item->refine < reformEntry->needRefineMin || item->refine > reformEntry->needRefineMax) {
		return;
	}

	// Random option count condition check
	if (reformEntry->needOptionNumMin > 0) {
		int i = 0;
		ARR_FIND(0, MAX_ITEM_RDM_OPT, i, item->option[i].id == 0);
		if (i < reformEntry->needOptionNumMin) {
			return;
		}
	}

	// Empty socket condition check
	if (reformEntry->requireEmptySocket) {
		for (int i = 0; i < MAX_SLOTS; ++i) {
			if (item->card[i] != 0) {
				return;
			}
		}
	}

	// Material check
	std::unordered_map<short, int> materialsToDelete;
	for (const auto& material : reformEntry->materials) {
		// Check if the player has material
		short materialIndex;
		if ((materialIndex = pc_search_inventory(sd, material.first)) < 0) {
			return;
		}

		// Check if the player has enough materials
		if (material.second > 0 && sd->inventory.u.items_inventory[materialIndex].amount < material.second) {
			return;
		}
		materialsToDelete[materialIndex] = material.second;
	}

	// Delete materials
	for (const auto& material : materialsToDelete) {
		if (material.second > 0 && pc_delitem(sd, material.first, material.second, 0, 0, LOG_TYPE_CONSUME)) {
			return;
		}
	}

	log_pick_pc(sd, LOG_TYPE_OTHER, -1, item);
	// Change item if preserve item flag is not set.
	if (!reformEntry->preserveItem) {
		item->nameid = reformEntry->resultItemId;
		sd->inventory_data[index] = itemdb_search(reformEntry->resultItemId);
	}
	// Change refine.
	item->refine = cap_value(item->refine + reformEntry->changeRefineValue, 0, MAX_REFINE);
	// Change grade.
	if (!reformEntry->preserveGrade) {
		item->enchantgrade = 0;
	}
	// Reset sockets, if preserve flag is not set.
	if (!reformEntry->preserveSockets) {
		for (int i = 0; i < MAX_SLOTS; ++i) {
			item->card[i] = 0;
		}
	}
	// Apply random option, if random option group is specified
	if (!reformEntry->randomOptGroup.expired()) {
		// TODO: Apply random option
	}

	clif_delitem(sd, index, 1, 3);
	clif_additem(sd, index, 1, 0);
	clif_reform_ack(sd, true);
	clif_preview_item(sd, index);
	log_pick_pc(sd, LOG_TYPE_OTHER, 1, item);
}

void do_init_reform() {
}

void do_final_reform() {
}