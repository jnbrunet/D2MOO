
D2MOO_prepare_targets(D2Client)

target_link_libraries(${D2ClientImplName}
  PRIVATE
    D2CommonDefinitions
    D2CMP
    D2Common
    D2Lang
    D2Net
    Fog
    winmm # For timeGetTime()
)

# Game
target_sources(${D2ClientImplName}
  PRIVATE
    src/D2Client.Offsets.cpp
    src/GAME/SCmd.cpp
    src/UI/automap.cpp
    src/UNIT/CUnit.cpp

    include/D2Client.h
    include/GAME/SCmd.h
    include/UI/automap.h
    include/UNIT/CUnit.h
)


D2MOO_target_source_group(D2Client)
