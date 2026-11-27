#include "Main.h";

Vehicle createVehicle(char* modelName, Vector3 pos, float heading)
{
	Hash model = GAMEPLAY::GET_HASH_KEY(modelName);
	return createVehicle(model, pos, heading);;
}

Vehicle createVehicle(VehicleHash vehicleHash, Vector3 position, float heading)
{
	return createVehicle((Hash)vehicleHash, position, heading);
}

Vehicle createVehicle(Hash model, Vector3 pos, float heading)
{
	Vehicle veh;

	if (!STREAMING::HAS_MODEL_LOADED(model))
	{
		STREAMING::REQUEST_MODEL(model, false);
	}

	// Bounded wait. An unbounded loop here is what made the game freeze when a
	// poster was accepted: the rivals spawn requests a horse model, and when that
	// model never finishes loading the script parked in this loop for good.
	int loadGuard = 0;
	while (!STREAMING::HAS_MODEL_LOADED(model) && loadGuard < 120)
	{
		WAIT(0);
		loadGuard++;
	}
	if (!STREAMING::HAS_MODEL_LOADED(model))
	{
		{
			std::stringstream sv;
			sv << "createVehicle: model failed to load, hash=" << (UINT32)model;
			log(sv.str());
		}
		return 0;
	}

	veh = VEHICLE::CREATE_VEHICLE(model, pos.x, pos.y, pos.z, heading, true, true, false, false);
	VEHICLE::SET_VEHICLE_ON_GROUND_PROPERLY(veh, 0);

	return veh;
}

void setVehicleCargo(Vehicle vehicle, VehicleCargoHash cargoHash)
{
	PROP::_0xD80FAF919A2E56EA(vehicle, cargoHash);
}