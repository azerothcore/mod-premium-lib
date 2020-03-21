AC_ADD_SCRIPT("${CMAKE_CURRENT_LIST_DIR}/src/PremiumLib.cpp")

AC_ADD_SCRIPT_LOADER("PremiumLib" "${CMAKE_CURRENT_LIST_DIR}/src/loader.h")

AC_ADD_CONFIG_FILE("${CMAKE_CURRENT_LIST_DIR}/conf/mod_premium_lib.conf.dist")