#pragma once

// D2Client + 0x15960 -> 6FAB5960
BOOL D2Client_UpdateCameraOriginFromOpenPanels();

// D2Client + 0x15A20 -> 6FAB5A20
// IDA currently names this routine D2Client_GetSelectedUnit; keep existing D2MOO declaration name for compatibility.
D2UnitStrc *D2Client_GetSelectedItem();
