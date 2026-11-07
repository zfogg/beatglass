if(pipewire_test_cases)
  set_tests_properties(${pipewire_test_cases} PROPERTIES LABELS "unit;platform;pipewire")
endif()
