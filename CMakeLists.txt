AC_ADD_SCRIPT("${CMAKE_CURRENT_LIST_DIR}/src/PremiumCommands.cpp")
AC_ADD_SCRIPT_LOADER("PremiumCommands" "${CMAKE_CURRENT_LIST_DIR}/src/loader.h")

AC_ADD_CONFIG_FILE("${CMAKE_CURRENT_LIST_DIR}/conf/my_custom.conf.dist")