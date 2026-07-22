# Console examples — each is a single main.cpp linking the qivot library.
foreach(ex tutorial1 tutorial2 tutorial3 tutorial4 tutorial5 schema jsonnested index)
    add_executable(${ex} ${ex}/main.cpp)
    target_link_libraries(${ex} PRIVATE Qivot::qivot)
endforeach()

# The threaded HTTP importer example needs the network-enabled library.
if(QIVOT_WITH_NETWORK)
    add_executable(jsonhttp jsonhttp/main.cpp)
    target_link_libraries(jsonhttp PRIVATE Qivot::qivot)
endif()

# The QML example (examples/qmlmodel) uses Qt Quick + QML type registration;
# build it with qmake (qmlmodel.pro). See the README.
