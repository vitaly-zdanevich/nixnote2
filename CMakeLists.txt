# CMake project file for IDEs and alternate builds.

# additional options to be set before use:
# EXAMPLE:
# cmake flags:
# -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH=/d/dev/Qt/6.10/gcc_64;/d/dev/libs
# environment:
# PKG_CONFIG_PATH=/d/dev/libs/lib/pkgconfig
# where in example "/d/dev/libs" is path where additional libs are installed (e.g. tidy)
# and "/d/dev/Qt/6.10/gcc_64" is example of Qt root

cmake_minimum_required (VERSION 3.16)
project (nixnote2)
add_definitions(-std=c++17)

find_package (Qt6 REQUIRED COMPONENTS
        Widgets
        Gui
        Sql
        Network
        WebEngineWidgets
        WebChannel
        PrintSupport
        Xml
        DBus
        Qml
        Concurrent
        Test
)

find_package(PkgConfig REQUIRED)
pkg_check_modules(TIDY REQUIRED tidy)
pkg_check_modules(POPPLER_QT6 REQUIRED poppler-qt6)
pkg_check_modules(HUNSPELL REQUIRED hunspell)
pkg_check_modules(LIBCURL REQUIRED libcurl)

set(QEVERCLOUD_USE_QT_WEB_ENGINE 0 CACHE STRING "Use QtWebEngine OAuth backend")
set(QEVERCLOUD_USE_SYSTEM_BROWSER 1 CACHE STRING "Use system browser OAuth backend")

if(QEVERCLOUD_USE_QT_WEB_ENGINE AND QEVERCLOUD_USE_SYSTEM_BROWSER)
        message(FATAL_ERROR "Choose either QtWebEngine OAuth or system browser OAuth, not both")
endif()

if(NOT QEVERCLOUD_USE_QT_WEB_ENGINE AND NOT QEVERCLOUD_USE_SYSTEM_BROWSER)
        message(FATAL_ERROR "Choose either QtWebEngine OAuth or system browser OAuth")
endif()

if(QEVERCLOUD_USE_QT_WEB_ENGINE)
        set(QEVERCLOUD_USE_QT_WEB_ENGINE_VALUE 1)
else()
        set(QEVERCLOUD_USE_QT_WEB_ENGINE_VALUE 0)
endif()

if(QEVERCLOUD_USE_SYSTEM_BROWSER)
        set(QEVERCLOUD_USE_SYSTEM_BROWSER_VALUE 1)
else()
        set(QEVERCLOUD_USE_SYSTEM_BROWSER_VALUE 0)
endif()

file(READ
        ${PROJECT_SOURCE_DIR}/src/qevercloud/QEverCloud/headers/VersionInfo.h.in
        QEVERCLOUD_VERSION_INFO_H)
string(REPLACE "$$QEVERCLOUD_USE_QT_WEB_ENGINE" "${QEVERCLOUD_USE_QT_WEB_ENGINE_VALUE}"
        QEVERCLOUD_VERSION_INFO_H "${QEVERCLOUD_VERSION_INFO_H}")
string(REPLACE "$$QEVERCLOUD_USE_SYSTEM_BROWSER" "${QEVERCLOUD_USE_SYSTEM_BROWSER_VALUE}"
        QEVERCLOUD_VERSION_INFO_H "${QEVERCLOUD_VERSION_INFO_H}")
file(WRITE ${PROJECT_BINARY_DIR}/VersionInfo.h "${QEVERCLOUD_VERSION_INFO_H}")

set(QEVERCLOUD_SERVICES_MOC ${PROJECT_BINARY_DIR}/Services.moc)
add_custom_command(
        OUTPUT ${QEVERCLOUD_SERVICES_MOC}
        COMMAND Qt6::moc
                -DQEVERCLOUD_USE_QT_WEB_ENGINE=${QEVERCLOUD_USE_QT_WEB_ENGINE_VALUE}
                -DQEVERCLOUD_USE_SYSTEM_BROWSER=${QEVERCLOUD_USE_SYSTEM_BROWSER_VALUE}
                -I${PROJECT_SOURCE_DIR}
                -I${PROJECT_SOURCE_DIR}/src/qevercloud/QEverCloud/headers
                -I${PROJECT_BINARY_DIR}
                ${PROJECT_SOURCE_DIR}/src/qevercloud/QEverCloud/src/generated/Services.cpp
                -o ${QEVERCLOUD_SERVICES_MOC}
        DEPENDS
                ${PROJECT_SOURCE_DIR}/src/qevercloud/QEverCloud/src/generated/Services.cpp
                ${PROJECT_SOURCE_DIR}/src/qevercloud/QEverCloud/headers/generated/Services.h
        VERBATIM)
add_custom_target(qevercloud_services_moc DEPENDS ${QEVERCLOUD_SERVICES_MOC})

set (nixnote2_src
        src/application.cpp
        src/global.cpp
        src/main.cpp
        src/nixnote.cpp
        src/cmdtools/addnote.cpp
        src/cmdtools/alternote.cpp
        src/cmdtools/cmdlinequery.cpp
        src/cmdtools/cmdlinetool.cpp
        src/cmdtools/deletenote.cpp
        src/cmdtools/emailnote.cpp
        src/cmdtools/extractnotes.cpp
        src/cmdtools/extractnotetext.cpp
        src/cmdtools/importnotes.cpp
        src/cmdtools/signalgui.cpp
        src/communication/communicationerror.cpp
        src/communication/communicationmanager.cpp
        src/dialog/aboutdialog.cpp
        src/dialog/accountdialog.cpp
        src/dialog/accountmaintenancedialog.cpp
        src/dialog/adduseraccountdialog.cpp
        src/dialog/closenotebookdialog.cpp
        src/dialog/databasestatus.cpp
        src/dialog/emaildialog.cpp
        src/dialog/encryptdialog.cpp
        src/dialog/endecryptdialog.cpp
        src/dialog/faderdialog.cpp
        src/dialog/htmlentitiesdialog.cpp
        src/dialog/insertlatexdialog.cpp
        src/dialog/insertlinkdialog.cpp
        src/dialog/locationdialog.cpp
        src/dialog/logindialog.cpp
        src/dialog/notebookproperties.cpp
        src/dialog/notehistoryselect.cpp
        src/dialog/noteproperties.cpp
        src/dialog/preferences/appearancepreferences.cpp
        src/dialog/preferences/debugpreferences.cpp
        src/dialog/preferences/emailpreferences.cpp
        src/dialog/preferences/exitpreferences.cpp
        src/dialog/preferences/localepreferences.cpp
        src/dialog/preferences/preferencesdialog.cpp
        src/dialog/preferences/searchpreferences.cpp
        src/dialog/preferences/syncpreferences.cpp
        src/dialog/preferences/thumbnailpreferences.cpp
        src/dialog/remindersetdialog.cpp
        src/dialog/savedsearchproperties.cpp
        src/dialog/shortcutdialog.cpp
        src/dialog/spellcheckdialog.cpp
        src/dialog/tabledialog.cpp
        src/dialog/tagproperties.cpp
        src/dialog/watchfolderadd.cpp
        src/dialog/watchfolderdialog.cpp
        src/email/emailaddress.cpp
        src/email/mimeattachment.cpp
        src/email/mimecontentformatter.cpp
        src/email/mimefile.cpp
        src/email/mimehtml.cpp
        src/email/mimeinlinefile.cpp
        src/email/mimemessage.cpp
        src/email/mimemultipart.cpp
        src/email/mimepart.cpp
        src/email/mimetext.cpp
        src/email/quotedprintable.cpp
        src/email/smtpclient.cpp
        src/exits/exitmanager.cpp
        src/exits/exitpoint.cpp
        src/filters/filtercriteria.cpp
        src/filters/filterengine.cpp
        src/filters/notesortfilterproxymodel.cpp
        src/filters/remotequery.cpp
        src/gui/browserWidgets/authoreditor.cpp
        src/gui/browserWidgets/colormenu.cpp
        src/gui/browserWidgets/dateeditor.cpp
        src/gui/browserWidgets/datetimeeditor.cpp
        src/gui/browserWidgets/editorbuttonbar.cpp
        src/gui/browserWidgets/expandbutton.cpp
        src/gui/browserWidgets/fontnamecombobox.cpp
        src/gui/browserWidgets/fontsizecombobox.cpp
        src/gui/browserWidgets/locationeditor.cpp
        src/gui/browserWidgets/notebookmenubutton.cpp
        src/gui/browserWidgets/ntitleeditor.cpp
        src/gui/browserWidgets/reminderbutton.cpp
        src/gui/browserWidgets/table/tablepropertiesdialog.cpp
        src/gui/browserWidgets/tageditor.cpp
        src/gui/browserWidgets/tageditornewtag.cpp
        src/gui/browserWidgets/tagviewer.cpp
        src/gui/browserWidgets/urleditor.cpp
        src/gui/datedelegate.cpp
        src/gui/externalbrowse.cpp
        src/gui/favoritesview.cpp
        src/gui/favoritesviewdelegate.cpp
        src/gui/favoritesviewitem.cpp
        src/gui/findreplace.cpp
        src/gui/flowlayout.cpp
        src/gui/imagedelegate.cpp
        src/gui/lineedit.cpp
        src/gui/nattributetree.cpp
        src/gui/nbrowserwindow.cpp
        src/gui/nmainmenubar.cpp
        src/gui/nnotebookview.cpp
        src/gui/nnotebookviewdelegate.cpp
        src/gui/nnotebookviewitem.cpp
        src/gui/nsearchview.cpp
        src/gui/nsearchviewitem.cpp
        src/gui/ntableview.cpp
        src/gui/ntableviewheader.cpp
        src/gui/ntabwidget.cpp
        src/gui/ntagview.cpp
        src/gui/ntagviewdelegate.cpp
        src/gui/ntagviewitem.cpp
        src/gui/ntrashtree.cpp
        src/gui/ntrashviewdelegate.cpp
        src/gui/numberdelegate.cpp
        src/gui/nwebpage.cpp
        src/gui/nwebview.cpp
        src/gui/plugins/pluginfactory.cpp
        src/gui/plugins/popplergraphicsview.cpp
        src/gui/plugins/popplerviewer.cpp
        src/gui/reminderorderdelegate.cpp
        src/gui/shortcutkeys.cpp
        src/gui/traymenu.cpp
        src/gui/treewidgeteditor.cpp
        src/gui/truefalsedelegate.cpp
        src/gui/widgetpanel.cpp
        src/logger/qsdebugoutput.cpp
        src/logger/qslog.cpp
        src/logger/qslogdest.cpp
        src/models/notecache.cpp
        src/models/notemodel.cpp
        src/models/ntreemodel.cpp
        src/oauth/oauthtokenizer.cpp
        src/hunspell/spellchecker.cpp
        src/qevercloud/QEverCloud/src/AsyncResult.cpp
        src/qevercloud/QEverCloud/src/AsyncResult_p.cpp
        src/qevercloud/QEverCloud/src/DurableService.cpp
        src/qevercloud/QEverCloud/src/EventLoopFinisher.cpp
        src/qevercloud/QEverCloud/src/EverCloudException.cpp
        src/qevercloud/QEverCloud/src/Exceptions.cpp
        src/qevercloud/QEverCloud/src/generated/Constants.cpp
        src/qevercloud/QEverCloud/src/generated/EDAMErrorCode.cpp
        src/qevercloud/QEverCloud/src/generated/Servers.cpp
        src/qevercloud/QEverCloud/src/generated/Services.cpp
        src/qevercloud/QEverCloud/src/generated/Types.cpp
        src/qevercloud/QEverCloud/src/Globals.cpp
        src/qevercloud/QEverCloud/src/Http.cpp
        src/qevercloud/QEverCloud/src/HttpRequestParser.cpp
        src/qevercloud/QEverCloud/src/HttpUtils.cpp
        src/qevercloud/QEverCloud/src/InkNoteImageDownloader.cpp
        src/qevercloud/QEverCloud/src/Log.cpp
        src/qevercloud/QEverCloud/src/Printable.cpp
        src/qevercloud/QEverCloud/src/RequestContext.cpp
        src/qevercloud/QEverCloud/src/Thumbnail.cpp
        src/qevercloud/QEverCloud/src/VersionInfo.cpp
        src/qevercloud/QEverCloud/src/oauth/AbstractOAuthEngine.cpp
        src/qevercloud/QEverCloud/src/oauth/NonceGenerator.cpp
        src/qevercloud/QEverCloud/src/oauth/OAuth.cpp
        src/qevercloud/QEverCloud/src/oauth/NetworkCookieJar.cpp
        src/qevercloud/QEverCloud/src/oauth/Utils.cpp
        src/reminders/reminderevent.cpp
        src/reminders/remindermanager.cpp
        src/settings/accountsmanager.cpp
        src/settings/colorsettings.cpp
        src/settings/filemanager.cpp
        src/settings/startupconfig.cpp
        src/sql/configstore.cpp
        src/sql/databaseconnection.cpp
        src/sql/databaseupgrade.cpp
        src/sql/datastore.cpp
        src/sql/favoritesrecord.cpp
        src/sql/favoritestable.cpp
        src/sql/filewatchertable.cpp
        src/sql/linkednotebooktable.cpp
        src/sql/notebooktable.cpp
        src/sql/notemetadata.cpp
        src/sql/notetable.cpp
        src/sql/nsqlquery.cpp
        src/sql/resourcetable.cpp
        src/sql/searchtable.cpp
        src/sql/sharednotebooktable.cpp
        src/sql/tagtable.cpp
        src/sql/usertable.cpp
        src/html/attachmenticonbuilder.cpp
        src/html/enmlformatter.cpp
        src/html/htmldom.cpp
        src/html/NoteFormatterBase.cpp
        src/html/noteformatter.cpp
        src/html/tagscanner.cpp
        src/html/thumbnailer.cpp
        src/threads/browserrunner.cpp
        src/threads/counterrunner.cpp
        src/threads/indexrunner.cpp
        src/threads/syncrunner.cpp
        src/utilities/crossmemorymapper.cpp
        src/utilities/debugtool.cpp
        src/utilities/encrypt.cpp
        src/utilities/mimereference.cpp
        src/utilities/noteindexer.cpp
        src/utilities/nuuid.cpp
        src/utilities/pixelconverter.cpp
        src/utilities/NixnoteStringUtils.cpp
        src/watcher/filewatcher.cpp
        src/watcher/filewatchermanager.cpp
        src/xml/batchimport.cpp
        src/xml/exportdata.cpp
        src/xml/importdata.cpp
        src/xml/importenex.cpp
        src/xml/xmlhighlighter.cpp
        src/quentier/utility/StringUtils.cpp
        src/quentier/utility/StringUtils_p.cpp
)

if(QEVERCLOUD_USE_QT_WEB_ENGINE)
        list(APPEND nixnote2_src
                src/qevercloud/QEverCloud/src/oauth/OAuthWebEngine.cpp)
elseif(QEVERCLOUD_USE_SYSTEM_BROWSER)
        list(APPEND nixnote2_src
                src/qevercloud/QEverCloud/src/oauth/OAuthSystemBrowser.cpp)
endif()

set(nixnote2_test_src
        testsrc/tests.cpp
        src/html/enmlformatter.cpp
        src/html/htmldom.cpp
        src/logger/qslog.cpp
        src/logger/qslogdest.cpp
        src/logger/qsdebugoutput.cpp
        src/utilities/NixnoteStringUtils.cpp
        src/utilities/encrypt.cpp
)
set (nixnote2_hdr
        src/application.h
        src/nixnote.h
        src/global.h
        src/cmdtools/addnote.h
        src/cmdtools/alternote.h
        src/cmdtools/cmdlinequery.h
        src/cmdtools/cmdlinetool.h
        src/cmdtools/deletenote.h
        src/cmdtools/emailnote.h
        src/cmdtools/extractnotes.h
        src/cmdtools/extractnotetext.h
        src/cmdtools/importnotes.h
        src/cmdtools/signalgui.h
        src/communication/communicationerror.h
        src/communication/communicationmanager.h
        src/dialog/aboutdialog.h
        src/dialog/accountdialog.h
        src/dialog/accountmaintenancedialog.h
        src/dialog/adduseraccountdialog.h
        src/dialog/closenotebookdialog.h
        src/dialog/databasestatus.h
        src/dialog/emaildialog.h
        src/dialog/encryptdialog.h
        src/dialog/endecryptdialog.h
        src/dialog/faderdialog.h
        src/dialog/htmlentitiesdialog.h
        src/dialog/insertlatexdialog.h
        src/dialog/insertlinkdialog.h
        src/dialog/locationdialog.h
        src/dialog/logindialog.h
        src/dialog/notebookproperties.h
        src/dialog/notehistoryselect.h
        src/dialog/noteproperties.h
        src/dialog/preferences/appearancepreferences.h
        src/dialog/preferences/debugpreferences.h
        src/dialog/preferences/emailpreferences.h
        src/dialog/preferences/exitpreferences.h
        src/dialog/preferences/localepreferences.h
        src/dialog/preferences/preferencesdialog.h
        src/dialog/preferences/searchpreferences.h
        src/dialog/preferences/syncpreferences.h
        src/dialog/preferences/thumbnailpreferences.h
        src/dialog/remindersetdialog.h
        src/dialog/savedsearchproperties.h
        src/dialog/shortcutdialog.h
        src/dialog/spellcheckdialog.h
        src/dialog/tabledialog.h
        src/dialog/tagproperties.h
        src/dialog/watchfolderadd.h
        src/dialog/watchfolderdialog.h
        src/email/emailaddress.h
        src/email/mimeattachment.h
        src/email/mimecontentformatter.h
        src/email/mimefile.h
        src/email/mimehtml.h
        src/email/mimeinlinefile.h
        src/email/mimemessage.h
        src/email/mimemultipart.h
        src/email/mimepart.h
        src/email/mimetext.h
        src/email/quotedprintable.h
        src/email/smtpclient.h
        src/email/smtpexports.h
        src/exits/exitmanager.h
        src/exits/exitpoint.h
        src/filters/filtercriteria.h
        src/filters/filterengine.h
        src/filters/notesortfilterproxymodel.h
        src/filters/remotequery.h
        src/gui/browserWidgets/authoreditor.h
        src/gui/browserWidgets/colormenu.h
        src/gui/browserWidgets/dateeditor.h
        src/gui/browserWidgets/datetimeeditor.h
        src/gui/browserWidgets/editorbuttonbar.h
        src/gui/browserWidgets/expandbutton.h
        src/gui/browserWidgets/fontnamecombobox.h
        src/gui/browserWidgets/fontsizecombobox.h
        src/gui/browserWidgets/locationeditor.h
        src/gui/browserWidgets/notebookmenubutton.h
        src/gui/browserWidgets/ntitleeditor.h
        src/gui/browserWidgets/reminderbutton.h
        src/gui/browserWidgets/table/tablepropertiesdialog.h
        src/gui/browserWidgets/tageditor.h
        src/gui/browserWidgets/tageditornewtag.h
        src/gui/browserWidgets/tagviewer.h
        src/gui/browserWidgets/urleditor.h
        src/gui/datedelegate.h
        src/gui/externalbrowse.h
        src/gui/favoritesview.h
        src/gui/favoritesviewdelegate.h
        src/gui/favoritesviewitem.h
        src/gui/findreplace.h
        src/gui/flowlayout.h
        src/gui/imagedelegate.h
        src/gui/lineedit.h
        src/gui/nattributetree.h
        src/gui/nbrowserwindow.h
        src/gui/nmainmenubar.h
        src/gui/nnotebookview.h
        src/gui/nnotebookviewdelegate.h
        src/gui/nnotebookviewitem.h
        src/gui/nsearchview.h
        src/gui/nsearchviewitem.h
        src/gui/ntableview.h
        src/gui/ntableviewheader.h
        src/gui/ntabwidget.h
        src/gui/ntagview.h
        src/gui/ntagviewdelegate.h
        src/gui/ntagviewitem.h
        src/gui/ntrashtree.h
        src/gui/ntrashviewdelegate.h
        src/gui/numberdelegate.h
        src/gui/nwebpage.h
        src/gui/nwebview.h
        src/gui/plugins/pluginfactory.h
        src/gui/plugins/popplergraphicsview.h
        src/gui/plugins/popplerviewer.h
        src/gui/reminderorderdelegate.h
        src/gui/shortcutkeys.h
        src/gui/traymenu.h
        src/gui/treewidgeteditor.h
        src/gui/truefalsedelegate.h
        src/gui/widgetpanel.h
        src/logger/qsdebugoutput.h
        src/logger/qslog.h
        src/logger/qslogdest.h
        src/models/notecache.h
        src/models/notemodel.h
        src/models/ntreemodel.h
        src/oauth/oauthtokenizer.h
        src/hunspell/spellchecker.h
        src/qevercloud/QEverCloud/headers/AsyncResult.h
        src/qevercloud/QEverCloud/src/AsyncResult_p.h
        src/qevercloud/QEverCloud/headers/DurableService.h
        src/qevercloud/QEverCloud/headers/EventLoopFinisher.h
        src/qevercloud/QEverCloud/headers/EverCloudException.h
        src/qevercloud/QEverCloud/headers/Exceptions.h
        src/qevercloud/QEverCloud/headers/generated/Constants.h
        src/qevercloud/QEverCloud/headers/generated/EDAMErrorCode.h
        src/qevercloud/QEverCloud/headers/generated/Servers.h
        src/qevercloud/QEverCloud/headers/generated/Services.h
        src/qevercloud/QEverCloud/headers/generated/Types.h
        src/qevercloud/QEverCloud/src/generated/Types_io.h
        src/qevercloud/QEverCloud/src/Http.h
        src/qevercloud/QEverCloud/src/HttpRequestData.h
        src/qevercloud/QEverCloud/src/HttpRequestParser.h
        src/qevercloud/QEverCloud/src/HttpUtils.h
        src/qevercloud/QEverCloud/src/Impl.h
        src/qevercloud/QEverCloud/headers/QEverCloud.h
        src/qevercloud/QEverCloud/headers/QEverCloudOAuth.h
        src/qevercloud/QEverCloud/headers/OAuth.h
        src/qevercloud/QEverCloud/src/oauth/NetworkCookieJar.h
        src/qevercloud/QEverCloud/headers/Optional.h
        src/qevercloud/QEverCloud/headers/Printable.h
        src/qevercloud/QEverCloud/headers/Thumbnail.h
        src/qevercloud/QEverCloud/src/Thrift.h
        src/reminders/reminderevent.h
        src/reminders/remindermanager.h
        src/settings/accountsmanager.h
        src/settings/colorsettings.h
        src/settings/filemanager.h
        src/settings/startupconfig.h
        src/sql/configstore.h
        src/sql/databaseconnection.h
        src/sql/databaseupgrade.h
        src/sql/datastore.h
        src/sql/favoritesrecord.h
        src/sql/favoritestable.h
        src/sql/filewatchertable.h
        src/sql/linkednotebooktable.h
        src/sql/notebooktable.h
        src/sql/notemetadata.h
        src/sql/notetable.h
        src/sql/nsqlquery.h
        src/sql/resourcetable.h
        src/sql/searchtable.h
        src/sql/sharednotebooktable.h
        src/sql/tagtable.h
        src/sql/usertable.h
        src/html/attachmenticonbuilder.h
        src/html/enmlformatter.h
        src/html/htmldom.h
        src/html/NoteFormatterBase.h
        src/html/noteformatter.h
        src/html/tagscanner.h
        src/html/thumbnailer.h
        src/threads/browserrunner.h
        src/threads/counterrunner.h
        src/threads/indexrunner.h
        src/threads/syncrunner.h
        src/utilities/crossmemorymapper.h
        src/utilities/debugtool.h
        src/utilities/encrypt.h
        src/utilities/mimereference.h
        src/utilities/noteindexer.h
        src/utilities/nuuid.h
        src/utilities/pixelconverter.h
        src/utilities/NixnoteStringUtils.h
        src/watcher/filewatcher.h
        src/watcher/filewatchermanager.h
        src/xml/batchimport.h
        src/xml/exportdata.h
        src/xml/importdata.h
        src/xml/importenex.h
        src/xml/xmlhighlighter.h
        src/quentier/utility/StringUtils.h
        src/quentier/utility/StringUtils_p.h
)

if(QEVERCLOUD_USE_QT_WEB_ENGINE)
        list(APPEND nixnote2_hdr
                src/qevercloud/QEverCloud/src/oauth/OAuthWebEngine.h)
elseif(QEVERCLOUD_USE_SYSTEM_BROWSER)
        list(APPEND nixnote2_hdr
                src/qevercloud/QEverCloud/src/oauth/OAuthSystemBrowser.h)
endif()

set(nixnote2_test_hdr
        testsrc/tests.h
        src/html/enmlformatter.h
        src/html/htmldom.h
        src/logger/qslog.h
        src/logger/qslogdest.h
        src/logger/qsdebugoutput.h
        src/utilities/NixnoteStringUtils.h
        src/utilities/encrypt.h
)

qt6_wrap_cpp(nixnote2_hdr_moc ${nixnote2_hdr})
qt6_wrap_cpp(nixnote2_test_hdr_moc ${nixnote2_test_hdr})

include_directories (${PROJECT_BINARY_DIR})
include_directories (${PROJECT_SOURCE_DIR}/src/qevercloud/QEverCloud/headers)
include_directories (${PROJECT_SOURCE_DIR})
include_directories(${TIDY_INCLUDE_DIRS})
include_directories(${POPPLER_QT6_INCLUDE_DIRS})
include_directories(${HUNSPELL_INCLUDE_DIRS})
include_directories(${LIBCURL_INCLUDE_DIRS})

add_executable(nixnote2 ${nixnote2_src} ${nixnote2_hdr_moc})
add_executable(tests ${nixnote2_test_src} ${nixnote2_test_hdr_moc})
add_dependencies(nixnote2 qevercloud_services_moc)

target_compile_definitions(nixnote2 PRIVATE
        QEVERCLOUD_USE_QT_WEB_ENGINE=${QEVERCLOUD_USE_QT_WEB_ENGINE_VALUE}
        QEVERCLOUD_USE_SYSTEM_BROWSER=${QEVERCLOUD_USE_SYSTEM_BROWSER_VALUE}
)

target_link_libraries(nixnote2
        Qt6::Widgets
        Qt6::Sql
        Qt6::Gui
        Qt6::Network
        Qt6::WebEngineWidgets
        Qt6::WebChannel
        Qt6::PrintSupport
        Qt6::Xml
        Qt6::DBus
        Qt6::Qml
        Qt6::Concurrent
        ${TIDY_LIBRARIES}
        ${POPPLER_QT6_LIBRARIES}
        ${HUNSPELL_LIBRARIES}
        ${LIBCURL_LIBRARIES}
)
target_link_libraries(tests
        Qt6::Widgets
        Qt6::Sql
        Qt6::Gui
        Qt6::Network
        Qt6::WebEngineWidgets
        Qt6::WebChannel
        Qt6::PrintSupport
        Qt6::Xml
        Qt6::DBus
        Qt6::Qml
        Qt6::Concurrent
        Qt6::Test
        ${TIDY_LIBRARIES}
)
