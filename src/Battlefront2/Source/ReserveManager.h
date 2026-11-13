#pragma once

#include "GameObject.h"

namespace ReserveManager
{
const GameObject* GetFirstReserver(const GameObject* pObj, ReserveType eType);
}
