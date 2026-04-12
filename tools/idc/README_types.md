# D2MOO -> IDA : Applying function type signatures
## Files generated
| File                      | Content                                    |
|---------------------------|--------------------------------------------|
| D2Common_types.idc        | 1785 SetType() calls for D2Common          |
| D2Game_types.idc          | 3180 SetType() calls for D2Game            |
| d2moo_types_for_ida.h     | Forward declarations for all D2MOO types   |

## Recommended workflow (two steps)

### Step 1: Load d2moo_types_for_ida.h into IDA
This tells IDA about all D2MOO custom struct types so SetType() can accept
function signatures that reference them (D2UnitStrc*, D2StatListStrc*, etc.).

  a. In IDA: **File > Load file > Parse C header file**
  b. In the dialog, set:
       - Compiler: **Visual C++**
       - Target:   **x86 (32-bit)**
  c. Select **`tools/idc/d2moo_types_for_ida.h`**
  d. Click OK — IDA adds ~200 types to the local type library.
     Verify: **View > Open subviews > Local types** (Shift+F1)

### Step 2: Run the IDC scripts
  1. **File > Script file...** (Alt+F7)
  2. Select `D2Common_types.idc`
  3. Check Output window:
       `[D2Common] Types applied: 1785 ok, 0 failed`
  4. Repeat for `D2Game_types.idc`

## What was fixed vs the first run
After the first run (1666 ok, 119 failed), these issues were identified
and fixed in the generator (`gen_ida_idc_types.ps1`):

| Problem                          | Fix                                      |
|----------------------------------|------------------------------------------|
| `D2SLayerStatIdStrc::PackedType` | Replaced with `int32_t` in generator    |
| `unsigned __stdcall foo()`       | Replaced with `unsigned int __stdcall`  |
| Unknown struct types in SetType  | Covered by loading d2moo_types_for_ida.h |
| `StatListValueChangeFunc` typedef| Defined in d2moo_types_for_ida.h        |

## Notes
- SetType() applies the full C signature: return type, calling convention,
  function name, and all parameter types.
- Run D2Common.idc (rename via ordinals) independently — order doesn't matter.
- Regenerate IDC files after D2MOO header changes:
    `powershell -ExecutionPolicy Bypass -File tools\gen_ida_idc_types.ps1`
- The d2moo_types_for_ida.h uses only plain C syntax (no C++ features)
  so IDA's parser handles it correctly.

## Calling conventions in D2MOO
  `__stdcall`   -- most exported functions (cleaned up ABI)
  `__fastcall`  -- most internal functions (original ABI)
  `__cdecl`     -- rare (variadic functions, some callbacks)
