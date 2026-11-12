#pragma once

namespace PopupText
{
enum PopupType { DISABLED = 0, HINT = 1, OBJECTIVE = 2, SELECTION = 3, MAX_POPUP_TYPES = 4 };
void Init();
bool IsEnabled();
bool GetPreventJump();
void SetPreventJump(bool bPrevent);
void Update(float fDeltaTime);
void ShowPopup(PopupType eType, unsigned int uTextId);
void DisablePopup();
void AddHint(unsigned int uHint);
int GetNumHints();
}
