#pragma once

#include "./plebux/CSGOStructs.hpp"
#include "Utils.hpp"

namespace CircleStrafe
{
	extern float RightMovement;
	extern bool IsActive;
	extern float StrafeAngle;
	void Start(SourceEngine::CUserCmd* pCmd);
	void Strafe(SourceEngine::CUserCmd* pCmd);
};

