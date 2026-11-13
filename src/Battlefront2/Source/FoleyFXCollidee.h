#pragma once

class FoleyFXCollideeClass;

class FoleyFXCollidee
{
public:

    FoleyFXCollidee(FoleyFXCollideeClass* pClass);

    FoleyFXCollideeClass* GetFoleyFXCollideeClass() const;

    FoleyFXCollideeClass* mCollideeClass;
};
