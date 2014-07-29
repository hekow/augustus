#include "Resource.h"

#include "Building.h"
#include "Calc.h"
#include "Terrain.h"
#include "Tutorial.h"

#include "Data/Building.h"
#include "Data/CityInfo.h"
#include "Data/Constants.h"
#include "Data/Graphics.h"
#include "Data/Grid.h"
#include "Data/Model.h"
#include "Data/Scenario.h"
#include "Data/Trade.h"

void Resource_calculateWarehouseStocks()
{
	for (int i = 0; i < 16; i++) {
		Data_CityInfo.resourceSpaceInWarehouses[i] = 0;
		Data_CityInfo.resourceStored[i] = 0;
	}
	for (int i = 1; i < MAX_BUILDINGS; i++) {
		if (Data_Buildings[i].inUse == 1 && Data_Buildings[i].type == Building_Warehouse) {
			Data_Buildings[i].hasRoadAccess = 0;
			if (Terrain_hasRoadAccess(Data_Buildings[i].x, Data_Buildings[i].y,
					Data_Buildings[i].size, 0, 0)) {
				Data_Buildings[i].hasRoadAccess = 1;
			} else if (Terrain_hasRoadAccess(Data_Buildings[i].x, Data_Buildings[i].y, 3, 0, 0)) {
				Data_Buildings[i].hasRoadAccess = 2;
			}
		}
	}
	for (int i = 0; i < MAX_BUILDINGS; i++) {
		if (Data_Buildings[i].inUse != 1 || Data_Buildings[i].type != Building_WarehouseSpace) {
			continue;
		}
		int warehouseId = Building_getMainBuildingId(i);
		if (Data_Buildings[warehouseId].hasRoadAccess) {
			Data_Buildings[i].hasRoadAccess = Data_Buildings[warehouseId].hasRoadAccess;
			if (Data_Buildings[i].subtype.warehouseResourceId) {
				int loads = Data_Buildings[i].loadsStored;
				int resource = Data_Buildings[i].subtype.warehouseResourceId;
				Data_CityInfo.resourceStored[resource] += loads;
				Data_CityInfo.resourceSpaceInWarehouses[resource] += 4 - loads;
			} else {
				Data_CityInfo.resourceSpaceInWarehouses[Resource_None] += 4;
			}
		}
	}
}

void Resource_calculateWorkshopStocks()
{
	for (int i = 0; i < 6; i++) {
		Data_CityInfo.resourceWorkshopRawMaterialStored[i] = 0;
		Data_CityInfo.resourceWorkshopRawMaterialSpace[i] = 0;
	}
	for (int i = 1; i < MAX_BUILDINGS; i++) {
		if (Data_Buildings[i].inUse != 1 || !BuildingIsWorkshop(Data_Buildings[i].type)) {
			continue;
		}
		Data_Buildings[i].hasRoadAccess = 0;
		if (Terrain_hasRoadAccess(Data_Buildings[i].x, Data_Buildings[i].y, Data_Buildings[i].size, 0, 0)) {
			Data_Buildings[i].hasRoadAccess = 1;
			int room = 2 - Data_Buildings[i].loadsStored;
			if (room < 0) {
				room = 0;
			}
			int workshopResource = Data_Buildings[i].subtype.workshopResource;
			Data_CityInfo.resourceWorkshopRawMaterialSpace[workshopResource] += room;
			Data_CityInfo.resourceWorkshopRawMaterialStored[workshopResource] += Data_Buildings[i].loadsStored;
		}
	}
}

int Resource_getGraphicIdOffset(int resource, int type)
{
	if (resource == Resource_Meat && Data_Scenario.allowedBuildings.wharf) {
		switch (type) {
			case 0: return 40;
			case 1: return 648;
			case 2: return 8;
			case 3: return 11;
			default: return 0;
		}
	} else {
		return 0;
	}
}

int Resource_getWorkshopWithRoomForRawMaterial(
	int x, int y, int resource, int distanceFromEntry, int roadNetworkId)
{
	if (Data_CityInfo.resourceStockpiled[resource]) {
		return 0;
	}
	int outputType;
	switch (resource) {
		case Resource_Olives: outputType = WorkshopResource_OlivesToOil; break;
		case Resource_Vines: outputType = WorkshopResource_VinesToWine; break;
		case Resource_Iron: outputType = WorkshopResource_IronToWeapons; break;
		case Resource_Timber: outputType = WorkshopResource_TimberToFurniture; break;
		case Resource_Clay: outputType = WorkshopResource_ClayToPottery; break;
		default: return 0;
	}
	int minDist = 10000;
	int minBuildingId = 0;
	for (int i = 1; i < MAX_BUILDINGS; i++) {
		struct Data_Building *b = &Data_Buildings[i];
		if (b->inUse != 1 || !BuildingIsWorkshop(i)) {
			continue;
		}
		if (!b->hasRoadAccess || b->distanceFromEntry <= 0) {
			continue;
		}
		if (b->subtype.workshopResource == outputType && b->roadNetworkId == roadNetworkId && b->loadsStored < 2) {
			int dist = Resource_getDistance(b->x, b->y, x, y, distanceFromEntry, b->distanceFromEntry);
			if (b->loadsStored > 0) {
				dist += 20;
			}
			if (dist < minDist) {
				minDist = dist;
				minBuildingId = i;
			}
		}
	}
	// TODO store b->20x/21y somewhere?
	return minBuildingId;
}

int Resource_getWorkshopForRawMaterial(
	int x, int y, int resource, int distanceFromEntry, int roadNetworkId)
{
	if (Data_CityInfo.resourceStockpiled[resource]) {
		return 0;
	}
	int outputType;
	switch (resource) {
		case Resource_Olives: outputType = WorkshopResource_OlivesToOil; break;
		case Resource_Vines: outputType = WorkshopResource_VinesToWine; break;
		case Resource_Iron: outputType = WorkshopResource_IronToWeapons; break;
		case Resource_Timber: outputType = WorkshopResource_TimberToFurniture; break;
		case Resource_Clay: outputType = WorkshopResource_ClayToPottery; break;
		default: return 0;
	}
	int minDist = 10000;
	int minBuildingId = 0;
	for (int i = 1; i < MAX_BUILDINGS; i++) {
		struct Data_Building *b = &Data_Buildings[i];
		if (b->inUse != 1 || !BuildingIsWorkshop(i)) {
			continue;
		}
		if (!b->hasRoadAccess || b->distanceFromEntry <= 0) {
			continue;
		}
		if (b->subtype.workshopResource == outputType && b->roadNetworkId == roadNetworkId) {
			int dist = 10 * b->loadsStored +
				Resource_getDistance(b->x, b->y, x, y, distanceFromEntry, b->distanceFromEntry);
			if (dist < minDist) {
				minDist = dist;
				minBuildingId = i;
			}
		}
	}
	// TODO store b->20x/21y somewhere?
	return minBuildingId;
}

int Resource_getBarracksForWeapon(int xUnused, int yUnused, int resource, int roadNetworkId)
{
	if (resource != Resource_Weapons) {
		return 0;
	}
	if (Data_CityInfo.resourceStockpiled[Resource_Weapons]) {
		return 0;
	}
	if (Data_CityInfo_Buildings.barracks.working <= 0) {
		return 0;
	}
	struct Data_Building *b = &Data_Buildings[Data_CityInfo.buildingBarracksBuildingId];
	if (b->loadsStored < 5 && Data_CityInfo.militaryLegionaryLegions > 0) {
		if (Terrain_hasRoadAccess(b->x, b->y, b->size, 0, 0) && b->roadNetworkId == roadNetworkId) {
			return Data_CityInfo.buildingBarracksBuildingId;
		}
	}
	return 0;
}

void Resource_addRawMaterialToWorkshop(int buildingId)
{
	if (buildingId > 0 && BuildingIsWorkshop(buildingId)) {
		Data_Buildings[buildingId].loadsStored++;
	}
}

void Resource_addWeaponToBarracks(int buildingId)
{
	if (buildingId > 0) {
		Data_Buildings[buildingId].loadsStored++;
	}
}

int Resource_getDistance(int x1, int y1, int x2, int y2, int distToEntry1, int distToEntry2)
{
	int diff;
	if (distToEntry1 > distToEntry2) {
		diff = distToEntry1 - distToEntry2;
	} else {
		diff = distToEntry2 - distToEntry2;
	}
	if (distToEntry1 == -1) {
		diff = 0;
	}
	return diff + Calc_distanceMaximum(x1, y1, x2, y2);
}

int Resource_determineWarehouseWorkerTask(int buildingId, int *resource)
{
	// TODO
	return 0;
}

int Resource_determineGranaryWorkerTask(int buildingId)
{
	// TODO
	return 0;
}
