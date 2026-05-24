set(QEVERCLOUD_FIND_PACKAGE_ARG "VERBOSE")

find_package(Qt6Core REQUIRED)
message(STATUS "Found Qt6 installation, version ${Qt6Core_VERSION}")

include(QEverCloudFindPackageWrapperMacro)
include(QEverCloudFindQt6DependenciesCore)

list(APPEND QT_INCLUDES
  ${Qt6Core_INCLUDE_DIRS}
  ${Qt6Network_INCLUDE_DIRS}
  ${Qt6Widgets_INCLUDE_DIRS})

list(APPEND QT_LIBRARIES
  Qt6::Core
  Qt6::Network
  Qt6::Widgets)

list(APPEND QT_DEFINITIONS
  ${Qt6Core_DEFINITIONS}
  ${Qt6Network_DEFINITIONS}
  ${Qt6Widgets_DEFINITIONS})

if(BUILD_TRANSLATIONS)
  include(QEverCloudFindQt6DependenciesTranslations)

  list(APPEND QT_INCLUDES
    ${Qt6LinguistTools_INCLUDE_DIRS})

  list(APPEND QT_LIBRARIES
    ${Qt6LinguistTools_LIBRARIES})

  list(APPEND QT_DEFINITIONS
    ${Qt6LinguistTools_DEFINITIONS})
endif()

if(BUILD_WITH_OAUTH_SUPPORT AND NOT QEVERCLOUD_USE_SYSTEM_BROWSER)
  include(QEverCloudFindQt6DependenciesWebEngineCore)
  set(QEVERCLOUD_USE_QT_WEB_ENGINE TRUE)

  if(QEVERCLOUD_USE_QT_WEB_ENGINE)
    add_definitions(-DQEVERCLOUD_USE_QT_WEB_ENGINE)
  endif()

  list(APPEND QT_INCLUDES
    ${QT_INCLUDES}
    ${Qt6WebEngineCore_INCLUDE_DIRS}
    ${Qt6WebEngineWidgets_INCLUDE_DIRS})

  list(APPEND QT_LIBRARIES
    ${QT_LIBRARIES}
    Qt6::WebEngineCore
    Qt6::WebEngineWidgets)

  list(APPEND QT_DEFINITIONS
    ${QT_DEFINITIONS}
    ${Qt6WebEngineCore_DEFINITIONS}
    ${Qt6WebEngineWidgets_DEFINITIONS})
endif()

list(REMOVE_DUPLICATES QT_INCLUDES)
list(REMOVE_DUPLICATES QT_LIBRARIES)
list(REMOVE_DUPLICATES QT_DEFINITIONS)

include_directories(SYSTEM "${QT_INCLUDES} ${SYSTEM}")
add_definitions(${QT_DEFINITIONS})

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR "ON")
