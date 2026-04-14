#pragma once

// D2Client.dll + 0x15960 (0x6FAB5960)
BOOL SELECT_UpdateCameraOriginFromOpenPanels();

// D2Client.dll + 0x15A20 (0x6FAB5A20)
// IDA currently names this routine D2Client_GetSelectedUnit; keep existing D2MOO declaration name for compatibility.
D2UnitStrc *SELECT_GetSelectedItem();
