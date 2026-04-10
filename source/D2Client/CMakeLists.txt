
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
    src/UI/automap.cpp
    src/UNIT/CUnit.cpp

    include/D2Client.h
    include/UI/automap.h
    include/UNIT/CUnit.h
)


D2MOO_target_source_group(D2Client)
